/* Test based on bug report from Ken Sedgewick, where routing would crash.
 * I took his gossip_store (this version is compacted) and brute forced routes from
 * his node with different amounts until I reproduced it */
#include "config.h"
#include <assert.h>
#include <common/channel_type.h>
#include <common/dijkstra.h>
#include <common/gossmap.h>
#include <common/gossip_store.h>
#include <common/route.h>
#include <common/setup.h>
#include <common/type_to_string.h>
#include <common/utils.h>
#include <math.h>
#include <stdio.h>
#include <inttypes.h>
#include <wire/peer_wiregen.h>
#include <unistd.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_bigsize */
bigsize_t fromwire_bigsize(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_bigsize called!\n"); abort(); }
/* Generated stub for fromwire_channel_id */
bool fromwire_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			 struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_channel_id called!\n"); abort(); }
/* Generated stub for fromwire_tlv */
bool fromwire_tlv(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
		  const struct tlv_record_type *types UNNEEDED, size_t num_types UNNEEDED,
		  void *record UNNEEDED, struct tlv_field **fields UNNEEDED,
		  const u64 *extra_types UNNEEDED, size_t *err_off UNNEEDED, u64 *err_type UNNEEDED)
{ fprintf(stderr, "fromwire_tlv called!\n"); abort(); }
/* Generated stub for towire_bigsize */
void towire_bigsize(u8 **pptr UNNEEDED, const bigsize_t val UNNEEDED)
{ fprintf(stderr, "towire_bigsize called!\n"); abort(); }
/* Generated stub for towire_channel_id */
void towire_channel_id(u8 **pptr UNNEEDED, const struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "towire_channel_id called!\n"); abort(); }
/* Generated stub for towire_tlv */
void towire_tlv(u8 **pptr UNNEEDED,
		const struct tlv_record_type *types UNNEEDED, size_t num_types UNNEEDED,
		const void *record UNNEEDED)
{ fprintf(stderr, "towire_tlv called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

/* Node id 03942f5fe67645fdce4584e7f159c1f0a396b05fbc15f0fb7d6e83c553037b1c73 */
static struct gossmap *gossmap;

static double capacity_bias(const struct gossmap *map,
			    const struct gossmap_chan *c,
			    int dir,
			    struct amount_msat amount)
{
	struct amount_sat capacity;
	u64 amtmsat = amount.millisatoshis; /* Raw: lengthy math */
	double capmsat;

	/* Can fail in theory if gossmap changed underneath. */
	if (!gossmap_chan_get_capacity(map, c, &capacity))
		return 0;

	capmsat = (double)capacity.satoshis * 1000; /* Raw: lengthy math */
	return -log((capmsat + 1 - amtmsat) / (capmsat + 1));
}

/* Prioritize costs over distance, but bias to larger channels. */
static u64 route_score(struct amount_msat fee,
		       struct amount_msat risk,
		       struct amount_msat total,
		       int dir,
		       const struct gossmap_chan *c)
{
	double score;
	struct amount_msat msat;

	/* These two are comparable, so simply sum them. */
	if (!amount_msat_add(&msat, fee, risk))
		msat = AMOUNT_MSAT(-1ULL);

	/* Slight tiebreaker bias: 1 msat per distance */
	if (!amount_msat_add(&msat, msat, AMOUNT_MSAT(1)))
		msat = AMOUNT_MSAT(-1ULL);

	/* Percent penalty at different channel capacities:
	 * 1%: 1%
	 * 10%: 11%
	 * 25%: 29%
	 * 50%: 69%
	 * 75%: 138%
	 * 90%: 230%
	 * 95%: 300%
	 * 99%: 461%
	 */
	score = (capacity_bias(gossmap, c, dir, total) + 1)
		* msat.millisatoshis; /* Raw: Weird math */
	if (score > 0xFFFFFFFF)
		return 0xFFFFFFFF;

	/* Cast unnecessary, but be explicit! */
	return (u64)score;
}

int main(int argc, char *argv[])
{
	const double riskfactor = 10;
	const u32 final_delay = 159;
	struct node_id my_id;
	const struct gossmap_node **nodes, *me;
	u64 amt;

	common_setup(argv[0]);

	/* This used to crash! */
	if (!argv[1])
		amt = 8388607;
	else
		amt = atol(argv[1]);
	gossmap = gossmap_load(tmpctx, "tests/data/routing_gossip_store", NULL);

	nodes = tal_arr(tmpctx, const struct gossmap_node *, 0);
	for (struct gossmap_node *n = gossmap_first_node(gossmap);
	     n;
	     n = gossmap_next_node(gossmap, n)) {
		tal_arr_expand(&nodes, n);
	}

	assert(node_id_from_hexstr("03942f5fe67645fdce4584e7f159c1f0a396b05fbc15f0fb7d6e83c553037b1c73",
				   strlen("03942f5fe67645fdce4584e7f159c1f0a396b05fbc15f0fb7d6e83c553037b1c73"),
				   &my_id));
	me = gossmap_find_node(gossmap, &my_id);
	assert(me);

	/* We overlay our own channels as zero fee & delay, since we don't pay fees */
	struct gossmap_localmods *localmods = gossmap_localmods_new(gossmap);
	for (size_t i = 0; i < me->num_chans; i++) {
		int dir;
		struct short_channel_id scid;
		struct gossmap_chan *c = gossmap_nth_chan(gossmap, me, i, &dir);

		if (!c->half[dir].enabled)
			continue;
		scid = gossmap_chan_scid(gossmap, c);
		assert(gossmap_local_updatechan(localmods, &scid,
						amount_msat(fp16_to_u64(c->half[dir].htlc_min)),
						amount_msat(fp16_to_u64(c->half[dir].htlc_max)),
						0, 0, 0, true, dir));
	}
	gossmap_apply_localmods(gossmap, localmods);

	printf("Destination node, success, probability, hops, fees, cltv, scid...\n");
	for (size_t i = 0; i < tal_count(nodes); i++) {
		const struct dijkstra *dij;
		struct route_hop *r;
		struct node_id them;

		if (nodes[i] == me)
			continue;

		dij = dijkstra(tmpctx, gossmap, nodes[i], amount_msat(amt), riskfactor,
			       route_can_carry, route_score, NULL);
		r = route_from_dijkstra(tmpctx, gossmap, dij, me, amount_msat(amt), final_delay);

		gossmap_node_get_id(gossmap, nodes[i], &them);

		printf("%s,", node_id_to_hexstr(tmpctx, &them));
		if (!r) {
			printf("0,0.0,");
		} else {
			double probability = 1;
			for (size_t j = 0; j < tal_count(r); j++) {
				struct amount_sat capacity_sat;
				u64 cap_msat;
				struct gossmap_chan *c = gossmap_find_chan(gossmap, &r[j].scid);
				assert(c);
				assert(gossmap_chan_get_capacity(gossmap, c, &capacity_sat));

				cap_msat = capacity_sat.satoshis * 1000;
				/* Assume linear distribution, implying probability depends on
				 * amount we would leave in channel */
				assert(cap_msat >= r[0].amount.millisatoshis);
				probability *= (double)(cap_msat - r[0].amount.millisatoshis) / cap_msat;
			}
			printf("1,%f,%zu,%"PRIu64",%u",
			       probability,
			       tal_count(r),
			       r[0].amount.millisatoshis - amt,
			       r[0].delay - final_delay);
			for (size_t j = 0; j < tal_count(r); j++)
				printf(",%s/%u", short_channel_id_to_str(tmpctx, &r[j].scid), r[j].direction);
		}
		printf("\n");
	}

	common_shutdown();
	return 0;
}
