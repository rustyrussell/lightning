#include "config.h"
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
#include <assert.h>

#include <bitcoin/short_channel_id.h>
#include <ccan/htable/htable_type.h>

struct chan_extra {
	struct short_channel_id scid;
	int value;
};

static inline const struct short_channel_id
chan_extra_scid(const struct chan_extra *cd)
{
	return cd->scid;
}

static inline size_t hash_scid(const struct short_channel_id scid)
{
	/* scids cost money to generate, so simple hash works here */
	return (scid.u64 >> 32)
		^ (scid.u64 >> 16)
		^ scid.u64;
}

static inline bool chan_extra_eq_scid(const struct chan_extra *cd,
				      const struct short_channel_id scid)
{
	return short_channel_id_eq(&scid, &cd->scid);
}

HTABLE_DEFINE_TYPE(struct chan_extra,
		   chan_extra_scid, hash_scid, chan_extra_eq_scid,
		   chan_extra_map);


static void destroy_chan_extra(struct chan_extra *ce,
			       struct chan_extra_map *chan_extra_map)
{
	printf("calling %s with value = %d\n",__PRETTY_FUNCTION__,ce->value);
	chan_extra_map_del(chan_extra_map, ce);
}
static inline void new_chan_extra_value(
	const tal_t *ctx,
	struct chan_extra_map *chan_extra_map,
	const struct short_channel_id scid,
	int value)
{
	// struct chan_extra *ce = tal(chan_extra_map, struct chan_extra);
	struct chan_extra *ce = tal(ctx, struct chan_extra);
	
	ce->scid = scid;
	ce->value=value;
	
	/* Remove self from map when done */
	chan_extra_map_add(chan_extra_map, ce);
	tal_add_destructor2(ce, destroy_chan_extra, chan_extra_map);
}

static struct chan_extra*
get_chan_extra_value(struct chan_extra_map *chan_extra_map,
			    const struct short_channel_id scid)
{
	struct chan_extra *ce;

	ce = chan_extra_map_get(chan_extra_map, scid);
	if (!ce)
		return NULL;
	return ce;
}

static void valgrind_ok1(void)
{
	struct short_channel_id scid1,scid2;
	
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	struct chan_extra_map *chan_extra_map
		= tal(this_ctx, struct chan_extra_map);
	
	chan_extra_map_init(chan_extra_map);
	
	assert(short_channel_id_from_str("1x1x0", 7, &scid1));
	assert(short_channel_id_from_str("2x1x0", 7, &scid2));
	
	{
		tal_t *local_ctx = tal(this_ctx,tal_t);
		
		/* in this case: elements are allocated in a local context, and freed at
		 * the end, i.e. before chan_extra_map is freed. */
		new_chan_extra_value(local_ctx,chan_extra_map,scid1,11);
		new_chan_extra_value(local_ctx,chan_extra_map,scid2,21);
		
		struct chan_extra *x1 = get_chan_extra_value(chan_extra_map,scid1);
		struct chan_extra *x2 = get_chan_extra_value(chan_extra_map,scid2);
		
		assert(x1->value==11);
		assert(x2->value==21);
		
		tal_free(local_ctx);
	}
	
	tal_free(this_ctx);

}
static void valgrind_ok2(void)
{
	struct short_channel_id scid1,scid2;
	
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	struct chan_extra_map *chan_extra_map
		= tal(this_ctx, struct chan_extra_map);
	
	chan_extra_map_init(chan_extra_map);
	
	assert(short_channel_id_from_str("1x2x0", 7, &scid1));
	assert(short_channel_id_from_str("2x2x0", 7, &scid2));
	
	
	/* in this case: elements are allocated with chan_extra_map as parent.
	 * the end of this function. i.e. before chan_extra_map is freed. */
	new_chan_extra_value(chan_extra_map,chan_extra_map,scid1,12);
	new_chan_extra_value(chan_extra_map,chan_extra_map,scid2,22);
	
	struct chan_extra *x1 = get_chan_extra_value(chan_extra_map,scid1);
	struct chan_extra *x2 = get_chan_extra_value(chan_extra_map,scid2);
	
	assert(x1->value==12);
	assert(x2->value==22);
	
	/* elements are freed before chan_extra_map is freed */
	tal_free(x1);
	tal_free(x2);
	
	tal_free(this_ctx);

}
static void valgrind_fail3(void)
{
	struct short_channel_id scid1,scid2;
	
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	struct chan_extra_map *chan_extra_map
		= tal(this_ctx, struct chan_extra_map);
	
	chan_extra_map_init(chan_extra_map);
	
	assert(short_channel_id_from_str("1x3x0", 7, &scid1));
	assert(short_channel_id_from_str("2x3x0", 7, &scid2));
	
	
	/* in this case: elements are allocated with chan_extra_map as parent.
	 * the end of this function. i.e. before chan_extra_map is freed. */
	new_chan_extra_value(chan_extra_map,chan_extra_map,scid1,13);
	new_chan_extra_value(chan_extra_map,chan_extra_map,scid2,23);
	
	struct chan_extra *x1 = get_chan_extra_value(chan_extra_map,scid1);
	struct chan_extra *x2 = get_chan_extra_value(chan_extra_map,scid2);
	
	assert(x1->value==13);
	assert(x2->value==23);
	
	/* Valgrind complains. It seems that the hash table is trying to remove
	 * the element at a moment when its memory has already been released. 
	 * If the following two lines are not commented the error disapears. */
	tal_free(x1);
	tal_free(x2);
	
	tal_free(this_ctx);
}

static void destroy_int(int* x)
{
	fprintf(stderr,"Destroying %d\n",*x);
}

/* This clearly shows that the parent is destroyed before the children. 
 * That's the reason why the chan_extra destructor fails. */
static void test_order(void)
{
	const tal_t *this_ctx = tal(tmpctx,tal_t);
	
	int *parent = tal(this_ctx,int);
	tal_add_destructor(parent,destroy_int);
	*parent = 1;
	
	int *child = tal(parent,int);
	tal_add_destructor(child,destroy_int);
	*child=2;
	
	tal_free(this_ctx);
	
	// prints:
	// Destroying 1
	// Destroying 2
}

int main(int argc, char *argv[])
{
	common_setup(argv[0]);
	valgrind_ok1();
	valgrind_ok2();
	valgrind_fail3();
	
	test_order();
	common_shutdown();
}

