#include "../routing.c"
#include "../common/timeout.c"
#include <stdio.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for cupdate_different */
bool cupdate_different(struct gossip_store *gs UNNEEDED,
		       const struct half_chan *hc UNNEEDED,
		       const u8 *cupdate UNNEEDED)
{ fprintf(stderr, "cupdate_different called!\n"); abort(); }
/* Generated stub for fromwire_gossipd_local_add_channel */
bool fromwire_gossipd_local_add_channel(const void *p UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, struct node_id *remote_node_id UNNEEDED, struct amount_sat *satoshis UNNEEDED)
{ fprintf(stderr, "fromwire_gossipd_local_add_channel called!\n"); abort(); }
/* Generated stub for fromwire_wireaddr */
bool fromwire_wireaddr(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct wireaddr *addr UNNEEDED)
{ fprintf(stderr, "fromwire_wireaddr called!\n"); abort(); }
/* Generated stub for gossip_store_add */
u64 gossip_store_add(struct gossip_store *gs UNNEEDED, const u8 *gossip_msg UNNEEDED,
		     u32 timestamp UNNEEDED, const u8 *addendum UNNEEDED)
{ fprintf(stderr, "gossip_store_add called!\n"); abort(); }
/* Generated stub for gossip_store_add_private_update */
u64 gossip_store_add_private_update(struct gossip_store *gs UNNEEDED, const u8 *update UNNEEDED)
{ fprintf(stderr, "gossip_store_add_private_update called!\n"); abort(); }
/* Generated stub for gossip_store_delete */
void gossip_store_delete(struct gossip_store *gs UNNEEDED,
			 struct broadcastable *bcast UNNEEDED,
			 int type UNNEEDED)
{ fprintf(stderr, "gossip_store_delete called!\n"); abort(); }
/* Generated stub for gossip_store_get */
const u8 *gossip_store_get(const tal_t *ctx UNNEEDED,
			   struct gossip_store *gs UNNEEDED,
			   u64 offset UNNEEDED)
{ fprintf(stderr, "gossip_store_get called!\n"); abort(); }
/* Generated stub for gossip_store_get_private_update */
const u8 *gossip_store_get_private_update(const tal_t *ctx UNNEEDED,
					  struct gossip_store *gs UNNEEDED,
					  u64 offset UNNEEDED)
{ fprintf(stderr, "gossip_store_get_private_update called!\n"); abort(); }
/* Generated stub for memleak_add_helper_ */
void memleak_add_helper_(const tal_t *p UNNEEDED, void (*cb)(struct htable *memtable UNNEEDED,
						    const tal_t *)){ }
/* Generated stub for memleak_remove_htable */
void memleak_remove_htable(struct htable *memtable UNNEEDED, const struct htable *ht UNNEEDED)
{ fprintf(stderr, "memleak_remove_htable called!\n"); abort(); }
/* Generated stub for memleak_remove_intmap_ */
void memleak_remove_intmap_(struct htable *memtable UNNEEDED, const struct intmap *m UNNEEDED)
{ fprintf(stderr, "memleak_remove_intmap_ called!\n"); abort(); }
/* Generated stub for nannounce_different */
bool nannounce_different(struct gossip_store *gs UNNEEDED,
			 const struct node *node UNNEEDED,
			 const u8 *nannounce UNNEEDED)
{ fprintf(stderr, "nannounce_different called!\n"); abort(); }
/* Generated stub for onion_type_name */
const char *onion_type_name(int e UNNEEDED)
{ fprintf(stderr, "onion_type_name called!\n"); abort(); }
/* Generated stub for sanitize_error */
char *sanitize_error(const tal_t *ctx UNNEEDED, const u8 *errmsg UNNEEDED,
		     struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "sanitize_error called!\n"); abort(); }
/* Generated stub for status_fmt */
void status_fmt(enum log_level level UNNEEDED, const char *fmt UNNEEDED, ...)

{ fprintf(stderr, "status_fmt called!\n"); abort(); }
/* Generated stub for towire_errorfmt */
u8 *towire_errorfmt(const tal_t *ctx UNNEEDED,
		    const struct channel_id *channel UNNEEDED,
		    const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "towire_errorfmt called!\n"); abort(); }
/* Generated stub for towire_gossip_store_channel_amount */
u8 *towire_gossip_store_channel_amount(const tal_t *ctx UNNEEDED, struct amount_sat satoshis UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_channel_amount called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

/* NOOP stub for gossip_store_new */
struct gossip_store *gossip_store_new(struct routing_state *rstate UNNEEDED,
				      struct list_head *peers UNNEEDED)
{
	return NULL;
}

int main(void)
{
	struct routing_state *rstate;
	struct timers timers;
	struct timer *t;
	struct short_channel_id scid1, scid2;

	setup_locale();
	setup_tmpctx();

	timers_init(&timers, time_mono());
	/* Random uninitalized node_id, we don't reference it. */
	rstate = new_routing_state(tmpctx, NULL, tal(tmpctx, struct node_id),
				   NULL, &timers, NULL, false, false);

	scid1.u64 = 100;
	scid2.u64 = 200;
	assert(!in_txout_failures(rstate, &scid1));
	assert(!in_txout_failures(rstate, &scid2));

	add_to_txout_failures(rstate, &scid1);
	assert(in_txout_failures(rstate, &scid1));
	assert(!in_txout_failures(rstate, &scid2));
	assert(rstate->num_txout_failures == 1);

	add_to_txout_failures(rstate, &scid2);
	assert(in_txout_failures(rstate, &scid1));
	assert(in_txout_failures(rstate, &scid2));
	assert(rstate->num_txout_failures == 2);

	/* Move time forward 1 hour. */
	t = timers_expire(&timers, timemono_add(time_mono(),
						time_from_sec(3601)));
	assert(t);
	timer_expired(NULL, t);

	/* Still there, just old.  Refresh scid1 */
	assert(rstate->num_txout_failures == 0);
	assert(in_txout_failures(rstate, &scid1));
	assert(rstate->num_txout_failures == 1);

	t = timers_expire(&timers, timemono_add(time_mono(),
						time_from_sec(3601)));
	assert(t);
	timer_expired(NULL, t);

	assert(rstate->num_txout_failures == 0);
	assert(in_txout_failures(rstate, &scid1));
	assert(rstate->num_txout_failures == 1);
	assert(!in_txout_failures(rstate, &scid2));

	tal_free(tmpctx);
	timers_cleanup(&timers);
	return 0;
}
