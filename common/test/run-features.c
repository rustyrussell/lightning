#include "../features.c"
#include <ccan/err/err.h>
#include <ccan/mem/mem.h>
#include <ccan/str/hex/hex.h>
#include <common/utils.h>
#include <stdio.h>
#include <wally_core.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for amount_asset_is_main */
bool amount_asset_is_main(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_is_main called!\n"); abort(); }
/* Generated stub for amount_asset_to_sat */
struct amount_sat amount_asset_to_sat(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_to_sat called!\n"); abort(); }
/* Generated stub for amount_sat_add */
 bool amount_sat_add(struct amount_sat *val UNNEEDED,
				       struct amount_sat a UNNEEDED,
				       struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_add called!\n"); abort(); }
/* Generated stub for amount_sat_eq */
bool amount_sat_eq(struct amount_sat a UNNEEDED, struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_eq called!\n"); abort(); }
/* Generated stub for amount_sat_sub */
 bool amount_sat_sub(struct amount_sat *val UNNEEDED,
				       struct amount_sat a UNNEEDED,
				       struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_sub called!\n"); abort(); }
/* Generated stub for fromwire_fail */
const void *fromwire_fail(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_fail called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

int main(void)
{
	u8 *bits, *lf;

	setup_locale();
	wally_init(0);
	secp256k1_ctx = wally_get_secp_context();
	setup_tmpctx();

	bits = tal_arr(tmpctx, u8, 0);
	for (size_t i = 0; i < 100; i += 3)
		set_feature_bit(&bits, i);
	for (size_t i = 0; i < 100; i++)
		assert(test_bit(bits, i / 8, i % 8) == ((i % 3) == 0));

	for (size_t i = 0; i < 100; i++)
		assert(feature_is_set(bits, i) == ((i % 3) == 0));

	/* Simple test: single byte */
	bits = tal_arr(tmpctx, u8, 1);

	/* Compulsory feature */
	bits[0] = (1 << 0);
	assert(feature_offered(bits, 0));
	assert(!feature_offered(bits, 2));
	assert(!feature_offered(bits, 8));
	assert(!feature_offered(bits, 16));

	/* Optional feature */
	bits[0] = (1 << 1);
	assert(feature_offered(bits, 0));
	assert(!feature_offered(bits, 2));
	assert(!feature_offered(bits, 8));
	assert(!feature_offered(bits, 16));

	/* Endian-sensitive test: big-endian means we frob last byte here */
	bits = tal_arrz(tmpctx, u8, 2);

	bits[1] = (1 << 0);
	assert(feature_offered(bits, 0));
	assert(!feature_offered(bits, 2));
	assert(!feature_offered(bits, 8));
	assert(!feature_offered(bits, 16));

	/* Optional feature */
	bits[1] = (1 << 1);
	assert(feature_offered(bits, 0));
	assert(!feature_offered(bits, 2));
	assert(!feature_offered(bits, 8));
	assert(!feature_offered(bits, 16));

	/* We always support no features. */
	memset(bits, 0, tal_count(bits));
	assert(features_unsupported(bits) == -1);

	/* We must support our own features. */
	lf = get_offered_features(tmpctx);
	assert(features_unsupported(lf) == -1);

	/* We can add random odd features, no problem. */
	for (size_t i = 1; i < 16; i += 2) {
		bits = tal_dup_arr(tmpctx, u8, lf, tal_count(lf), 0);
		set_feature_bit(&bits, i);
		assert(features_unsupported(bits) == -1);
	}

	/* We can't add random even features. */
	for (size_t i = 0; i < 16; i += 2) {
		bits = tal_dup_arr(tmpctx, u8, lf, tal_count(lf), 0);
		set_feature_bit(&bits, i);

		/* Special case for missing compulsory feature */
		if (i == 2) {
			assert(features_unsupported(bits) == i);
		} else {
			assert((features_unsupported(bits) == -1)
			       == feature_supported(i, our_features,
						    ARRAY_SIZE(our_features)));
		}
	}

	wally_cleanup(0);
	tal_free(tmpctx);
	return 0;
}
