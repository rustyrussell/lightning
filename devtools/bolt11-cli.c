#include "config.h"
#include <bitcoin/address.h>
#include <bitcoin/base58.h>
#include <bitcoin/privkey.h>
#include <bitcoin/script.h>
#include <ccan/err/err.h>
#include <ccan/opt/opt.h>
#include <ccan/str/hex/hex.h>
#include <ccan/tal/str/str.h>
#include <ccan/time/time.h>
#include <common/bech32.h>
#include <common/bolt11.h>
#include <common/features.h>
#include <common/hash_u5.h>
#include <common/setup.h>
#include <common/type_to_string.h>
#include <common/version.h>
#include <inttypes.h>
#include <stdio.h>

#define NO_ERROR 0
#define ERROR_BAD_DECODE 1
#define ERROR_USAGE 3

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

static bool bolt11_sign(const u5 *u5bytes,
			const u8 *hrpu8,
			secp256k1_ecdsa_recoverable_signature *rsig,
			struct privkey *privkey)
{
	struct hash_u5 hu5;
	struct sha256 sha;
	char *hrp;

	hrp = tal_dup_arr(tmpctx, char, (char *)hrpu8, tal_count(hrpu8), 1);
	hrp[tal_count(hrpu8)] = '\0';

	hash_u5_init(&hu5, hrp);
	hash_u5(&hu5, u5bytes, tal_count(u5bytes));
	hash_u5_done(&hu5, &sha);

	/*~ By no small coincidence, this libsecp routine uses the exact
	 * recovery signature format mandated by BOLT 11. */
	if (!secp256k1_ecdsa_sign_recoverable(secp256k1_ctx, rsig,
                                              (const u8 *)&sha,
                                              privkey->secret.data,
                                              NULL, NULL)) {
		abort();
	}
	return true;
}

int main(int argc, char *argv[])
{
	const tal_t *ctx = tal(NULL, char);
	const char *method;
	struct bolt11 *b11;
	struct bolt11_field *extra;
	char *fail, *description = NULL;

	common_setup(argv[0]);

	opt_set_alloc(opt_allocfn, tal_reallocfn, tal_freefn);
	opt_register_noarg("--help|-h", opt_usage_and_exit,
			   "decode <bolt11> OR\n"
			   "encode <secretkey> <preimage> <description> [amount]",
			   "Show this message");
	opt_register_arg("--hashed-description", opt_set_charp, opt_show_charp,
			 &description,
			 "Description to check hashed description against");
	opt_register_version();

	opt_early_parse(argc, argv, opt_log_stderr_exit);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	method = argv[1];
	if (!method)
		errx(ERROR_USAGE, "Need at least one argument\n%s",
		     opt_usage(argv[0], NULL));

	if (streq(method, "encode")) {
		struct privkey privkey;
		struct secret preimage;
		char *b11str;

		if (argc < 5)
			errx(ERROR_USAGE, "Need args\n%s",
			     opt_usage(argv[0], NULL));

		if (!hex_decode(argv[2], strlen(argv[2]), &privkey, sizeof(privkey)))
			errx(ERROR_USAGE, "Need 32 hexbyte privkey");
		if (!hex_decode(argv[3], strlen(argv[3]), &preimage,
				sizeof(preimage)))
			errx(ERROR_USAGE, "Need 32 hexbyte preimage");
		if (argv[5]) {
			struct amount_msat msat;
			if (!parse_amount_msat(&msat, argv[5], strlen(argv[5])))
				errx(ERROR_USAGE, "Need valid amount");
			b11 = new_bolt11(tmpctx, &msat);
		} else {
			b11 = new_bolt11(tmpctx, NULL);
		}
		b11->timestamp = time_now().ts.tv_sec;
		b11->description = argv[4];
		b11->chain = chainparams_for_network("regtest");
		sha256(&b11->payment_hash, &preimage, sizeof(preimage));
		b11->payment_secret = talz(b11, struct secret);
		b11str = bolt11_encode(tmpctx, b11, false, bolt11_sign, &privkey);
		assert(bolt11_decode(tmpctx, b11str, NULL, NULL, NULL, NULL));
		printf("%s\n", b11str);
		tal_free(ctx);
		common_shutdown();
		return NO_ERROR;
	}

	if (!streq(method, "decode"))
		errx(ERROR_USAGE, "Need decode argument\n%s",
		     opt_usage(argv[0], NULL));

	if (!argv[2])
		errx(ERROR_USAGE, "Need argument\n%s",
		     opt_usage(argv[0], NULL));

	b11 = bolt11_decode(ctx, argv[2], NULL, description, NULL, &fail);
	if (!b11)
		errx(ERROR_BAD_DECODE, "%s", fail);

	printf("currency: %s\n", b11->chain->lightning_hrp);
	printf("timestamp: %"PRIu64" (%s)\n",
	       b11->timestamp, fmt_time(ctx, b11->timestamp));
	printf("expiry: %"PRIu64" (%s)\n",
	       b11->expiry, fmt_time(ctx, b11->timestamp + b11->expiry));
	printf("payee: %s\n",
	       type_to_string(ctx, struct node_id, &b11->receiver_id));
	printf("payment_hash: %s\n",
	       tal_hexstr(ctx, &b11->payment_hash, sizeof(b11->payment_hash)));
	printf("min_final_cltv_expiry: %u\n", b11->min_final_cltv_expiry);
        if (b11->msat) {
		printf("msatoshi: %"PRIu64"\n", b11->msat->millisatoshis); /* Raw: raw int for backwards compat */
		printf("amount_msat: %s\n",
		       type_to_string(tmpctx, struct amount_msat, b11->msat));
	}
        if (b11->description)
                printf("description: '%s'\n", b11->description);
        if (b11->description_hash)
		printf("description_hash: %s\n",
		       tal_hexstr(ctx, b11->description_hash,
				  sizeof(*b11->description_hash)));
	if (b11->payment_secret)
		printf("payment_secret: %s\n",
		       tal_hexstr(ctx, b11->payment_secret,
				  sizeof(*b11->payment_secret)));
	if (tal_bytelen(b11->features)) {
		printf("features:");
		for (size_t i = 0; i < tal_bytelen(b11->features) * CHAR_BIT; i++) {
			if (feature_is_set(b11->features, i))
				printf(" %zu", i);
		}
		printf("\n");
	}
	for (size_t i = 0; i < tal_count(b11->fallbacks); i++) {
                struct bitcoin_address pkh;
                struct ripemd160 sh;
                struct sha256 wsh;

		printf("fallback: %s\n", tal_hex(ctx, b11->fallbacks[i]));
                if (is_p2pkh(b11->fallbacks[i], &pkh)) {
			printf("fallback-P2PKH: %s\n",
			       bitcoin_to_base58(ctx, b11->chain,
						 &pkh));
                } else if (is_p2sh(b11->fallbacks[i], &sh)) {
			printf("fallback-P2SH: %s\n",
			       p2sh_to_base58(ctx,
					      b11->chain,
					      &sh));
                } else if (is_p2wpkh(b11->fallbacks[i], &pkh)) {
                        char out[73 + strlen(b11->chain->onchain_hrp)];
                        if (segwit_addr_encode(out, b11->chain->onchain_hrp, 0,
                                               (const u8 *)&pkh, sizeof(pkh)))
				printf("fallback-P2WPKH: %s\n", out);
                } else if (is_p2wsh(b11->fallbacks[i], &wsh)) {
                        char out[73 + strlen(b11->chain->onchain_hrp)];
                        if (segwit_addr_encode(out, b11->chain->onchain_hrp, 0,
                                               (const u8 *)&wsh, sizeof(wsh)))
				printf("fallback-P2WSH: %s\n", out);
                }
        }

	for (size_t i = 0; i < tal_count(b11->routes); i++) {
		printf("route: (node/chanid/fee/expirydelta) ");
		for (size_t n = 0; n < tal_count(b11->routes[i]); n++) {
			printf(" %s/%s/%u/%u/%u",
			       type_to_string(ctx, struct node_id,
					      &b11->routes[i][n].pubkey),
			       type_to_string(ctx, struct short_channel_id,
					      &b11->routes[i][n].short_channel_id),
			       b11->routes[i][n].fee_base_msat,
			       b11->routes[i][n].fee_proportional_millionths,
			       b11->routes[i][n].cltv_expiry_delta);
		}
		printf("\n");
	}

	if (b11->metadata)
		printf("metadata: %s\n",
		       tal_hex(ctx, b11->metadata));

	list_for_each(&b11->extra_fields, extra, list) {
		char *data = tal_arr(ctx, char, tal_count(extra->data)+1);
		size_t i;

		for (i = 0; i < tal_count(extra->data); i++)
			data[i] = bech32_charset[extra->data[i]];

		data[i] = '\0';
		printf("unknown: %c %s\n", extra->tag, data);
	}

	printf("signature: %s\n",
	       type_to_string(ctx, secp256k1_ecdsa_signature, &b11->sig));
	tal_free(ctx);
	common_shutdown();
	return NO_ERROR;
}
