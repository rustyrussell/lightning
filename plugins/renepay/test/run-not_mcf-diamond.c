#include "config.h"
#include "../flow.c"
#include "../not_mcf.c"
#include <ccan/array_size/array_size.h>
#include <ccan/read_write_all/read_write_all.h>
#include <common/bigsize.h>
#include <common/channel_id.h>
#include <common/gossip_store.h>
#include <common/node_id.h>
#include <common/setup.h>
#include <common/type_to_string.h>
#include <common/wireaddr.h>
#include <stdio.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_bigsize */
bigsize_t fromwire_bigsize(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_bigsize called!\n"); abort(); }
/* Generated stub for fromwire_channel_id */
bool fromwire_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			 struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_channel_id called!\n"); abort(); }
/* Generated stub for fromwire_wireaddr */
bool fromwire_wireaddr(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct wireaddr *addr UNNEEDED)
{ fprintf(stderr, "fromwire_wireaddr called!\n"); abort(); }
/* Generated stub for towire_bigsize */
void towire_bigsize(u8 **pptr UNNEEDED, const bigsize_t val UNNEEDED)
{ fprintf(stderr, "towire_bigsize called!\n"); abort(); }
/* Generated stub for towire_channel_id */
void towire_channel_id(u8 **pptr UNNEEDED, const struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "towire_channel_id called!\n"); abort(); }
/* Generated stub for towire_wireaddr */
void towire_wireaddr(u8 **pptr UNNEEDED, const struct wireaddr *addr UNNEEDED)
{ fprintf(stderr, "towire_wireaddr called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

static u8 empty_map[] = {
	0
};

static void print_flows(const char *desc,
			const struct gossmap *gossmap,
			struct flow **flows)
{
	printf("%s: %zu subflows\n", desc, tal_count(flows));
	for (size_t i = 0; i < tal_count(flows); i++) {
		struct amount_msat fee, delivered;
		printf("   ");
		for (size_t j = 0; j < tal_count(flows[i]->path); j++) {
			struct short_channel_id scid
				= gossmap_chan_scid(gossmap,
						    flows[i]->path[j]);
			printf("%s%s", j ? "->" : "",
			       type_to_string(tmpctx, struct short_channel_id, &scid));
		}
		delivered = flows[i]->amounts[tal_count(flows[i]->amounts)-1];
		if (!amount_msat_sub(&fee, flows[i]->amounts[0], delivered))
			abort();
		printf(" prob %.2f, %s delivered with fee %s\n",
		       flows[i]->success_prob,
		       type_to_string(tmpctx, struct amount_msat, &delivered),
		       type_to_string(tmpctx, struct amount_msat, &fee));
	}
}

int main(int argc, char *argv[])
{
	int fd;
	char *gossfile;
	struct gossmap *gossmap;
	struct node_id l1, l2, l3, l4;
	struct flow **flows;
	struct short_channel_id scid12, scid13, scid24, scid34;
	struct gossmap_localmods *mods;
	struct capacity_range *capacities;

	common_setup(argv[0]);

	fd = tmpdir_mkstemp(tmpctx, "run-not_mcf-diamond.XXXXXX", &gossfile);
	ASSERT(write_all(fd, empty_map, sizeof(empty_map)));

	gossmap = gossmap_load(tmpctx, gossfile, NULL);
	ASSERT(gossmap);

	/* These are in ascending order, for easy direction setting */
	ASSERT(node_id_from_hexstr("022d223620a359a47ff7f7ac447c85c46c923da53389221a0054c11c1e3ca31d59", 66, &l1));
	ASSERT(node_id_from_hexstr("0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518", 66, &l2));
	ASSERT(node_id_from_hexstr("035d2b1192dfba134e10e540875d366ebc8bc353d5aa766b80c090b39c3a5d885d", 66, &l3));
	ASSERT(node_id_from_hexstr("0382ce59ebf18be7d84677c2e35f23294b9992ceca95491fcf8a56c6cb2d9de199", 66, &l4));
	ASSERT(short_channel_id_from_str("1x2x0", 7, &scid12));
	ASSERT(short_channel_id_from_str("1x3x0", 7, &scid13));
	ASSERT(short_channel_id_from_str("2x4x0", 7, &scid24));
	ASSERT(short_channel_id_from_str("3x4x0", 7, &scid34));

	mods = gossmap_localmods_new(tmpctx);

	/* 1->2->4 has capacity 10M sat, 1->3->4 has capacity 5M sat (lower fee!) */
	ASSERT(gossmap_local_addchan(mods, &l1, &l2, &scid12, NULL));
	ASSERT(gossmap_local_updatechan(mods, &scid12,
					AMOUNT_MSAT(0),
					AMOUNT_MSAT(1000000000),
					0, 1000, 5,
					true,
					0));
	ASSERT(gossmap_local_addchan(mods, &l2, &l4, &scid24, NULL));
	ASSERT(gossmap_local_updatechan(mods, &scid24,
					AMOUNT_MSAT(0),
					AMOUNT_MSAT(1000000000),
					0, 1000, 5,
					true,
					0));
	ASSERT(gossmap_local_addchan(mods, &l1, &l3, &scid13, NULL));
	ASSERT(gossmap_local_updatechan(mods, &scid13,
					AMOUNT_MSAT(0),
					AMOUNT_MSAT(500000000),
					0, 500, 5,
					true,
					0));
	ASSERT(gossmap_local_addchan(mods, &l3, &l4, &scid34, NULL));
	ASSERT(gossmap_local_updatechan(mods, &scid34,
					AMOUNT_MSAT(0),
					AMOUNT_MSAT(500000000),
					0, 500, 5,
					true,
					0));

	gossmap_apply_localmods(gossmap, mods);

	/* The local chans have no "capacity", so set them manually. */
	capacities = flow_capacity_init(tmpctx, gossmap);
	capacities[gossmap_chan_idx(gossmap, gossmap_find_chan(gossmap, &scid12))].max
		= AMOUNT_MSAT(1000000000);
	capacities[gossmap_chan_idx(gossmap, gossmap_find_chan(gossmap, &scid24))].max
		= AMOUNT_MSAT(1000000000);
	capacities[gossmap_chan_idx(gossmap, gossmap_find_chan(gossmap, &scid13))].max
		= AMOUNT_MSAT(500000000);
	capacities[gossmap_chan_idx(gossmap, gossmap_find_chan(gossmap, &scid34))].max
		= AMOUNT_MSAT(500000000);

	flows = minflow(tmpctx, gossmap,
			gossmap_find_node(gossmap, &l1),
			gossmap_find_node(gossmap, &l4),
			capacities,
			/* Half the capacity */
			AMOUNT_MSAT(500000000),
			0.00001,
			1);

	print_flows("Simple minflow", gossmap, flows);
	ASSERT(tal_count(flows) == 2);

	common_shutdown();
}
