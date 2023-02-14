#include "config.h"
#include "../routing.c"
#include "../common/timeout.c"
#include <common/blinding.h>
#include <common/channel_type.h>
#include <common/ecdh.h>
#include <common/json_stream.h>
#include <common/onionreply.h>
#include <common/setup.h>
#include <stdio.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for blinding_hash_e_and_ss */
void blinding_hash_e_and_ss(const struct pubkey *e UNNEEDED,
			    const struct secret *ss UNNEEDED,
			    struct sha256 *sha UNNEEDED)
{ fprintf(stderr, "blinding_hash_e_and_ss called!\n"); abort(); }
/* Generated stub for blinding_next_privkey */
bool blinding_next_privkey(const struct privkey *e UNNEEDED,
			   const struct sha256 *h UNNEEDED,
			   struct privkey *next UNNEEDED)
{ fprintf(stderr, "blinding_next_privkey called!\n"); abort(); }
/* Generated stub for blinding_next_pubkey */
bool blinding_next_pubkey(const struct pubkey *pk UNNEEDED,
			  const struct sha256 *h UNNEEDED,
			  struct pubkey *next UNNEEDED)
{ fprintf(stderr, "blinding_next_pubkey called!\n"); abort(); }
/* Generated stub for cupdate_different */
bool cupdate_different(struct gossip_store *gs UNNEEDED,
		       const struct half_chan *hc UNNEEDED,
		       const u8 *cupdate UNNEEDED)
{ fprintf(stderr, "cupdate_different called!\n"); abort(); }
/* Generated stub for gossip_store_add */
u64 gossip_store_add(struct gossip_store *gs UNNEEDED, const u8 *gossip_msg UNNEEDED,
		     u32 timestamp UNNEEDED, bool push UNNEEDED, bool zombie UNNEEDED, bool spam UNNEEDED,
		     const u8 *addendum UNNEEDED)
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
/* Generated stub for gossip_store_mark_channel_deleted */
void gossip_store_mark_channel_deleted(struct gossip_store *gs UNNEEDED,
				       const struct short_channel_id *scid UNNEEDED)
{ fprintf(stderr, "gossip_store_mark_channel_deleted called!\n"); abort(); }
/* Generated stub for gossip_store_mark_channel_zombie */
void gossip_store_mark_channel_zombie(struct gossip_store *gs UNNEEDED,
				      struct broadcastable *bcast UNNEEDED)
{ fprintf(stderr, "gossip_store_mark_channel_zombie called!\n"); abort(); }
/* Generated stub for gossip_store_mark_cupdate_zombie */
void gossip_store_mark_cupdate_zombie(struct gossip_store *gs UNNEEDED,
				      struct broadcastable *bcast UNNEEDED)
{ fprintf(stderr, "gossip_store_mark_cupdate_zombie called!\n"); abort(); }
/* Generated stub for memleak_add_helper_ */
void memleak_add_helper_(const tal_t *p UNNEEDED, void (*cb)(struct htable *memtable UNNEEDED,
						    const tal_t *)){ }
/* Generated stub for memleak_scan_htable */
void memleak_scan_htable(struct htable *memtable UNNEEDED, const struct htable *ht UNNEEDED)
{ fprintf(stderr, "memleak_scan_htable called!\n"); abort(); }
/* Generated stub for memleak_scan_intmap_ */
void memleak_scan_intmap_(struct htable *memtable UNNEEDED, const struct intmap *m UNNEEDED)
{ fprintf(stderr, "memleak_scan_intmap_ called!\n"); abort(); }
/* Generated stub for nannounce_different */
bool nannounce_different(struct gossip_store *gs UNNEEDED,
			 const struct node *node UNNEEDED,
			 const u8 *nannounce UNNEEDED,
			 bool *only_missing_tlv UNNEEDED)
{ fprintf(stderr, "nannounce_different called!\n"); abort(); }
/* Generated stub for notleak_ */
void *notleak_(void *ptr UNNEEDED, bool plus_children UNNEEDED)
{ fprintf(stderr, "notleak_ called!\n"); abort(); }
/* Generated stub for peer_supplied_good_gossip */
void peer_supplied_good_gossip(struct peer *peer UNNEEDED, size_t amount UNNEEDED)
{ fprintf(stderr, "peer_supplied_good_gossip called!\n"); abort(); }
/* Generated stub for sanitize_error */
char *sanitize_error(const tal_t *ctx UNNEEDED, const u8 *errmsg UNNEEDED,
		     struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "sanitize_error called!\n"); abort(); }
/* Generated stub for status_failed */
void status_failed(enum status_failreason code UNNEEDED,
		   const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "status_failed called!\n"); abort(); }
/* Generated stub for status_fmt */
void status_fmt(enum log_level level UNNEEDED,
		const struct node_id *peer UNNEEDED,
		const char *fmt UNNEEDED, ...)

{ fprintf(stderr, "status_fmt called!\n"); abort(); }
/* Generated stub for towire_warningfmt */
u8 *towire_warningfmt(const tal_t *ctx UNNEEDED,
		      const struct channel_id *channel UNNEEDED,
		      const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "towire_warningfmt called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

/* NOOP stub for gossip_store_new */
struct gossip_store *gossip_store_new(struct routing_state *rstate UNNEEDED,
				      struct list_head *peers UNNEEDED)
{
	return NULL;
}

int main(int argc, char *argv[])
{
	struct routing_state *rstate;
	struct timers timers;
	struct timer *t;
	struct short_channel_id scid1, scid2;

	common_setup(argv[0]);

	timers_init(&timers, time_mono());
	/* Random uninitalized node_id, we don't reference it. */
	rstate = new_routing_state(tmpctx, tal(tmpctx, struct node_id),
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
	timer_expired(t);

	/* Still there, just old.  Refresh scid1 */
	assert(rstate->num_txout_failures == 0);
	assert(in_txout_failures(rstate, &scid1));
	assert(rstate->num_txout_failures == 1);

	t = timers_expire(&timers, timemono_add(time_mono(),
						time_from_sec(3601)));
	assert(t);
	timer_expired(t);

	assert(rstate->num_txout_failures == 0);
	assert(in_txout_failures(rstate, &scid1));
	assert(rstate->num_txout_failures == 1);
	assert(!in_txout_failures(rstate, &scid2));

	common_shutdown();
	timers_cleanup(&timers);
	return 0;
}
