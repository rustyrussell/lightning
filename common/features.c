#include "features.h"
#include <assert.h>
#include <ccan/array_size/array_size.h>
#include <wire/peer_wire.h>

static const u32 our_localfeatures[] = {
	LOCAL_DATA_LOSS_PROTECT,
	LOCAL_INITIAL_ROUTING_SYNC,
	LOCAL_UPFRONT_SHUTDOWN_SCRIPT,
	LOCAL_GOSSIP_QUERIES,
#if EXPERIMENTAL_FEATURES
	LOCAL_EXTENDED_GOSSIP_QUERIES
#endif
};

static const u32 our_globalfeatures[] = {
};

/* BOLT #1:
 *
 * All data fields are unsigned big-endian unless otherwise specified.
 */
static void set_bit(u8 **ptr, u32 bit)
{
	size_t len = tal_count(*ptr);
	if (bit / 8 >= len) {
		size_t newlen = (bit / 8) + 1;
		u8 *newarr = tal_arrz(tal_parent(*ptr), u8, newlen);
		memcpy(newarr + (newlen - len), *ptr, len);
		tal_free(*ptr);
		*ptr = newarr;
		len = newlen;
	}
	(*ptr)[len - 1 - bit / 8] |= (1 << (bit % 8));
}

static bool test_bit(const u8 *features, size_t byte, unsigned int bit)
{
	assert(byte < tal_count(features));
	return features[tal_count(features) - 1 - byte] & (1 << (bit % 8));
}

/* We don't insist on anything, it's all optional. */
static u8 *mkfeatures(const tal_t *ctx, const u32 *arr, size_t n)
{
	u8 *f = tal_arr(ctx, u8, 0);

	for (size_t i = 0; i < n; i++)
		set_bit(&f, OPTIONAL_FEATURE(arr[i]));
	return f;
}

u8 *get_offered_globalfeatures(const tal_t *ctx)
{
	return mkfeatures(ctx,
			  our_globalfeatures, ARRAY_SIZE(our_globalfeatures));
}

u8 *get_offered_localfeatures(const tal_t *ctx)
{
	return mkfeatures(ctx,
			  our_localfeatures, ARRAY_SIZE(our_localfeatures));
}

static bool feature_set(const u8 *features, size_t bit)
{
	size_t bytenum = bit / 8;

	if (bytenum >= tal_count(features))
		return false;

	return test_bit(features, bytenum, bit % 8);
}

bool feature_offered(const u8 *features, size_t f)
{
	assert(f % 2 == 0);

	return feature_set(features, f)
		|| feature_set(features, OPTIONAL_FEATURE(f));
}

static bool feature_supported(int feature_bit,
			      const u32 *supported,
			      size_t num_supported)
{
	for (size_t i = 0; i < num_supported; i++) {
		if (supported[i] == feature_bit)
			return true;
	}
	return false;
}

/**
 * all_supported_features - Check if we support what's being asked
 *
 * Given the features vector that the remote connection is expecting
 * from us, we check to see if we support all even bit features, i.e.,
 * the required features.
 *
 * @bitmap: the features bitmap the peer is asking for
 * @supported: array of features we support
 * @num_supported: how many elements in supported
 */
static bool all_supported_features(const u8 *bitmap,
				   const u32 *supported,
				   size_t num_supported)
{
	size_t len = tal_count(bitmap) * 8;

	/* It's OK to be odd: only check even bits. */
	for (size_t bitnum = 0; bitnum < len; bitnum += 2) {
		if (!test_bit(bitmap, bitnum/8, bitnum%8))
			continue;

		if (feature_supported(bitnum, supported, num_supported))
			continue;

		return false;
	}
	return true;
}

bool features_supported(const u8 *globalfeatures, const u8 *localfeatures)
{
	/* BIT 2 would logically be "compulsory initial_routing_sync", but
	 * that does not exist, so we special case it. */
	if (feature_set(localfeatures,
			COMPULSORY_FEATURE(LOCAL_INITIAL_ROUTING_SYNC)))
		return false;

	return all_supported_features(globalfeatures,
				      our_globalfeatures,
				      ARRAY_SIZE(our_globalfeatures))
		&& all_supported_features(localfeatures,
					  our_localfeatures,
					  ARRAY_SIZE(our_localfeatures));
}

bool local_feature_negotiated(const u8 *lfeatures, size_t f)
{
	if (!feature_offered(lfeatures, f))
		return false;
	return feature_supported(f, our_localfeatures,
				 ARRAY_SIZE(our_localfeatures));
}

bool global_feature_negotiated(const u8 *gfeatures, size_t f)
{
	if (!feature_offered(gfeatures, f))
		return false;
	return feature_supported(f, our_globalfeatures,
				 ARRAY_SIZE(our_globalfeatures));
}
