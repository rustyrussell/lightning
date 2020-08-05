#include "../gossip_rcvd_filter.c"
#include "../pseudorand.c"
#include "../../wire/fromwire.c"
#include <common/utils.h>
#include <stdio.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for amount_asset_extract_value */
u8 *amount_asset_extract_value(const tal_t *ctx UNNEEDED, struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_extract_value called!\n"); abort(); }
/* Generated stub for amount_asset_is_main */
bool amount_asset_is_main(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_is_main called!\n"); abort(); }
/* Generated stub for amount_asset_to_sat */
struct amount_sat amount_asset_to_sat(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_to_sat called!\n"); abort(); }
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
/* Generated stub for fromwire_amount_sat */
struct amount_sat fromwire_amount_sat(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_amount_sat called!\n"); abort(); }
/* Generated stub for memleak_remove_htable */
void memleak_remove_htable(struct htable *memtable UNNEEDED, const struct htable *ht UNNEEDED)
{ fprintf(stderr, "memleak_remove_htable called!\n"); abort(); }
/* Generated stub for towire */
void towire(u8 **pptr UNNEEDED, const void *data UNNEEDED, size_t len UNNEEDED)
{ fprintf(stderr, "towire called!\n"); abort(); }
/* Generated stub for towire_amount_sat */
void towire_amount_sat(u8 **pptr UNNEEDED, const struct amount_sat sat UNNEEDED)
{ fprintf(stderr, "towire_amount_sat called!\n"); abort(); }
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
/* AUTOGENERATED MOCKS END */

void memleak_add_helper_(const tal_t *p UNNEEDED,
			 void (*cb)(struct htable *memtable,
				    const tal_t *) UNNEEDED)
{
}

static u8 *mkgossip(const tal_t *ctx, const char *str)
{
	return tal_hexdata(ctx, str, strlen(str));
}

int main(void)
{
	const tal_t *ctx = tal(NULL, char);
	struct gossip_rcvd_filter *f = new_gossip_rcvd_filter(ctx);
	const u8 *msg[3], *badmsg;

	setup_locale();

	msg[0] = mkgossip(ctx, "0100231024fcd59aa58ca8e2ed8f71e07843fc576dd6b2872681960ce64f5f3cd3b5386211a103736bf1de2c03a74f5885d50ea30d21a82e4389339ad13149ac7f52942e6ff0778952b7cb001350d1e2edd25cee80c4c64d624a0273be5436923f5524f1f7e4586007203b2f2c47d6863052529321ebb8e0a171ed013c889bbeaa7a462e9826861c608428509804eb4dd5f75dc5e6baa03205933759fd7abcb2e0304b1a895abb7de3d24e92ade99a6a14f51ac9852ef3daf68b8ad40459d8a6124f23e1271537347b6a1bc9bff1f3e6f60e93177b3bf1d53e76771be9c974ba6b1c6d4916762c0867c13f3617e4893f6272c64fa360aaf6c2a94af7739c498bab3600006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d619000000000008a8090000510000020cf679b34b5819dfbaf68663bdd636c92117b6c04981940e878818b736c0cdb702809e936f0e82dfce13bcc47c77112db068f569e1db29e7bf98bcdd68b838ee8402590349bcd37d04b81022e52bd65d5119a43e0d7b78f526971ede38d2cd1c0d9e02eec6ac38e4acad847cd42b0946977896527b9e1f7dd59525a1a1344a3cea7fa3");
	msg[1] = mkgossip(ctx, "0102ccc0a84e4ce09f522f7765db7c30b822ebb346eb17dda92612d03cc8e53ee1454b6c9a918a60ac971e623fd056687f17a01d3c7e805723f7b68be0e8544013546fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d619000000000008a80900005100005d06bacc0102009000000000000003e8000003e8000000010000000005e69ec0");
	msg[2] = mkgossip(ctx, "01017914e70e3ef662e8d75166ea64465f8679d042bdc26b91c7de6d2c5bdd1f73654f4df7c010ea4c41538bbc5f0f6528dfd48097c7c18f3febae4dc36819550c5900005c8d5bac020cf679b34b5819dfbaf68663bdd636c92117b6c04981940e878818b736c0cdb73399ff5468655f4c656176696e677300000000000000000000000000000000000000000007018e3b64fd2607");

	/* Not a gossip msg. */
	badmsg = tal_hexdata(ctx, "00100000", strlen("00100000"));

	gossip_rcvd_filter_add(f, msg[0]);
	assert(htable_count(f->cur) == 1);
	assert(htable_count(f->old) == 0);

	gossip_rcvd_filter_add(f, msg[1]);
	assert(htable_count(f->cur) == 2);
	assert(htable_count(f->old) == 0);

	gossip_rcvd_filter_add(f, msg[2]);
	assert(htable_count(f->cur) == 3);
	assert(htable_count(f->old) == 0);

	gossip_rcvd_filter_add(f, badmsg);
	assert(htable_count(f->cur) == 3);
	assert(htable_count(f->old) == 0);

	assert(gossip_rcvd_filter_del(f, msg[0]));
	assert(htable_count(f->cur) == 2);
	assert(htable_count(f->old) == 0);
	assert(!gossip_rcvd_filter_del(f, msg[0]));
	assert(htable_count(f->cur) == 2);
	assert(htable_count(f->old) == 0);
	assert(gossip_rcvd_filter_del(f, msg[1]));
	assert(htable_count(f->cur) == 1);
	assert(htable_count(f->old) == 0);
	assert(!gossip_rcvd_filter_del(f, msg[1]));
	assert(htable_count(f->cur) == 1);
	assert(htable_count(f->old) == 0);
	assert(gossip_rcvd_filter_del(f, msg[2]));
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 0);
	assert(!gossip_rcvd_filter_del(f, msg[2]));
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 0);
	assert(!gossip_rcvd_filter_del(f, badmsg));
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 0);

	/* Re-add them, and age. */
	gossip_rcvd_filter_add(f, msg[0]);
	gossip_rcvd_filter_add(f, msg[1]);
	gossip_rcvd_filter_add(f, msg[2]);
	assert(htable_count(f->cur) == 3);
	assert(htable_count(f->old) == 0);

	gossip_rcvd_filter_age(f);
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 3);

	/* Delete 1 and 2. */
	assert(gossip_rcvd_filter_del(f, msg[2]));
	assert(gossip_rcvd_filter_del(f, msg[1]));
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 1);
	assert(!gossip_rcvd_filter_del(f, msg[2]));
	assert(!gossip_rcvd_filter_del(f, msg[1]));
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 1);
	assert(!gossip_rcvd_filter_del(f, badmsg));
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 1);

	/* Re-add 2, and age. */
	gossip_rcvd_filter_add(f, msg[2]);
	assert(htable_count(f->cur) == 1);
	assert(htable_count(f->old) == 1);

	gossip_rcvd_filter_age(f);
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 1);

	/* Now, only 2 remains. */
	assert(!gossip_rcvd_filter_del(f, msg[0]));
	assert(!gossip_rcvd_filter_del(f, msg[1]));
	assert(gossip_rcvd_filter_del(f, msg[2]));
	assert(!gossip_rcvd_filter_del(f, msg[2]));
	assert(htable_count(f->cur) == 0);
	assert(htable_count(f->old) == 0);

	/* They should have no children, and f should only have 2. */
	assert(!tal_first(f->cur));
	assert(!tal_first(f->old));

	assert((tal_first(f) == f->cur
		&& tal_next(f->cur) == f->old
		&& tal_next(f->old) == NULL)
	       || (tal_first(f) == f->old
		   && tal_next(f->old) == f->cur
		   && tal_next(f->cur) == NULL));

	tal_free(ctx);
	return 0;
}
