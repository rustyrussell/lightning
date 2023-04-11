#include "config.h"

#include <ccan/array_size/array_size.h>
#include <ccan/err/err.h>
#include <ccan/opt/opt.h>
#include <ccan/tal/str/str.h>
#include <ccan/read_write_all/read_write_all.h>
#include <common/bigsize.h>
#include <common/channel_id.h>
#include <common/node_id.h>
#include <common/setup.h>
#include <common/type_to_string.h>
#include <common/wireaddr.h>
#include <inttypes.h>
#include <stdio.h>

//static bool print_enable = true;
//#define SUPERVERBOSE(...) do { if (print_enable) printf(__VA_ARGS__); } while(0)
  #include "../not_mcf.c"
  #include "../flow.c"

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

/* not_mcf sets NDEBUG, so assert() is useless */
#define ASSERT(x) do { if (!(x)) abort(); } while(0)

static void print_flows(const char *desc,
			const struct gossmap *gossmap,
			struct flow **flows)
{
	struct amount_msat total_fee = AMOUNT_MSAT(0),
		total_delivered = AMOUNT_MSAT(0);

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
		if (!amount_msat_add(&total_fee, total_fee, fee))
			abort();
		if (!amount_msat_add(&total_delivered, total_delivered, delivered))
			abort();
	}
	printf("Delivered %s at fee %s\n",
	       type_to_string(tmpctx, struct amount_msat, &total_delivered),
	       type_to_string(tmpctx, struct amount_msat, &total_fee));
}

int main(int argc, char *argv[])
{
	struct gossmap *gossmap;
	struct flow **flows;
	struct node_id srcid, dstid;
	struct gossmap_node *src, *dst;
	struct amount_msat amount;
	bool verbose = false;

	common_setup(argv[0]);
	opt_register_noarg("-v|--verbose", opt_set_bool, &verbose,
			   "Increase verbosity");

	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (argc != 6)
		errx(1, "Usage: %s <gossmap> <srcnode> <dstnode> <amount> <feecert>", argv[0]);

	gossmap = gossmap_load(tmpctx, argv[1], NULL);
	assert(gossmap);

	if (!node_id_from_hexstr(argv[2], strlen(argv[2]), &srcid)
	    || !node_id_from_hexstr(argv[3], strlen(argv[3]), &dstid))
		errx(1, "Usage: %s <srcnode> <dstnode>", argv[0]);

	src = gossmap_find_node(gossmap, &srcid);
	assert(src);
	dst = gossmap_find_node(gossmap, &dstid);
	assert(dst);
	assert(src != dst);

	amount = amount_msat(atol(argv[4]));
	assert(!amount_msat_eq(amount, AMOUNT_MSAT(0)));

	flows = minflow(tmpctx, gossmap, src, dst,
			flow_capacity_init(tmpctx, gossmap),
			amount,
			atof(argv[5]),
			1);

	print_flows("Flows", gossmap, flows);
	common_shutdown();
}
