#include "config.h"
#include <bitcoin/chainparams.h>
#include <ccan/err/err.h>
#include <ccan/opt/opt.h>
#include <ccan/tal/str/str.h>
#include <common/bech32_util.h>
#include <common/bolt12_merkle.h>
#include <common/configdir.h>
#include <common/features.h>
#include <common/iso4217.h>
#include <common/setup.h>
#include <common/type_to_string.h>
#include <common/version.h>
#include <inttypes.h>
#include <secp256k1_schnorrsig.h>
#include <stdio.h>
#include <time.h>

#define NO_ERROR 0
#define ERROR_BAD_DECODE 1
#define ERROR_USAGE 3

static bool well_formed = true;

/* Tal wrappers for opt. */
static void *opt_allocfn(size_t size)
{
	return tal_arr_label(NULL, char, size, TAL_LABEL("opt_allocfn", ""));
}

static void *tal_reallocfn(void *ptr, size_t size)
{
	if (!ptr)
		return opt_allocfn(size);
	tal_resize_(&ptr, 1, size, false);
	return ptr;
}

static void tal_freefn(void *ptr)
{
	tal_free(ptr);
}

static char *fmt_time(const tal_t *ctx, u64 time)
{
	/* ctime is not sane.  Take pointer, returns \n in string. */
	time_t t = time;
	const char *p = ctime(&t);

	return tal_fmt(ctx, "%.*s", (int)strcspn(p, "\n"), p);
}

static bool must_str(bool expected, const char *complaint, const char *fieldname)
{
	if (!expected) {
		fprintf(stderr, "%s %s\n", complaint, fieldname);
		well_formed = false;
		return false;
	}
	return true;
}

#define must_have(obj, field) \
	must_str((obj)->field != NULL, "Missing", stringify(field))
#define must_not_have(obj, field) \
	must_str((obj)->field == NULL, "Unnecessary", stringify(field))

static void print_chains(const struct bitcoin_blkid *chains)
{
	printf("chains:");
	for (size_t i = 0; i < tal_count(chains); i++) {
		printf(" %s", type_to_string(tmpctx, struct bitcoin_blkid, &chains[i]));
	}
	printf("\n");
}

static void print_chain(const struct bitcoin_blkid *chain)
{
	printf("chain: %s\n",
	       type_to_string(tmpctx, struct bitcoin_blkid, chain));
}

static bool print_amount(const struct bitcoin_blkid *chains,
			 const char *iso4217, u64 amount)
{
	const char *currency;
	unsigned int minor_unit;
	bool ok = true;

	/* BOLT-offers #12:
	 * - if the currency for `amount` is that of the first entry in `chains`:
	 *   - MUST specify `amount` in multiples of the minimum
	 *     lightning-payable unit (e.g. milli-satoshis for bitcoin).
	 * - otherwise:
	 *   - MUST specify `iso4217` as an ISO 4712 three-letter code.
	 *   - MUST specify `amount` in the currency unit adjusted by the
	 *     ISO 4712 exponent (e.g. USD cents).
	 */
	if (!iso4217) {
		if (tal_count(chains) == 0)
			currency = "bc";
		else {
			const struct chainparams *ch;

			ch = chainparams_by_chainhash(&chains[0]);
			if (!ch) {
				currency = tal_fmt(tmpctx, "UNKNOWN CHAINHASH %s",
						   type_to_string(tmpctx,
								  struct bitcoin_blkid,
								  &chains[0]));
				ok = false;
			} else
				currency = ch->lightning_hrp;
		}
		minor_unit = 11;
	} else {
		const struct iso4217_name_and_divisor *iso;
		iso = find_iso4217(iso4217, tal_bytelen(iso4217));
		if (iso) {
			minor_unit = iso->minor_unit;
			currency = iso->name;
		} else {
			minor_unit = 0;
			currency = tal_fmt(tmpctx, "%.*s (UNKNOWN CURRENCY)",
					   (int)tal_bytelen(iso4217), iso4217);
			ok = false;
		}
	}

	if (!minor_unit)
		printf("amount: %"PRIu64"%s\n", amount, currency);
	else {
		u64 minor_div = 1;
		for (size_t i = 0; i < minor_unit; i++)
			minor_div *= 10;
		printf("amount: %"PRIu64".%.*"PRIu64"%s\n",
		       amount / minor_div, minor_unit, amount % minor_div,
		       currency);
	}

	return ok;
}

static void print_description(const char *description)
{
	printf("description: %.*s\n",
	       (int)tal_bytelen(description), description);
}

static void print_issuer(const char *issuer)
{
	printf("issuer: %.*s\n", (int)tal_bytelen(issuer), issuer);
}

static void print_node_id(const struct pubkey *node_id)
{
	printf("node_id: %s\n", type_to_string(tmpctx, struct pubkey, node_id));
}

static void print_quantity_max(u64 max)
{
	printf("quantity_max: %"PRIu64"\n", max);
}

static bool print_recurrance(const struct recurrence *recurrence,
			     const struct recurrence_paywindow *paywindow,
			     const u32 *limit,
			     const struct recurrence_base *base)
{
	const char *unit;
	bool ok = true;

	/* BOLT-offers-recurrence #12:
	 * Thus, each offer containing a recurring payment has:
	 * 1. A `time_unit` defining 0 (seconds), 1 (days), 2 (months),
	 *    3 (years).
	 * 2. A `period`, defining how often (in `time_unit`) it has to be paid.
	 * 3. An optional `recurrence_limit` of total payments to be paid.
	 * 4. An optional `recurrence_base`:
	 *    * `basetime`, defining when the first period starts
	 *       in seconds since 1970-01-01 UTC.
	 *    * `start_any_period` if non-zero, meaning you don't have to start
	 *       paying at the period indicated by `basetime`, but can use
	 *       `recurrence_start` to indicate what period you are starting at.
	 * 5. An optional `recurrence_paywindow`:
	 *    * `seconds_before`, defining how many seconds prior to the start of
	 *       the period a payment will be accepted.
	 *    * `proportional_amount`, if set indicating that a payment made
	 *       during the period itself will be charged proportionally to the
	 *       remaining time in the period (e.g. 150 seconds into a 1500
	 *       second period gives a 10% discount).
	 *    * `seconds_after`, defining how many seconds after the start of the
	 *       period a payment will be accepted.
	 *   If this field is missing, payment will be accepted during the prior
	 *   period and the paid-for period.
	 */
	switch (recurrence->time_unit) {
	case 0:
		unit = "seconds";
		break;
	case 1:
		unit = "days";
		break;
	case 2:
		unit = "months";
		break;
	case 3:
		unit = "years";
		break;
	default:
		fprintf(stderr, "recurrence: unknown time_unit %u", recurrence->time_unit);
		unit = "";
		ok = false;
	}
	printf("recurrence: every %u %s", recurrence->period, unit);
	if (limit)
		printf(" limit %u", *limit);
	if (base) {
		printf(" start %"PRIu64" (%s)",
		       base->basetime,
		       fmt_time(tmpctx, base->basetime));
		if (base->start_any_period)
			printf(" (can start any period)");
	}
	if (paywindow) {
		printf(" paywindow -%u to +%u",
		       paywindow->seconds_before, paywindow->seconds_after);
		if (paywindow->proportional_amount)
			printf(" (pay proportional)");
	}
	printf("\n");

	return ok;
}

static void print_absolute_expiry(u64 expiry)
{
	printf("absolute_expiry: %"PRIu64" (%s)\n",
	       expiry, fmt_time(tmpctx, expiry));
}

static void print_features(const u8 *features)
{
	printf("features:");
	for (size_t i = 0; i < tal_bytelen(features) * CHAR_BIT; i++) {
		if (feature_is_set(features, i))
			printf(" %zu", i);
	}
	printf("\n");
}

static bool print_blindedpaths(struct blinded_path **paths,
			       struct blinded_payinfo **blindedpay)
{
	size_t bp_idx = 0;

	for (size_t i = 0; i < tal_count(paths); i++) {
		struct onionmsg_hop **p = paths[i]->path;
		printf("blindedpath %zu/%zu: blinding %s",
		       i, tal_count(paths),
		       type_to_string(tmpctx, struct pubkey,
				      &paths[i]->blinding));
		printf("blindedpath %zu/%zu: path ",
		       i, tal_count(paths));
		for (size_t j = 0; j < tal_count(p); j++) {
			printf(" %s:%s",
			       type_to_string(tmpctx, struct pubkey,
					      &p[j]->blinded_node_id),
			       tal_hex(tmpctx, p[j]->encrypted_recipient_data));
			if (blindedpay) {
				if (bp_idx < tal_count(blindedpay))
					printf("fee=%u/%u,cltv=%u,features=%s",
					       blindedpay[bp_idx]->fee_base_msat,
					       blindedpay[bp_idx]->fee_proportional_millionths,
					       blindedpay[bp_idx]->cltv_expiry_delta,
					       tal_hex(tmpctx,
						       blindedpay[bp_idx]->features));
				bp_idx++;
			}
		}
		printf("\n");
	}
	if (blindedpay && tal_count(blindedpay) != bp_idx) {
		fprintf(stderr, "Expected %zu blindedpay fields, got %zu\n",
			bp_idx, tal_count(blindedpay));
		return false;
	}
	return true;
}

static bool print_signature(const char *messagename,
			    const char *fieldname,
			    const struct tlv_field *fields,
			    const struct pubkey *node_id,
			    const struct bip340sig *sig)
{
	struct sha256 m, shash;

	/* No key, it's already invalid */
	if (!node_id)
		return false;

	merkle_tlv(fields, &m);
	sighash_from_merkle(messagename, fieldname, &m, &shash);
	if (!check_schnorr_sig(&shash, &node_id->pubkey, sig)) {
		fprintf(stderr, "%s: INVALID\n", fieldname);
		return false;
	}
	printf("%s: %s\n",
	       fieldname,
	       type_to_string(tmpctx, struct bip340sig, sig));
	return true;
}

static void print_quantity(u64 q)
{
	printf("quantity: %"PRIu64"\n", q);
}

static void print_recurrence_counter(const u32 *recurrence_counter,
				     const u32 *recurrence_start)
{
	printf("recurrence_counter: %u", *recurrence_counter);
	if (recurrence_start)
		printf(" (start +%u)", *recurrence_start);
	printf("\n");
}

static bool print_recurrence_counter_with_base(const u32 *recurrence_counter,
					       const u32 *recurrence_start,
					       const u64 *recurrence_base)
{
	if (!recurrence_base) {
		fprintf(stderr, "Missing recurrence_base\n");
		return false;
	}
	printf("recurrence_counter: %u", *recurrence_counter);
	if (recurrence_start)
		printf(" (start +%u)", *recurrence_start);
	printf(" (base %"PRIu64")\n", *recurrence_base);
	return true;
}

static void print_payer_id(const struct pubkey *payer_id,
			   const u8 *metadata)
{
	printf("invreq_payer_id: %s",
	       type_to_string(tmpctx, struct pubkey, payer_id));
	if (metadata)
		printf(" (invreq_metadata %s)",
		       tal_hex(tmpctx, metadata));
	printf("\n");
}

static void print_payer_note(const char *payer_note)
{
	printf("payer_note: %.*s\n",
	       (int)tal_bytelen(payer_note), payer_note);
}

static void print_created_at(u64 timestamp)
{
	printf("created_at: %"PRIu64" (%s)\n",
	       timestamp, fmt_time(tmpctx, timestamp));
}

static void print_payment_hash(const struct sha256 *payment_hash)
{
	printf("payment_hash: %s\n",
	       type_to_string(tmpctx, struct sha256, payment_hash));
}

static void print_relative_expiry(u64 *created_at, u32 *relative)
{
	/* Ignore if already malformed */
	if (!created_at)
		return;

	/* BOLT-offers #12:
	 * - if `relative_expiry` is present:
	 *   - MUST reject the invoice if the current time since 1970-01-01 UTC
	 *     is greater than `created_at` plus `seconds_from_creation`.
	 *  - otherwise:
	 *    - MUST reject the invoice if the current time since 1970-01-01 UTC
	 *      is greater than `created_at` plus 7200.
	 */
	if (!relative)
		printf("relative_expiry: %u (%s) (default)\n",
		       BOLT12_DEFAULT_REL_EXPIRY,
		       fmt_time(tmpctx, *created_at + BOLT12_DEFAULT_REL_EXPIRY));
	else
		printf("relative_expiry: %u (%s)\n", *relative,
		       fmt_time(tmpctx, *created_at + *relative));
}

static void print_fallbacks(struct fallback_address **fallbacks)
{
	for (size_t i = 0; i < tal_count(fallbacks); i++) {
		/* FIXME: format properly! */
		printf("fallback: %u %s\n",
		       fallbacks[i]->version,
		       tal_hex(tmpctx, fallbacks[i]->address));
	}
}

static bool print_extra_fields(const struct tlv_field *fields)
{
	bool ok = true;

	for (size_t i = 0; i < tal_count(fields); i++) {
		if (fields[i].meta)
			continue;
		if (fields[i].numtype % 2) {
			printf("UNKNOWN EVEN field %"PRIu64": %s\n",
			       fields[i].numtype,
			       tal_hexstr(tmpctx, fields[i].value, fields[i].length));
			ok = false;
		} else {
			printf("Unknown field %"PRIu64": %s\n",
			       fields[i].numtype,
			       tal_hexstr(tmpctx, fields[i].value, fields[i].length));
		}
	}
	return ok;
}

int main(int argc, char *argv[])
{
	const tal_t *ctx = tal(NULL, char);
	const char *method;
	char *hrp;
	u8 *data;
	char *fail;
	bool to_hex = false;

	common_setup(argv[0]);
	deprecated_apis = true;

	opt_set_alloc(opt_allocfn, tal_reallocfn, tal_freefn);
	opt_register_noarg("--help|-h", opt_usage_and_exit,
			   "decode|decodehex <bolt12>\n"
			   "encodehex <hrp> <hexstr>...",
			   "Show this message");
	opt_register_version();

	opt_early_parse(argc, argv, opt_log_stderr_exit);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	method = argv[1];
	if (!method)
		errx(ERROR_USAGE, "Need at least one argument\n%s",
		     opt_usage(argv[0], NULL));

	if (streq(method, "encodehex")) {
		char *nospaces;

		if (argc < 4)
			errx(ERROR_USAGE, "Need hrp and hexstr...\n%s",
			     opt_usage(argv[0], NULL));
		nospaces = tal_arr(ctx, char, 0);
		for (size_t i = 3; i < argc; i++) {
			const char *src;

			for (src = argv[i]; *src; src++) {
				if (cisspace(*src))
					continue;
				tal_arr_expand(&nospaces, *src);
			}
		}
		data = tal_hexdata(ctx, nospaces, tal_bytelen(nospaces));
		if (!data)
			errx(ERROR_USAGE, "Invalid hexstr\n%s",
			     opt_usage(argv[0], NULL));
		printf("%s\n", to_bech32_charset(ctx, argv[2], data));
		goto out;
	}

	if (streq(method, "decode"))
		to_hex = false;
	else if (streq(method, "decodehex"))
		to_hex = true;
	else
		errx(ERROR_USAGE,
		     "Need encodehex/decode/decodehex argument\n%s",
		     opt_usage(argv[0], NULL));

	if (!argv[2])
		errx(ERROR_USAGE, "Need argument\n%s",
		     opt_usage(argv[0], NULL));

	if (!from_bech32_charset(ctx, argv[2], strlen(argv[2]), &hrp, &data))
		errx(ERROR_USAGE, "Bad bech32 string\n%s",
		     opt_usage(argv[0], NULL));

	if (to_hex) {
		const u8 *cursor = data;
		size_t max = tal_bytelen(data);

		printf("%s %s\n", hrp, tal_hex(ctx, data));
		/* Now break down each element */
		while (max) {
			bigsize_t len;
			const u8 *s = cursor;
			fromwire_bigsize(&cursor, &max);
			if (!cursor)
				errx(ERROR_BAD_DECODE, "Bad type");
			printf("%s ", tal_hexstr(ctx, s, cursor - s));

			s = cursor;
			len = fromwire_bigsize(&cursor, &max);
			if (!cursor)
				errx(ERROR_BAD_DECODE, "Bad len");
			printf("%s ", tal_hexstr(ctx, s, cursor - s));
			s = cursor;
			fromwire(&cursor, &max, NULL, len);
			if (!cursor)
				errx(ERROR_BAD_DECODE, "Bad value");
			printf("%s\n", tal_hexstr(ctx, s, cursor - s));
		}
		goto out;
	}

	if (streq(hrp, "lno")) {
		const struct tlv_offer *offer
			= offer_decode(ctx, argv[2], strlen(argv[2]),
				       NULL, NULL, &fail);
		if (!offer)
			errx(ERROR_BAD_DECODE, "Bad offer: %s", fail);

		if (offer->offer_chains)
			print_chains(offer->offer_chains);
		if (offer->offer_amount)
			well_formed &= print_amount(offer->offer_chains,
						    offer->offer_currency,
						    *offer->offer_amount);
		if (must_have(offer, offer_description))
			print_description(offer->offer_description);
		if (offer->offer_issuer)
			print_issuer(offer->offer_issuer);
		if (must_have(offer, offer_node_id))
			print_node_id(offer->offer_node_id);
		if (offer->offer_quantity_max)
			print_quantity_max(*offer->offer_quantity_max);
		if (offer->offer_recurrence)
			well_formed &= print_recurrance(offer->offer_recurrence,
							offer->offer_recurrence_paywindow,
							offer->offer_recurrence_limit,
							offer->offer_recurrence_base);
		if (offer->offer_absolute_expiry)
			print_absolute_expiry(*offer->offer_absolute_expiry);
		if (offer->offer_features)
			print_features(offer->offer_features);
		if (offer->offer_paths)
			print_blindedpaths(offer->offer_paths, NULL);
		if (!print_extra_fields(offer->fields))
			well_formed = false;
	} else if (streq(hrp, "lnr")) {
		const struct tlv_invoice_request *invreq
			= invrequest_decode(ctx, argv[2], strlen(argv[2]),
					    NULL, NULL, &fail);
		if (!invreq)
			errx(ERROR_BAD_DECODE, "Bad invreq: %s", fail);

		if (invreq->invreq_chain)
			print_chain(invreq->invreq_chain);
		if (must_have(invreq, invreq_payer_id))
			print_payer_id(invreq->invreq_payer_id,
				       invreq->invreq_metadata);
		if (invreq->invreq_payer_note)
			print_payer_note(invreq->invreq_payer_note);
		if (must_have(invreq, invreq_amount))
			well_formed &= print_amount(invreq->invreq_chain,
						    NULL,
						    *invreq->invreq_amount);
		if (invreq->invreq_features)
			print_features(invreq->invreq_features);
		if (invreq->invreq_quantity)
			print_quantity(*invreq->invreq_quantity);
		if (must_have(invreq, signature)) {
			well_formed = print_signature("invoice_request",
						      "signature",
						      invreq->fields,
						      invreq->invreq_payer_id,
						      invreq->signature);
		}
		if (invreq->invreq_recurrence_counter) {
			print_recurrence_counter(invreq->invreq_recurrence_counter,
						 invreq->invreq_recurrence_start);
		} else {
			must_not_have(invreq, invreq_recurrence_start);
		}
		if (!print_extra_fields(invreq->fields))
			well_formed = false;
	} else if (streq(hrp, "lni")) {
		const struct tlv_invoice *invoice
			= invoice_decode(ctx, argv[2], strlen(argv[2]),
					 NULL, NULL, &fail);
		if (!invoice)
			errx(ERROR_BAD_DECODE, "Bad invoice: %s", fail);

		if (invoice->invreq_chain)
			print_chain(invoice->invreq_chain);

		if (must_have(invoice, invoice_amount))
			well_formed &= print_amount(invoice->invreq_chain,
						    NULL,
						    *invoice->invoice_amount);
		if (must_have(invoice, offer_description))
			print_description(invoice->offer_description);
		if (invoice->invoice_features)
			print_features(invoice->invoice_features);
		if (invoice->invoice_paths) {
			must_have(invoice, invoice_blindedpay);
			well_formed &= print_blindedpaths(invoice->invoice_paths,
							  invoice->invoice_blindedpay);
		} else
			must_not_have(invoice, invoice_blindedpay);
		if (invoice->offer_issuer)
			print_issuer(invoice->offer_issuer);
		if (must_have(invoice, offer_node_id))
			print_node_id(invoice->offer_node_id);
		if (invoice->invreq_quantity)
			print_quantity(*invoice->invreq_quantity);
		if (invoice->invreq_recurrence_counter) {
			well_formed &=
				print_recurrence_counter_with_base(invoice->invreq_recurrence_counter,
								   invoice->invreq_recurrence_start,
								   invoice->invoice_recurrence_basetime);
		} else {
			must_not_have(invoice, invreq_recurrence_start);
			must_not_have(invoice, invoice_recurrence_basetime);
		}
		if (must_have(invoice, invreq_payer_id))
			print_payer_id(invoice->invreq_payer_id,
					invoice->invreq_metadata);
		if (must_have(invoice, invoice_created_at))
			print_created_at(*invoice->invoice_created_at);
		if (invoice->invreq_payer_note)
			print_payer_note(invoice->invreq_payer_note);
		print_relative_expiry(invoice->invoice_created_at,
				      invoice->invoice_relative_expiry);
		if (must_have(invoice, invoice_payment_hash))
			print_payment_hash(invoice->invoice_payment_hash);
		if (invoice->invoice_fallbacks)
			print_fallbacks(invoice->invoice_fallbacks);
		if (must_have(invoice, signature))
			well_formed &= print_signature("invoice", "signature",
						       invoice->fields,
						       invoice->offer_node_id,
						       invoice->signature);
		if (!print_extra_fields(invoice->fields))
			well_formed = false;
	} else
		errx(ERROR_BAD_DECODE, "Unknown prefix %s", hrp);

out:
	tal_free(ctx);
	common_shutdown();

	if (well_formed)
		return NO_ERROR;
	else
		return ERROR_BAD_DECODE;
}
