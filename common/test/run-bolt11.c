#include "../amount.c"
#include "../bech32.c"
#include "../bech32_util.c"
#include "../bolt11.c"
#include "../node_id.c"
#include "../hash_u5.c"
#include <ccan/err/err.h>
#include <ccan/mem/mem.h>
#include <ccan/str/hex/hex.h>
#include <stdio.h>
#include <wally_core.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_node_id */
void fromwire_node_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct node_id *id UNNEEDED)
{ fprintf(stderr, "fromwire_node_id called!\n"); abort(); }
/* Generated stub for fromwire_short_channel_id */
void fromwire_short_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			       struct short_channel_id *short_channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_short_channel_id called!\n"); abort(); }
/* Generated stub for fromwire_u16 */
u16 fromwire_u16(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_u16 called!\n"); abort(); }
/* Generated stub for fromwire_u32 */
u32 fromwire_u32(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_u32 called!\n"); abort(); }
/* Generated stub for towire_node_id */
void towire_node_id(u8 **pptr UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "towire_node_id called!\n"); abort(); }
/* Generated stub for towire_short_channel_id */
void towire_short_channel_id(u8 **pptr UNNEEDED,
			     const struct short_channel_id *short_channel_id UNNEEDED)
{ fprintf(stderr, "towire_short_channel_id called!\n"); abort(); }
/* Generated stub for towire_u16 */
void towire_u16(u8 **pptr UNNEEDED, u16 v UNNEEDED)
{ fprintf(stderr, "towire_u16 called!\n"); abort(); }
/* Generated stub for towire_u32 */
void towire_u32(u8 **pptr UNNEEDED, u32 v UNNEEDED)
{ fprintf(stderr, "towire_u32 called!\n"); abort(); }
/* Generated stub for type_to_string_ */
const char *type_to_string_(const tal_t *ctx UNNEEDED, const char *typename UNNEEDED,
			    union printable_types u UNNEEDED)
{ fprintf(stderr, "type_to_string_ called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

static struct privkey privkey;

static bool test_sign(const u5 *u5bytes,
		      const u8 *hrpu8,
		      secp256k1_ecdsa_recoverable_signature *rsig,
		      void *unused UNUSED)
{
	struct hash_u5 hu5;
	char *hrp;
	struct sha256 sha;

	hrp = tal_dup_arr(NULL, char, (char *)hrpu8, tal_count(hrpu8), 1);
	hrp[tal_count(hrpu8)] = '\0';

	hash_u5_init(&hu5, hrp);
	hash_u5(&hu5, u5bytes, tal_count(u5bytes));
	hash_u5_done(&hu5, &sha);
	tal_free(hrp);

        if (!secp256k1_ecdsa_sign_recoverable(secp256k1_ctx, rsig,
                                              (const u8 *)&sha,
                                              privkey.secret.data,
                                              NULL, NULL))
		abort();

	return true;
}

static void test_b11(const char *b11str,
		     const struct bolt11 *expect_b11,
		     const char *hashed_desc)
{
	struct bolt11 *b11;
	char *fail;
	char *reproduce;
	struct bolt11_field *b11_extra, *expect_extra;

	b11 = bolt11_decode(tmpctx, b11str, hashed_desc, &fail);
	if (!b11)
		errx(1, "%s:%u:%s", __FILE__, __LINE__, fail);

	assert(b11->chain == expect_b11->chain);
	assert(b11->timestamp == expect_b11->timestamp);
	if (!b11->msat)
		assert(!expect_b11->msat);
	else
		assert(amount_msat_eq(*b11->msat, *expect_b11->msat));
	assert(sha256_eq(&b11->payment_hash, &expect_b11->payment_hash));
	if (!b11->description)
		assert(!expect_b11->description);
	else
		assert(streq(b11->description, expect_b11->description));

	assert(b11->expiry == expect_b11->expiry);
	assert(b11->min_final_cltv_expiry == expect_b11->min_final_cltv_expiry);

	assert(tal_count(b11->fallbacks) == tal_count(expect_b11->fallbacks));
	for (size_t i = 0; i < tal_count(b11->fallbacks); i++)
		assert(memeq(b11->fallbacks[i], tal_count(b11->fallbacks[i]),
			     expect_b11->fallbacks[i],
			     tal_count(expect_b11->fallbacks[i])));

	/* FIXME: compare routes. */
	assert(tal_count(b11->routes) == tal_count(expect_b11->routes));

	expect_extra = list_top(&expect_b11->extra_fields, struct bolt11_field,
				list);
	list_for_each(&b11->extra_fields, b11_extra, list) {
		assert(expect_extra->tag == b11_extra->tag);
		assert(memeq(expect_extra->data, tal_bytelen(expect_extra->data),
			     b11_extra->data, tal_bytelen(b11_extra->data)));
		expect_extra = list_next(&expect_b11->extra_fields,
					 expect_extra, list);
	}
	assert(!expect_extra);

	/* Re-encode to check */
	reproduce = bolt11_encode(tmpctx, b11, false, test_sign, NULL);
	for (size_t i = 0; i < strlen(reproduce); i++) {
		if (reproduce[i] != b11str[i]
		    && reproduce[i] != tolower(b11str[i]))
			abort();
	}
	assert(strlen(reproduce) == strlen(b11str));
}

int main(void)
{
	setup_locale();

	struct bolt11 *b11;
	struct node_id node;
	struct amount_msat msatoshi;
	const char *badstr;
        struct bolt11_field *extra;

	wally_init(0);
	secp256k1_ctx = wally_get_secp_context();
	setup_tmpctx();

	/* BOLT #11:
	 *
	 * # Examples
	 *
	 * NB: all the following examples are signed with `priv_key`=`e126f68f7eafcc8b74f54d269fe206be715000f94dac067d1c04a8ca3b2db734`.
	 */
	if (!hex_decode("e126f68f7eafcc8b74f54d269fe206be715000f94dac067d1c04a8ca3b2db734",
			strlen("e126f68f7eafcc8b74f54d269fe206be715000f94dac067d1c04a8ca3b2db734"),
			&privkey, sizeof(privkey)))
		abort();

	/* BOLT #11:
	 *
	 * > ### Please make a donation of any amount using payment_hash 0001020304050607080900010203040506070809000102030405060708090102 to me @03e7156ae33b0a208d0744199163177e909e80176e55d97a2f221ede0f934dd9ad
	 * > lnbc1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpl2pkx2ctnv5sxxmmwwd5kgetjypeh2ursdae8g6twvus8g6rfwvs8qun0dfjkxaq8rkx3yf5tcsyz3d73gafnh3cax9rn449d9p5uxz9ezhhypd0elx87sjle52x86fux2ypatgddc6k63n7erqz25le42c4u4ecky03ylcqca784w
	 */
	if (!node_id_from_hexstr("03e7156ae33b0a208d0744199163177e909e80176e55d97a2f221ede0f934dd9ad", strlen("03e7156ae33b0a208d0744199163177e909e80176e55d97a2f221ede0f934dd9ad"), &node))
		abort();

	/* BOLT #11:
	 *
	 * Breakdown:
	 *
	 * * `lnbc`: prefix, Lightning on Bitcoin mainnet
	 * * `1`: Bech32 separator
	 * * `pvjluez`: timestamp (1496314658)
	 * * `p`: payment hash
	 *   * `p5`: `data_length` (`p` = 1, `5` = 20; 1 * 32 + 20 == 52)
	 *   * `qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypq`: payment hash 0001020304050607080900010203040506070809000102030405060708090102
	 * * `d`: short description
	 *   * `pl`: `data_length` (`p` = 1, `l` = 31; 1 * 32 + 31 == 63)
	 *   * `2pkx2ctnv5sxxmmwwd5kgetjypeh2ursdae8g6twvus8g6rfwvs8qun0dfjkxaq`: 'Please consider supporting this project'
	 * * `8rkx3yf5tcsyz3d73gafnh3cax9rn449d9p5uxz9ezhhypd0elx87sjle52x86fux2ypatgddc6k63n7erqz25le42c4u4ecky03ylcq`: signature
	 * * `ca784w`: Bech32 checksum
	 */
	b11 = new_bolt11(tmpctx, NULL);
	b11->chain = chainparams_for_network("bitcoin");
	b11->timestamp = 1496314658;
	if (!hex_decode("0001020304050607080900010203040506070809000102030405060708090102",
			strlen("0001020304050607080900010203040506070809000102030405060708090102"),
			&b11->payment_hash, sizeof(b11->payment_hash)))
		abort();
	b11->receiver_id = node;
	b11->description = "Please consider supporting this project";

	test_b11("lnbc1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpl2pkx2ctnv5sxxmmwwd5kgetjypeh2ursdae8g6twvus8g6rfwvs8qun0dfjkxaq8rkx3yf5tcsyz3d73gafnh3cax9rn449d9p5uxz9ezhhypd0elx87sjle52x86fux2ypatgddc6k63n7erqz25le42c4u4ecky03ylcqca784w", b11, NULL);

	/* BOLT #11:
	 *
	 * > ### Please send $3 for a cup of coffee to the same peer, within one minute
	 * > lnbc2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpuaztrnwngzn3kdzw5hydlzf03qdgm2hdq27cqv3agm2awhz5se903vruatfhq77w3ls4evs3ch9zw97j25emudupq63nyw24cg27h2rspfj9srp
	 *
	 * Breakdown:
	 *
	 * * `lnbc`: prefix, Lightning on Bitcoin mainnet
	 * * `2500u`: amount (2500 micro-bitcoin)
	 * * `1`: Bech32 separator
	 * * `pvjluez`: timestamp (1496314658)
	 * * `p`: payment hash...
	 * * `d`: short description
	 *   * `q5`: `data_length` (`q` = 0, `5` = 20; 0 * 32 + 20 == 20)
	 *   * `xysxxatsyp3k7enxv4js`: '1 cup coffee'
	 * * `x`: expiry time
	 *   * `qz`: `data_length` (`q` = 0, `z` = 2; 0 * 32 + 2 == 2)
	 *   * `pu`: 60 seconds (`p` = 1, `u` = 28; 1 * 32 + 28 == 60)
	 * * `aztrnwngzn3kdzw5hydlzf03qdgm2hdq27cqv3agm2awhz5se903vruatfhq77w3ls4evs3ch9zw97j25emudupq63nyw24cg27h2rsp`: signature
	 * * `fj9srp`: Bech32 checksum
	 */
	msatoshi = AMOUNT_MSAT(2500 * (1000ULL * 100000000) / 1000000);
	b11 = new_bolt11(tmpctx, &msatoshi);
	b11->chain = chainparams_for_network("bitcoin");
	b11->timestamp = 1496314658;
	if (!hex_decode("0001020304050607080900010203040506070809000102030405060708090102",
			strlen("0001020304050607080900010203040506070809000102030405060708090102"),
			&b11->payment_hash, sizeof(b11->payment_hash)))
		abort();
	b11->receiver_id = node;
	b11->description = "1 cup coffee";
	b11->expiry = 60;

	test_b11("lnbc2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpuaztrnwngzn3kdzw5hydlzf03qdgm2hdq27cqv3agm2awhz5se903vruatfhq77w3ls4evs3ch9zw97j25emudupq63nyw24cg27h2rspfj9srp", b11, NULL);

	/* BOLT #11:
	 *
	 * > ### Now send $24 for an entire list of things (hashed)
	 * > lnbc20m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqscc6gd6ql3jrc5yzme8v4ntcewwz5cnw92tz0pc8qcuufvq7khhr8wpald05e92xw006sq94mg8v2ndf4sefvf9sygkshp5zfem29trqq2yxxz7
	 *
	 * Breakdown:
	 *
	 * * `lnbc`: prefix, Lightning on Bitcoin mainnet
	 * * `20m`: amount (20 milli-bitcoin)
	 * * `1`: Bech32 separator
	 * * `pvjluez`: timestamp (1496314658)
	 * * `p`: payment hash...
	 * * `h`: tagged field: hash of description
	 *   * `p5`: `data_length` (`p` = 1, `5` = 20; 1 * 32 + 20 == 52)
	 *   * `8yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqs`: SHA256 of 'One piece of chocolate cake, one icecream cone, one pickle, one slice of swiss cheese, one slice of salami, one lollypop, one piece of cherry pie, one sausage, one cupcake, and one slice of watermelon'
	 * * `cc6gd6ql3jrc5yzme8v4ntcewwz5cnw92tz0pc8qcuufvq7khhr8wpald05e92xw006sq94mg8v2ndf4sefvf9sygkshp5zfem29trqq`: signature
	 * * `2yxxz7`: Bech32 checksum
	 */
	msatoshi = AMOUNT_MSAT(20 * (1000ULL * 100000000) / 1000);
	b11 = new_bolt11(tmpctx, &msatoshi);
	b11->chain = chainparams_for_network("bitcoin");
	b11->timestamp = 1496314658;
	if (!hex_decode("0001020304050607080900010203040506070809000102030405060708090102",
			strlen("0001020304050607080900010203040506070809000102030405060708090102"),
			&b11->payment_hash, sizeof(b11->payment_hash)))
		abort();
	b11->receiver_id = node;
	b11->description_hash = tal(b11, struct sha256);
	test_b11("lnbc20m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqscc6gd6ql3jrc5yzme8v4ntcewwz5cnw92tz0pc8qcuufvq7khhr8wpald05e92xw006sq94mg8v2ndf4sefvf9sygkshp5zfem29trqq2yxxz7", b11, "One piece of chocolate cake, one icecream cone, one pickle, one slice of swiss cheese, one slice of salami, one lollypop, one piece of cherry pie, one sausage, one cupcake, and one slice of watermelon");

	/* Malformed bolt11 strings (no '1'). */
	badstr = "lnbc20mpvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqscc6gd6ql3jrc5yzme8v4ntcewwz5cnw92tz0pc8qcuufvq7khhr8wpald05e92xw006sq94mg8v2ndf4sefvf9sygkshp5zfem29trqq2yxxz7";

	for (size_t i = 0; i <= strlen(badstr); i++) {
		char *fail;
		if (bolt11_decode(tmpctx, tal_strndup(tmpctx, badstr, i),
				  NULL, &fail))
			abort();
		assert(strstr(fail, "Bad bech32")
		       || strstr(fail, "Invoices must start with ln"));
	}

	/* ALL UPPERCASE is allowed (useful for QR codes) */
	msatoshi = AMOUNT_MSAT(2500 * (1000ULL * 100000000) / 1000000);
	b11 = new_bolt11(tmpctx, &msatoshi);
	b11->chain = chainparams_for_network("bitcoin");
	b11->timestamp = 1496314658;
	if (!hex_decode("0001020304050607080900010203040506070809000102030405060708090102",
			strlen("0001020304050607080900010203040506070809000102030405060708090102"),
			&b11->payment_hash, sizeof(b11->payment_hash)))
		abort();
	b11->receiver_id = node;
	b11->description = "1 cup coffee";
	b11->expiry = 60;

	test_b11("LNBC2500U1PVJLUEZPP5QQQSYQCYQ5RQWZQFQQQSYQCYQ5RQWZQFQQQSYQCYQ5RQWZQFQYPQDQ5XYSXXATSYP3K7ENXV4JSXQZPUAZTRNWNGZN3KDZW5HYDLZF03QDGM2HDQ27CQV3AGM2AWHZ5SE903VRUATFHQ77W3LS4EVS3CH9ZW97J25EMUDUPQ63NYW24CG27H2RSPFJ9SRP", b11, NULL);

	/* Unknown field handling */
	if (!node_id_from_hexstr("02330d13587b67a85c0a36ea001c4dba14bcd48dda8988f7303275b040bffb6abd", strlen("02330d13587b67a85c0a36ea001c4dba14bcd48dda8988f7303275b040bffb6abd"), &node))
		abort();
	msatoshi = AMOUNT_MSAT(3000000000);
	b11 = new_bolt11(tmpctx, &msatoshi);
	b11->chain = chainparams_for_network("testnet");
	b11->timestamp = 1554294928;
	if (!hex_decode("850aeaf5f69670e8889936fc2e0cff3ceb0c3b5eab8f04ae57767118db673a91",
			strlen("850aeaf5f69670e8889936fc2e0cff3ceb0c3b5eab8f04ae57767118db673a91"),
			&b11->payment_hash, sizeof(b11->payment_hash)))
		abort();
	b11->min_final_cltv_expiry = 9;
	b11->receiver_id = node;
	b11->description = "Payment request with multipart support";
	b11->expiry = 28800;
        extra = tal(b11, struct bolt11_field);
	extra->tag = 'v';
	extra->data = tal_arr(extra, u5, 77);
	for (size_t i = 0; i < 77; i++)
		extra->data[i] = bech32_charset_rev[(u8)"dp68gup69uhnzwfj9cejuvf3xshrwde68qcrswf0d46kcarfwpshyaplw3skw0tdw4k8g6tsv9e8g"[i]];
	list_add(&b11->extra_fields, &extra->list);

	test_b11("lntb30m1pw2f2yspp5s59w4a0kjecw3zyexm7zur8l8n4scw674w8sftjhwec33km882gsdpa2pshjmt9de6zqun9w96k2um5ypmkjargypkh2mr5d9cxzun5ypeh2ursdae8gxqruyqvzddp68gup69uhnzwfj9cejuvf3xshrwde68qcrswf0d46kcarfwpshyaplw3skw0tdw4k8g6tsv9e8g4a3hx0v945csrmpm7yxyaamgt2xu7mu4xyt3vp7045n4k4czxf9kj0vw0m8dr5t3pjxuek04rtgyy8uzss5eet5gcyekd6m7u0mzv5sp7mdsag", b11, NULL);

	/* FIXME: Test the others! */
	wally_cleanup(0);
	tal_free(tmpctx);
	return 0;
}
