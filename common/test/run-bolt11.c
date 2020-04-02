#include "../amount.c"
#include "../bech32.c"
#include "../bech32_util.c"
#include "../bolt11.c"
#include "../features.c"
#include "../node_id.c"
#include "../hash_u5.c"
#include "../memleak.c"
#include "../wire/fromwire.c"
#include "../wire/towire.c"
#include <ccan/err/err.h>
#include <ccan/mem/mem.h>
#include <ccan/str/hex/hex.h>
#include <stdio.h>
#include <wally_core.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for bigsize_get */
size_t bigsize_get(const u8 *p UNNEEDED, size_t max UNNEEDED, bigsize_t *val UNNEEDED)
{ fprintf(stderr, "bigsize_get called!\n"); abort(); }
/* Generated stub for bigsize_put */
size_t bigsize_put(u8 buf[BIGSIZE_MAX_LEN] UNNEEDED, bigsize_t v UNNEEDED)
{ fprintf(stderr, "bigsize_put called!\n"); abort(); }
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

	b11 = bolt11_decode(tmpctx, b11str, NULL, hashed_desc, &fail);
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

	if (!b11->payment_secret)
		assert(!expect_b11->payment_secret);
	else
		assert(memeq(b11->payment_secret, sizeof(*b11->payment_secret),
			     expect_b11->payment_secret,
			     sizeof(*expect_b11->payment_secret)));
	assert(memeq(b11->features, tal_bytelen(b11->features),
		     expect_b11->features, tal_bytelen(expect_b11->features)));
	assert(b11->expiry == expect_b11->expiry);
	assert(b11->min_final_cltv_expiry == expect_b11->min_final_cltv_expiry);

	assert(tal_count(b11->fallbacks) == tal_count(expect_b11->fallbacks));
	for (size_t i = 0; i < tal_count(b11->fallbacks); i++)
		assert(memeq(b11->fallbacks[i], tal_count(b11->fallbacks[i]),
			     expect_b11->fallbacks[i],
			     tal_count(expect_b11->fallbacks[i])));

	assert(tal_count(b11->routes) == tal_count(expect_b11->routes));
	for (size_t i = 0; i < tal_count(b11->routes); i++) {
		assert(tal_count(b11->routes[i])
		       == tal_count(expect_b11->routes[i]));
		for (size_t j = 0; j < tal_count(b11->routes[i]); j++) {
			const struct route_info *r = &b11->routes[i][j],
				*er = &expect_b11->routes[i][j];
			assert(node_id_eq(&er->pubkey, &r->pubkey));
			assert(er->cltv_expiry_delta == r->cltv_expiry_delta);
			assert(short_channel_id_eq(&er->short_channel_id,
						   &r->short_channel_id));
			assert(er->fee_base_msat == r->fee_base_msat);
			assert(er->fee_proportional_millionths
			       == r->fee_proportional_millionths);
		}
	}

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
	char *fail;
	struct feature_set *fset;

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
		if (bolt11_decode(tmpctx, tal_strndup(tmpctx, badstr, i),
				  NULL, NULL, &fail))
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

	/* BOLT #11:
	 *
	 * > ### Please send $30 for coffee beans to the same peer, which supports features 9, 15 and 99, using secret 0x1111111111111111111111111111111111111111111111111111111111111111
	 * > lnbc25m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5vdhkven9v5sxyetpdeessp5zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygs9q5sqqqqqqqqqqqqqqqpqsq67gye39hfg3zd8rgc80k32tvy9xk2xunwm5lzexnvpx6fd77en8qaq424dxgt56cag2dpt359k3ssyhetktkpqh24jqnjyw6uqd08sgptq44qu
	 *
	 * Breakdown:
	 *
	 * * `lnbc`: prefix, Lightning on Bitcoin mainnet
	 * * `25m`: amount (25 milli-bitcoin)
	 * * `1`: Bech32 separator
	 * * `pvjluez`: timestamp (1496314658)
	 * * `p`: payment hash...
	 * * `d`: short description
	 *   * `q5`: `data_length` (`q` = 0, `5` = 20; 0 * 32 + 20 == 20)
	 *   * `vdhkven9v5sxyetpdees`: 'coffee beans'
	 * * `s`: payment secret
	 *   * `p5`: `data_length` (`p` = 1, `5` = 20; 1 * 32 + 20 == 52)
	 *   * `zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygs`: 0x1111111111111111111111111111111111111111111111111111111111111111
	 * * `9`: features
	 *  * `q5`: `data_length` (`q` = 0, `5` = 20; 0 * 32 + 20 == 20)
	 *  * `sqqqqqqqqqqqqqqqpqsq`: b1000....00001000001000000000
	 ** `67gye39hfg3zd8rgc8032tvy9xk2xunwm5lzexnvpx6fd77en8qaq424dxgt56cag2dpt359k3ssyhetktkpqh24jqnjyw6uqd08sgp`: signature
	 ** `tq44qu`: Bech32 checksum
	 */
	msatoshi = AMOUNT_MSAT(25 * (1000ULL * 100000000) / 1000);
	b11 = new_bolt11(tmpctx, &msatoshi);
	b11->chain = chainparams_for_network("bitcoin");
	b11->timestamp = 1496314658;
	if (!hex_decode("0001020304050607080900010203040506070809000102030405060708090102",
			strlen("0001020304050607080900010203040506070809000102030405060708090102"),
			&b11->payment_hash, sizeof(b11->payment_hash)))
		abort();
	b11->receiver_id = node;
	b11->description = "coffee beans";
	b11->payment_secret = tal(b11, struct secret);
	memset(b11->payment_secret, 0x11, sizeof(*b11->payment_secret));
	set_feature_bit(&b11->features, 9);
	set_feature_bit(&b11->features, 15);
	set_feature_bit(&b11->features, 99);

	test_b11("lnbc25m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5vdhkven9v5sxyetpdeessp5zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygs9q5sqqqqqqqqqqqqqqqpqsq67gye39hfg3zd8rgc80k32tvy9xk2xunwm5lzexnvpx6fd77en8qaq424dxgt56cag2dpt359k3ssyhetktkpqh24jqnjyw6uqd08sgptq44qu", b11, NULL);

	/* BOLT-4e228a7fb4ea78af914d1ce82a63cbce8026279e #11
	 *
	 * > ### Same, but including fields which must be ignored.
	 * > lnbc25m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5vdhkven9v5sxyetpdeessp5zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygs9q5sqqqqqqqqqqqqqqqpqsq2qrqqqfppnqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqppnqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpp4qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqhpnqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqhp4qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqspnqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqsp4qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqnp5qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqnpkqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq2jxxfsnucm4jf4zwtznpaxphce606fvhvje5x7d4gw7n73994hgs7nteqvenq8a4ml8aqtchv5d9pf7l558889hp4yyrqv6a7zpq9fgpskqhza
	 *
	 * Breakdown:
	 *
	 * * `lnbc`: prefix, Lightning on Bitcoin mainnet
	 * * `25m`: amount (25 milli-bitcoin)
	 * * `1`: Bech32 separator
	 * * `pvjluez`: timestamp (1496314658)
	 * * `p`: payment hash...
	 * * `d`: short description
	 *   * `q5`: `data_length` (`q` = 0, `5` = 20; 0 * 32 + 20 == 20)
	 *   * `vdhkven9v5sxyetpdees`: 'coffee beans'
	 * * `s`: payment secret
	 *   * `p5`: `data_length` (`p` = 1, `5` = 20; 1 * 32 + 20 == 52)
	 *   * `zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygs`: 0x1111111111111111111111111111111111111111111111111111111111111111
	 * * `9`: features
	 *   * `q5`: `data_length` (`q` = 0, `5` = 20; 0 * 32 + 20 == 20)
	 *   * `sqqqqqqqqqqqqqqqpqsq`: b1000....00001000001000000000
	 * * `2`: unknown field
	 *   * `qr`: `data_length` (`q` = 0, `r` = 3; 0 * 32 + 3 == 3)
	 *   * `qqq`: zeroes
	 * * `f`: tagged field: fallback address
	 *   * `pp`: `data_length` (`p` = 1, `p` = 1; 1 * 32 + 1 == 33)
	 *   * `nqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`: fallback address type 19 (ignored)
	 * * `p`: payment hash
	 *   * `pn`: `data_length` (`p` = 1, `n` = 19; 1 * 32 + 19 == 51) (ignored)
	 *   * `qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`
	 * * `p`: payment hash
	 *   * `p4`: `data_length` (`p` = 1, `4` = 21; 1 * 32 + 21 == 53) (ignored)
	 *   * `qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`
	 * * `h`: hash of description
	 *   * `pn`: `data_length` (`p` = 1, `n` = 19; 1 * 32 + 19 == 51) (ignored)
	 *   * `qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`
	 * * `h`: hash of description
	 *   * `p4`: `data_length` (`p` = 1, `4` = 21; 1 * 32 + 21 == 53) (ignored)
	 *   * `qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`
	 * * `s`: payment secret
	 *   * `pn`: `data_length` (`p` = 1, `n` = 19; 1 * 32 + 19 == 51) (ignored)
	 *   * `qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`
	 * * `s`: payment secret
	 *   * `p4`: `data_length` (`p` = 1, `4` = 21; 1 * 32 + 21 == 53) (ignored)
	 *   * `qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`
	 * * `n`: node id
	 *   * `p5`: `data_length` (`p` = 1, `5` = 20; 1 * 32 + 20 == 52) (ignored)
	 *   * `qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`
	 * * `n`: node id
	 *   * `pk`: `data_length` (`p` = 1, `k` = 22; 1 * 32 + 22 == 54) (ignored)
	 *   * `qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq`
	 * * `2jxxfsnucm4jf4zwtznpaxphce606fvhvje5x7d4gw7n73994hgs7nteqvenq8a4ml8aqtchv5d9pf7l558889hp4yyrqv6a7zpq9fgp`: signature
	 * * `skqhza`: Bech32 checksum
	 */
	extra = tal_arr(b11, struct bolt11_field, 10);
	/* Unknown field */
	extra[0].tag = '2';
	extra[0].data = tal_arrz(extra, u8, 3);
	list_add_tail(&b11->extra_fields, &extra[0].list);
	/* f with unknown version */
	extra[1].tag = 'f';
	extra[1].data = tal_arrz(extra, u8, 33);
	list_add_tail(&b11->extra_fields, &extra[1].list);
	extra[1].data[0] = 19;
	/* p field too short & long */
	extra[2].tag = 'p';
	extra[2].data = tal_arrz(extra, u8, 51);
	list_add_tail(&b11->extra_fields, &extra[2].list);
	extra[3].tag = 'p';
	extra[3].data = tal_arrz(extra, u8, 53);
	list_add_tail(&b11->extra_fields, &extra[3].list);
	/* h field too short & long */
	extra[4].tag = 'h';
	extra[4].data = tal_arrz(extra, u8, 51);
	list_add_tail(&b11->extra_fields, &extra[4].list);
	extra[5].tag = 'h';
	extra[5].data = tal_arrz(extra, u8, 53);
	list_add_tail(&b11->extra_fields, &extra[5].list);
	/* s field too short & long */
	extra[6].tag = 's';
	extra[6].data = tal_arrz(extra, u8, 51);
	list_add_tail(&b11->extra_fields, &extra[6].list);
	extra[7].tag = 's';
	extra[7].data = tal_arrz(extra, u8, 53);
	list_add_tail(&b11->extra_fields, &extra[7].list);
	/* n field too short & long */
	extra[8].tag = 'n';
	extra[8].data = tal_arrz(extra, u8, 52);
	list_add_tail(&b11->extra_fields, &extra[8].list);
	extra[9].tag = 'n';
	extra[9].data = tal_arrz(extra, u8, 54);
	list_add_tail(&b11->extra_fields, &extra[9].list);

	test_b11("lnbc25m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5vdhkven9v5sxyetpdeessp5zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygs9q5sqqqqqqqqqqqqqqqpqsq2qrqqqfppnqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqppnqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpp4qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqhpnqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqhp4qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqspnqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqsp4qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqnp5qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqnpkqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq2jxxfsnucm4jf4zwtznpaxphce606fvhvje5x7d4gw7n73994hgs7nteqvenq8a4ml8aqtchv5d9pf7l558889hp4yyrqv6a7zpq9fgpskqhza", b11, NULL);

	/* BOLT #11:
	 *
	 * > # Same, but adding invalid unknown feature 100
	 * > lnbc25m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5vdhkven9v5sxyetpdeessp5zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygs9q4psqqqqqqqqqqqqqqqpqsqq40wa3khl49yue3zsgm26jrepqr2eghqlx86rttutve3ugd05em86nsefzh4pfurpd9ek9w2vp95zxqnfe2u7ckudyahsa52q66tgzcp6t2dyk
	 */
	/* Clear extra fields from previous */
	list_head_init(&b11->extra_fields);
	/* This one can be encoded, but not decoded */
	set_feature_bit(&b11->features, 100);
	badstr = bolt11_encode(tmpctx, b11, false, test_sign, NULL);
	assert(streq(badstr, "lnbc25m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5vdhkven9v5sxyetpdeessp5zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygs9q4psqqqqqqqqqqqqqqqpqsqq40wa3khl49yue3zsgm26jrepqr2eghqlx86rttutve3ugd05em86nsefzh4pfurpd9ek9w2vp95zxqnfe2u7ckudyahsa52q66tgzcp6t2dyk"));
	/* Empty set of allowed bits, ensures this fails! */
	fset = tal(tmpctx, struct feature_set);
	fset->bits[BOLT11_FEATURE] = tal_arr(fset, u8, 0);
	assert(!bolt11_decode(tmpctx, badstr, fset, NULL, &fail));
	assert(streq(fail, "9: unknown feature bit 100"));

	/* We'd actually allow this if we either (1) don't check, or (2) accept that feature in
	 * either compulsory or optional forms. */
	assert(bolt11_decode(tmpctx, badstr, NULL, NULL, &fail));

	set_feature_bit(&fset->bits[BOLT11_FEATURE], 100);
	assert(bolt11_decode(tmpctx, badstr, fset, NULL,&fail));

	clear_feature_bit(fset->bits[BOLT11_FEATURE], 100);
	set_feature_bit(&fset->bits[BOLT11_FEATURE], 101);
	assert(bolt11_decode(tmpctx, badstr, fset, NULL, &fail));

	/* BOLT-1fbccd30bb503203e4a255de67f9adb504563425 #11:
	 *
	 * > ### Please send 0.00967878534 BTC for a list of items within one week, amount in pico-BTC
	 * > lnbc9678785340p1pwmna7lpp5gc3xfm08u9qy06djf8dfflhugl6p7lgza6dsjxq454gxhj9t7a0sd8dgfkx7cmtwd68yetpd5s9xar0wfjn5gpc8qhrsdfq24f5ggrxdaezqsnvda3kkum5wfjkzmfqf3jkgem9wgsyuctwdus9xgrcyqcjcgpzgfskx6eqf9hzqnteypzxz7fzypfhg6trddjhygrcyqezcgpzfysywmm5ypxxjemgw3hxjmn8yptk7untd9hxwg3q2d6xjcmtv4ezq7pqxgsxzmnyyqcjqmt0wfjjq6t5v4khxxqyjw5qcqp2rzjq0gxwkzc8w6323m55m4jyxcjwmy7stt9hwkwe2qxmy8zpsgg7jcuwz87fcqqeuqqqyqqqqlgqqqqn3qq9qn07ytgrxxzad9hc4xt3mawjjt8znfv8xzscs7007v9gh9j569lencxa8xeujzkxs0uamak9aln6ez02uunw6rd2ht2sqe4hz8thcdagpleym0j
	 *
	 * Breakdown:
	 *
	 * * `lnbc`: prefix, Lightning on bitcoin mainnet
	 * * `9678785340p`: amount (9678785340 pico-bitcoin = 967878534 milli satoshi)
	 * * `1`: Bech32 separator
	 * * `pwmna7l`: timestamp (1572468703)
	 * * `p`: payment hash.
	 *   * `p5`: `data_length` (`p` = 1, `5` = 20; 1 * 32 + 20 == 52)
	 *   * `gc3xfm08u9qy06djf8dfflhugl6p7lgza6dsjxq454gxhj9t7a0s`: payment hash 462264ede7e14047e9b249da94fefc47f41f7d02ee9b091815a5506bc8abf75f
	 * * `d`: short description
	 *   * `8d`: `data_length` (`8` = 7, `d` = 13; 7 * 32 + 13 == 237)
	 *   * `gfkx7cmtwd68yetpd5s9xar0wfjn5gpc8qhrsdfq24f5ggrxdaezqsnvda3kkum5wfjkzmfqf3jkgem9wgsyuctwdus9xgrcyqcjcgpzgfskx6eqf9hzqnteypzxz7fzypfhg6trddjhygrcyqezcgpzfysywmm5ypxxjemgw3hxjmn8yptk7untd9hxwg3q2d6xjcmtv4ezq7pqxgsxzmnyyqcjqmt0wfjjq6t5v4khx`: 'Blockstream Store: 88.85 USD for Blockstream Ledger Nano S x 1, \"Back In My Day\" Sticker x 2, \"I Got Lightning Working\" Sticker x 2 and 1 more items'
	 * * `x`: expiry time
	 *   * `qy`: `data_length` (`q` = 0, `y` = 2; 0 * 32 + 4 == 4)
	 *   * `jw5q`: 604800 seconds (`j` = 18, `w` = 14, `5` = 20, `q` = 0; 18 * 32^3 + 14 * 32^2 + 20 * 32 + 0 == 604800)
	 * * `c`: `min_final_cltv_expiry`
	 *   * `qp`: `data_length` (`q` = 0, `p` = 1; 0 * 32 + 1 == 1)
	 *   * `2`: min_final_cltv_expiry = 10
	 * * `r`: tagged field: route information
	 *   * `zj`: `data_length` (`z` = 2, `j` = 18; 2 * 32 + 18 == 82)
	 *   * `q0gxwkzc8w6323m55m4jyxcjwmy7stt9hwkwe2qxmy8zpsgg7jcuwz87fcqqeuqqqyqqqqlgqqqqn3qq9q`:
	 *     * pubkey: 03d06758583bb5154774a6eb221b1276c9e82d65bbaceca806d90e20c108f4b1c7
	 *     * short_channel_id: 589390x3312x1
	 *     * fee_base_msat = 1000
	 *     * fee_proportional_millionths = 2500
	 *     * cltv_expiry_delta = 40
	 * * `n07ytgrxxzad9hc4xt3mawjjt8znfv8xzscs7007v9gh9j569lencxa8xeujzkxs0uamak9aln6ez02uunw6rd2ht2sqe4hz8thcdagp`: signature
	 * * `leym0j`: Bech32 checksum
	 */
	msatoshi = AMOUNT_MSAT(967878534);
	b11 = new_bolt11(tmpctx, &msatoshi);
	b11->chain = chainparams_for_network("bitcoin");
	b11->timestamp = 1572468703;
	b11->expiry = 604800;
	b11->min_final_cltv_expiry = 10;
	if (!hex_decode("462264ede7e14047e9b249da94fefc47f41f7d02ee9b091815a5506bc8abf75f",
			strlen("462264ede7e14047e9b249da94fefc47f41f7d02ee9b091815a5506bc8abf75f"),
			&b11->payment_hash, sizeof(b11->payment_hash)))
		abort();
	b11->receiver_id = node;
	b11->description = "Blockstream Store: 88.85 USD for Blockstream Ledger Nano S x 1, \"Back In My Day\" Sticker x 2, \"I Got Lightning Working\" Sticker x 2 and 1 more items";
	b11->routes = tal_arr(tmpctx, struct route_info *, 1);
	b11->routes[0] = tal(b11->routes, struct route_info);
	if (!node_id_from_hexstr("03d06758583bb5154774a6eb221b1276c9e82d65bbaceca806d90e20c108f4b1c7", strlen("03d06758583bb5154774a6eb221b1276c9e82d65bbaceca806d90e20c108f4b1c7"), &b11->routes[0]->pubkey))
		abort();
	if (!short_channel_id_from_str("589390x3312x1", strlen("589390x3312x1"),
				       &b11->routes[0]->short_channel_id))
		abort();
	b11->routes[0]->fee_base_msat = 1000;
	b11->routes[0]->fee_proportional_millionths = 2500;
	b11->routes[0]->cltv_expiry_delta = 40;
	test_b11("lnbc9678785340p1pwmna7lpp5gc3xfm08u9qy06djf8dfflhugl6p7lgza6dsjxq454gxhj9t7a0sd8dgfkx7cmtwd68yetpd5s9xar0wfjn5gpc8qhrsdfq24f5ggrxdaezqsnvda3kkum5wfjkzmfqf3jkgem9wgsyuctwdus9xgrcyqcjcgpzgfskx6eqf9hzqnteypzxz7fzypfhg6trddjhygrcyqezcgpzfysywmm5ypxxjemgw3hxjmn8yptk7untd9hxwg3q2d6xjcmtv4ezq7pqxgsxzmnyyqcjqmt0wfjjq6t5v4khxxqyjw5qcqp2rzjq0gxwkzc8w6323m55m4jyxcjwmy7stt9hwkwe2qxmy8zpsgg7jcuwz87fcqqeuqqqyqqqqlgqqqqn3qq9qn07ytgrxxzad9hc4xt3mawjjt8znfv8xzscs7007v9gh9j569lencxa8xeujzkxs0uamak9aln6ez02uunw6rd2ht2sqe4hz8thcdagpleym0j", b11, NULL);

	/* BOLT-4e228a7fb4ea78af914d1ce82a63cbce8026279e #11:
	 *
	 * > ### Bech32 checksum is invalid.
	 * > lnbc2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpquwpc4curk03c9wlrswe78q4eyqc7d8d0xqzpuyk0sg5g70me25alkluzd2x62aysf2pyy8edtjeevuv4p2d5p76r4zkmneet7uvyakky2zr4cusd45tftc9c5fh0nnqpnl2jfll544esqchsrnt
	 */
	assert(!bolt11_decode(tmpctx, "lnbc2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpquwpc4curk03c9wlrswe78q4eyqc7d8d0xqzpuyk0sg5g70me25alkluzd2x62aysf2pyy8edtjeevuv4p2d5p76r4zkmneet7uvyakky2zr4cusd45tftc9c5fh0nnqpnl2jfll544esqchsrnt", NULL, NULL, &fail));
	assert(streq(fail, "Bad bech32 string"));

	/* BOLT-4e228a7fb4ea78af914d1ce82a63cbce8026279e #11:
	 * > ### Malformed bech32 string (no 1)
	 * > pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpquwpc4curk03c9wlrswe78q4eyqc7d8d0xqzpuyk0sg5g70me25alkluzd2x62aysf2pyy8edtjeevuv4p2d5p76r4zkmneet7uvyakky2zr4cusd45tftc9c5fh0nnqpnl2jfll544esqchsrny
	 */
	assert(!bolt11_decode(tmpctx, "pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpquwpc4curk03c9wlrswe78q4eyqc7d8d0xqzpuyk0sg5g70me25alkluzd2x62aysf2pyy8edtjeevuv4p2d5p76r4zkmneet7uvyakky2zr4cusd45tftc9c5fh0nnqpnl2jfll544esqchsrny", NULL, NULL, &fail));
	assert(streq(fail, "Bad bech32 string"));

	/* BOLT-4e228a7fb4ea78af914d1ce82a63cbce8026279e #11:
	 * > ### Malformed bech32 string (mixed case)
	 * > LNBC2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpquwpc4curk03c9wlrswe78q4eyqc7d8d0xqzpuyk0sg5g70me25alkluzd2x62aysf2pyy8edtjeevuv4p2d5p76r4zkmneet7uvyakky2zr4cusd45tftc9c5fh0nnqpnl2jfll544esqchsrny
	 */
	assert(!bolt11_decode(tmpctx, "LNBC2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpquwpc4curk03c9wlrswe78q4eyqc7d8d0xqzpuyk0sg5g70me25alkluzd2x62aysf2pyy8edtjeevuv4p2d5p76r4zkmneet7uvyakky2zr4cusd45tftc9c5fh0nnqpnl2jfll544esqchsrny", NULL, NULL, &fail));
	assert(streq(fail, "Bad bech32 string"));

	/* BOLT-4e228a7fb4ea78af914d1ce82a63cbce8026279e #11:
	 * > ### Signature is not recoverable.
	 * > lnbc2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpuaxtrnwngzn3kdzw5hydlzf03qdgm2hdq27cqv3agm2awhz5se903vruatfhq77w3ls4evs3ch9zw97j25emudupq63nyw24cg27h2rspk28uwq
	 */
	assert(!bolt11_decode(tmpctx, "lnbc2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpuaxtrnwngzn3kdzw5hydlzf03qdgm2hdq27cqv3agm2awhz5se903vruatfhq77w3ls4evs3ch9zw97j25emudupq63nyw24cg27h2rspk28uwq", NULL, NULL, &fail));
	assert(streq(fail, "signature recovery failed"));

	/* BOLT-4e228a7fb4ea78af914d1ce82a63cbce8026279e #11:
	 * > ### String is too short.
	 * > lnbc1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpl2pkx2ctnv5sxxmmwwd5kgetjypeh2ursdae8g6na6hlh
	 */
	assert(!bolt11_decode(tmpctx, "lnbc1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpl2pkx2ctnv5sxxmmwwd5kgetjypeh2ursdae8g6na6hlh", NULL, NULL, &fail));

	/* BOLT-4e228a7fb4ea78af914d1ce82a63cbce8026279e #11:
	 * > ### Invalid multiplier
	 * > lnbc2500x1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpujr6jxr9gq9pv6g46y7d20jfkegkg4gljz2ea2a3m9lmvvr95tq2s0kvu70u3axgelz3kyvtp2ywwt0y8hkx2869zq5dll9nelr83zzqqpgl2zg
	 */
	assert(!bolt11_decode(tmpctx, "lnbc2500x1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpujr6jxr9gq9pv6g46y7d20jfkegkg4gljz2ea2a3m9lmvvr95tq2s0kvu70u3axgelz3kyvtp2ywwt0y8hkx2869zq5dll9nelr83zzqqpgl2zg", NULL, NULL, &fail));
	assert(streq(fail, "Invalid amount postfix 'x'"));

	/* BOLT- #11:
	 * > ### Invalid sub-millisatoshi precision.
	 * > lnbc2500000001p1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpu7hqtk93pkf7sw55rdv4k9z2vj050rxdr6za9ekfs3nlt5lr89jqpdmxsmlj9urqumg0h9wzpqecw7th56tdms40p2ny9q4ddvjsedzcplva53s
	 */
	assert(!bolt11_decode(tmpctx, "lnbc2500000001p1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpu7hqtk93pkf7sw55rdv4k9z2vj050rxdr6za9ekfs3nlt5lr89jqpdmxsmlj9urqumg0h9wzpqecw7th56tdms40p2ny9q4ddvjsedzcplva53s", NULL, NULL, &fail));
	assert(streq(fail, "Invalid sub-millisatoshi amount '2500000001p'"));

	/* FIXME: Test the others! */
	wally_cleanup(0);
	tal_free(tmpctx);
	return 0;
}
