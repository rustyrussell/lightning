#include "config.h"
#include "../../wire/bolt12_wiregen.c"
#include "../../wire/fromwire.c"
#include "../../wire/onion_wiregen.c"
#include "../../wire/tlvstream.c"
#include "../bech32.c"
#include "../bech32_util.c"
#include "../bigsize.c"
#include "../bolt12.c"
#include "../features.c"
#include "../json_parse.c"
#include "../json_parse_simple.c"
#include "../sciddir_or_pubkey.c"
#include <ccan/array_size/array_size.h>
#include <ccan/tal/grab_file/grab_file.h>
#include <ccan/tal/path/path.h>
#include <common/setup.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for amount_asset_is_main */
bool amount_asset_is_main(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_is_main called!\n"); abort(); }
/* Generated stub for amount_asset_to_sat */
struct amount_sat amount_asset_to_sat(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_to_sat called!\n"); abort(); }
/* Generated stub for amount_feerate */
 bool amount_feerate(u32 *feerate UNNEEDED, struct amount_sat fee UNNEEDED, size_t weight UNNEEDED)
{ fprintf(stderr, "amount_feerate called!\n"); abort(); }
/* Generated stub for amount_sat */
struct amount_sat amount_sat(u64 satoshis UNNEEDED)
{ fprintf(stderr, "amount_sat called!\n"); abort(); }
/* Generated stub for amount_sat_add */
 bool amount_sat_add(struct amount_sat *val UNNEEDED,
				       struct amount_sat a UNNEEDED,
				       struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_add called!\n"); abort(); }
/* Generated stub for amount_sat_eq */
bool amount_sat_eq(struct amount_sat a UNNEEDED, struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_eq called!\n"); abort(); }
/* Generated stub for amount_sat_greater_eq */
bool amount_sat_greater_eq(struct amount_sat a UNNEEDED, struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_greater_eq called!\n"); abort(); }
/* Generated stub for amount_sat_sub */
 bool amount_sat_sub(struct amount_sat *val UNNEEDED,
				       struct amount_sat a UNNEEDED,
				       struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_sub called!\n"); abort(); }
/* Generated stub for amount_sat_to_asset */
struct amount_asset amount_sat_to_asset(struct amount_sat *sat UNNEEDED, const u8 *asset UNNEEDED)
{ fprintf(stderr, "amount_sat_to_asset called!\n"); abort(); }
/* Generated stub for amount_tx_fee */
struct amount_sat amount_tx_fee(u32 fee_per_kw UNNEEDED, size_t weight UNNEEDED)
{ fprintf(stderr, "amount_tx_fee called!\n"); abort(); }
/* Generated stub for fromwire_amount_msat */
struct amount_msat fromwire_amount_msat(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_amount_msat called!\n"); abort(); }
/* Generated stub for merkle_tlv */
void merkle_tlv(const struct tlv_field *fields UNNEEDED, struct sha256 *merkle UNNEEDED)
{ fprintf(stderr, "merkle_tlv called!\n"); abort(); }
/* Generated stub for mvt_tag_str */
const char *mvt_tag_str(enum mvt_tag tag UNNEEDED)
{ fprintf(stderr, "mvt_tag_str called!\n"); abort(); }
/* Generated stub for node_id_from_hexstr */
bool node_id_from_hexstr(const char *str UNNEEDED, size_t slen UNNEEDED, struct node_id *id UNNEEDED)
{ fprintf(stderr, "node_id_from_hexstr called!\n"); abort(); }
/* Generated stub for parse_amount_msat */
bool parse_amount_msat(struct amount_msat *msat UNNEEDED, const char *s UNNEEDED, size_t slen UNNEEDED)
{ fprintf(stderr, "parse_amount_msat called!\n"); abort(); }
/* Generated stub for parse_amount_sat */
bool parse_amount_sat(struct amount_sat *sat UNNEEDED, const char *s UNNEEDED, size_t slen UNNEEDED)
{ fprintf(stderr, "parse_amount_sat called!\n"); abort(); }
/* Generated stub for pubkey_from_node_id */
bool pubkey_from_node_id(struct pubkey *key UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "pubkey_from_node_id called!\n"); abort(); }
/* Generated stub for sighash_from_merkle */
void sighash_from_merkle(const char *messagename UNNEEDED,
			 const char *fieldname UNNEEDED,
			 const struct sha256 *merkle UNNEEDED,
			 struct sha256 *sighash UNNEEDED)
{ fprintf(stderr, "sighash_from_merkle called!\n"); abort(); }
/* Generated stub for towire */
void towire(u8 **pptr UNNEEDED, const void *data UNNEEDED, size_t len UNNEEDED)
{ fprintf(stderr, "towire called!\n"); abort(); }
/* Generated stub for towire_amount_msat */
void towire_amount_msat(u8 **pptr UNNEEDED, const struct amount_msat msat UNNEEDED)
{ fprintf(stderr, "towire_amount_msat called!\n"); abort(); }
/* Generated stub for towire_bool */
void towire_bool(u8 **pptr UNNEEDED, bool v UNNEEDED)
{ fprintf(stderr, "towire_bool called!\n"); abort(); }
/* Generated stub for towire_secp256k1_ecdsa_signature */
void towire_secp256k1_ecdsa_signature(u8 **pptr UNNEEDED,
			      const secp256k1_ecdsa_signature *signature UNNEEDED)
{ fprintf(stderr, "towire_secp256k1_ecdsa_signature called!\n"); abort(); }
/* Generated stub for towire_sha256 */
void towire_sha256(u8 **pptr UNNEEDED, const struct sha256 *sha256 UNNEEDED)
{ fprintf(stderr, "towire_sha256 called!\n"); abort(); }
/* Generated stub for towire_tu32 */
void towire_tu32(u8 **pptr UNNEEDED, u32 v UNNEEDED)
{ fprintf(stderr, "towire_tu32 called!\n"); abort(); }
/* Generated stub for towire_tu64 */
void towire_tu64(u8 **pptr UNNEEDED, u64 v UNNEEDED)
{ fprintf(stderr, "towire_tu64 called!\n"); abort(); }
/* Generated stub for towire_u16 */
void towire_u16(u8 **pptr UNNEEDED, u16 v UNNEEDED)
{ fprintf(stderr, "towire_u16 called!\n"); abort(); }
/* Generated stub for towire_u32 */
void towire_u32(u8 **pptr UNNEEDED, u32 v UNNEEDED)
{ fprintf(stderr, "towire_u32 called!\n"); abort(); }
/* Generated stub for towire_u64 */
void towire_u64(u8 **pptr UNNEEDED, u64 v UNNEEDED)
{ fprintf(stderr, "towire_u64 called!\n"); abort(); }
/* Generated stub for towire_u8 */
void towire_u8(u8 **pptr UNNEEDED, u8 v UNNEEDED)
{ fprintf(stderr, "towire_u8 called!\n"); abort(); }
/* Generated stub for towire_u8_array */
void towire_u8_array(u8 **pptr UNNEEDED, const u8 *arr UNNEEDED, size_t num UNNEEDED)
{ fprintf(stderr, "towire_u8_array called!\n"); abort(); }
/* Generated stub for towire_utf8_array */
void towire_utf8_array(u8 **pptr UNNEEDED, const char *arr UNNEEDED, size_t num UNNEEDED)
{ fprintf(stderr, "towire_utf8_array called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

int main(int argc, char *argv[])
{
	char *json;
	size_t i;
	jsmn_parser parser;
	jsmntok_t toks[5000];
	const jsmntok_t *t;
	struct feature_set *feature_set;
	common_setup(argv[0]);

	if (argv[1])
		json = grab_file(tmpctx, argv[1]);
	else {
		char *dir = getenv("BOLTDIR");
		json = grab_file(tmpctx,
				 path_join(tmpctx,
					   dir ? dir : ".tmp.lightningrfc",
					   "bolt12/offers-test.json"));
		if (!json) {
			printf("test file not found, skipping\n");
			goto out;
		}
	}

	jsmn_init(&parser);
	if (jsmn_parse(&parser, json, strlen(json), toks, ARRAY_SIZE(toks)) < 0)
		abort();

	/* This gives an empty bolt12 feature set. */
	feature_set = feature_set_for_feature(tmpctx, OPT_VAR_ONION);
	json_for_each_arr(i, t, toks) {
		bool valid;
		const char *desc, *bolt12;
		char *fail;
		struct tlv_offer *offer;

		assert(json_scan(tmpctx, json, t,
				 "{description:%,valid:%,bolt12:%}",
				 JSON_SCAN_TAL(tmpctx, json_strdup, &desc),
				 JSON_SCAN(json_to_bool, &valid),
				 JSON_SCAN_TAL(tmpctx, json_strdup, &bolt12)) == NULL);
		printf("%s: ", desc);
		offer = offer_decode(tmpctx, bolt12, strlen(bolt12),
				     feature_set, NULL, &fail);
		if (!valid) {
			if (offer) {
				printf("ERROR should be invalid!\n");
				abort();
			}
			printf("XFAIL %s\n", fail);
		} else {
			size_t j;
			const jsmntok_t *f, *fields = json_get_member(json, t, "fields");
			bool have_unknown = false;
			if (!offer) {
				printf("ERROR should be valid, but %s!\n", fail);
				abort();
			}
			if (fields->size != tal_count(offer->fields)) {
				printf("ERROR expected %u fields but decoded %zu!\n",
				       fields->size, tal_count(offer->fields));
				abort();
			}

			json_for_each_arr(j, f, fields) {
				u64 type, length;
				u8 *value;
				assert(json_scan(tmpctx, json, f,
						 "{type:%,length:%,hex:%}",
						 JSON_SCAN(json_to_u64, &type),
						 JSON_SCAN(json_to_u64, &length),
						 JSON_SCAN_TAL(tmpctx, json_tok_bin_from_hex, &value)) == NULL);
				if (offer->fields[j].numtype != type) {
					printf("ERROR expected field %"PRIu64" got %"PRIu64"\n",
					       type, offer->fields[j].numtype);
					abort();
				}
				assert(offer->fields[j].length == length);
				assert(memeq(offer->fields[j].value,
					     offer->fields[j].length,
					     value, length));
			}
			assert(tal_count(offer->fields) == j - have_unknown);
			printf("PASS\n");
		}
	}

out:
	common_shutdown();
	return 0;
}
