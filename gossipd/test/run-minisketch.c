#define MINISKETCH_CAPACITY 10
#include "../routing.c"
#include "../minisketch.c"
#include <stdio.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for add_unknown_scid */
bool add_unknown_scid(struct seeker *seeker UNNEEDED,
		      const struct short_channel_id *scid UNNEEDED,
		      struct peer *peer UNNEEDED)
{ fprintf(stderr, "add_unknown_scid called!\n"); abort(); }
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
		     u32 timestamp UNNEEDED, bool push UNNEEDED, const u8 *addendum UNNEEDED)
{ fprintf(stderr, "gossip_store_add called!\n"); abort(); }
/* Generated stub for gossip_store_add_private_update */
u64 gossip_store_add_private_update(struct gossip_store *gs UNNEEDED, const u8 *update UNNEEDED)
{ fprintf(stderr, "gossip_store_add_private_update called!\n"); abort(); }
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
/* Generated stub for notleak_ */
void *notleak_(const void *ptr UNNEEDED, bool plus_children UNNEEDED)
{ fprintf(stderr, "notleak_ called!\n"); abort(); }
/* Generated stub for onion_type_name */
const char *onion_type_name(int e UNNEEDED)
{ fprintf(stderr, "onion_type_name called!\n"); abort(); }
/* Generated stub for peer_supplied_good_gossip */
void peer_supplied_good_gossip(struct peer *peer UNNEEDED, size_t amount UNNEEDED)
{ fprintf(stderr, "peer_supplied_good_gossip called!\n"); abort(); }
/* Generated stub for queue_peer_from_store */
void queue_peer_from_store(struct peer *peer UNNEEDED,
			   const struct broadcastable *bcast UNNEEDED)
{ fprintf(stderr, "queue_peer_from_store called!\n"); abort(); }
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
/* Generated stub for towire_errorfmt */
u8 *towire_errorfmt(const tal_t *ctx UNNEEDED,
		    const struct channel_id *channel UNNEEDED,
		    const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "towire_errorfmt called!\n"); abort(); }
/* Generated stub for towire_gossip_store_channel_amount */
u8 *towire_gossip_store_channel_amount(const tal_t *ctx UNNEEDED, struct amount_sat satoshis UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_channel_amount called!\n"); abort(); }
/* Generated stub for uniquify_node_ids */
void uniquify_node_ids(struct node_id **ids UNNEEDED)
{ fprintf(stderr, "uniquify_node_ids called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

/* Empty stubs */
struct gossip_store *gossip_store_new(struct routing_state *rstate UNNEEDED,
				      struct list_head *peers UNNEEDED)
{
	return NULL;
}

struct oneshot *new_reltimer_(struct timers *timers UNNEEDED,
			      const tal_t *ctx UNNEEDED,
			      struct timerel expire UNNEEDED,
			      void (*cb)(void *) UNNEEDED, void *arg UNNEEDED)
{
	return NULL;
}

void gossip_store_delete(struct gossip_store *gs UNNEEDED,
			 struct broadcastable *bcast UNNEEDED,
			 int type UNNEEDED)
{
}

#if EXPERIMENTAL_FEATURES
static void node_id_from_privkey(const struct privkey *p, struct node_id *id)
{
	struct pubkey k;
	pubkey_from_privkey(p, &k);
	node_id_from_pubkey(id, &k);
}

static void check_encode(struct routing_state *rstate, const struct chan *c)
{
	u64 val = sketchval_encode(rstate->current_blockheight, c);
	struct short_channel_id scid;
	u32 cupdate_ts1, cupdate_ts1_bits,
		cupdate_ts2, cupdate_ts2_bits,
		nannounce_ts1, nannounce_ts1_bits,
		nannounce_ts2, nannounce_ts2_bits;

	assert(sketchval_decode(rstate->current_blockheight,
				 val,
				 &scid,
				 &cupdate_ts1, &cupdate_ts1_bits,
				 &cupdate_ts2, &cupdate_ts2_bits,
				 &nannounce_ts1, &nannounce_ts1_bits,
				 &nannounce_ts2, &nannounce_ts2_bits));
	assert(short_channel_id_eq(&c->scid, &scid));
	if (is_halfchan_defined(&c->half[0]))
		assert(c->half[0].bcast.timestamp % ((1 << cupdate_ts1_bits) - 1)
		       == cupdate_ts1);
	else
		assert(cupdate_ts1 == ((1 << cupdate_ts1_bits) - 1));
	if (is_halfchan_defined(&c->half[1]))
		assert(c->half[1].bcast.timestamp % ((1 << cupdate_ts2_bits) - 1)
		       == cupdate_ts2);
	else
		assert(cupdate_ts2 == ((1 << cupdate_ts2_bits) - 1));
	if (c->nodes[0]->bcast.index != 0)
		assert(c->nodes[0]->bcast.timestamp % ((1 << nannounce_ts1_bits) - 1)
		       == nannounce_ts1);
	else
		assert(nannounce_ts1 == ((1 << nannounce_ts1_bits) - 1));
	if (c->nodes[1]->bcast.index != 0)
		assert(c->nodes[1]->bcast.timestamp % ((1 << nannounce_ts2_bits) - 1)
		       == nannounce_ts2);
	else
		assert(nannounce_ts2 == ((1 << nannounce_ts2_bits) - 1));
}

static bool print_sketchval(struct routing_state *rstate,
			    u32 blockheight,
			    const struct node_id *n1,
			    const struct node_id *n2,
			    const char *scidstr)
{
	struct short_channel_id scid;
	u64 val;
	struct chan *c;

	rstate->current_blockheight = blockheight;
	if (!short_channel_id_from_str(scidstr, strlen(scidstr), &scid))
		abort();
	c = new_chan(rstate, &scid, n1, n2, AMOUNT_SAT(1000));
	c->half[0].bcast.index = 1;
	c->half[0].bcast.timestamp = 1574216405;
	c->nodes[0]->bcast.index = 1;
	c->nodes[0]->bcast.timestamp = 1574216526;
	c->half[1].bcast.index = 1;
	c->half[1].bcast.timestamp = 1574216665;
	c->nodes[1]->bcast.index = 1;
	c->nodes[1]->bcast.timestamp = 1574216695;
	val = sketchval_encode(rstate->current_blockheight, c);
	if (val != 0) {
		printf("%s: 0x%16"PRIx64"\n", scidstr, val);
		check_encode(rstate, c);
	}
	return val != 0;
}

int main(void)
{
	struct routing_state *rstate;
	u8 *ser;
	struct short_channel_id scid;
	struct chan *c;
	struct privkey privkeyn1, privkeyn2;
	struct node_id n1, n2;

	setup_locale();
	secp256k1_ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY
						 | SECP256K1_CONTEXT_SIGN);
	setup_tmpctx();

	rstate = new_routing_state(tmpctx, talz(tmpctx, struct node_id),
				   NULL, NULL, NULL, false, false);
	/* A serialized empty sketch (size 10, seed 0) */
	minisketch_set_seed(rstate->minisketch, 0);

	ser = tal_arr(rstate, u8, minisketch_serialized_size(rstate->minisketch));
	minisketch_serialize(rstate->minisketch, ser);
	printf("%s\n", tal_hex(tmpctx, ser));

	/* The encoding for a channel_announcement for 100x2x1 with no
	 * channel updates or node updates.
	 */
	memset(&privkeyn1, 'a', sizeof(privkeyn1));
	memset(&privkeyn2, 'b', sizeof(privkeyn2));
	node_id_from_privkey(&privkeyn1, &n1);
	node_id_from_privkey(&privkeyn2, &n2);

	rstate->current_blockheight = 100;
	if (!short_channel_id_from_str("100x2x1", strlen("100x2x1"), &scid))
		abort();
	c = new_chan(rstate, &scid, &n1, &n2, AMOUNT_SAT(1000));
	printf("0x%16"PRIx64"\n", sketchval_encode(rstate->current_blockheight, c));
	check_encode(rstate, c);

	/* If we have a channel_update from a with timestamp of 1574216405: */
	c->half[0].bcast.index = 1;
	c->half[0].bcast.timestamp = 1574216405;
	printf("0x%16"PRIx64"\n", sketchval_encode(rstate->current_blockheight, c));
	check_encode(rstate, c);

	/* If we have a node_announcement from a, with a timestamp of 1574216526: */
	c->nodes[0]->bcast.index = 1;
	c->nodes[0]->bcast.timestamp = 1574216526;
	printf("0x%16"PRIx64"\n", sketchval_encode(rstate->current_blockheight, c));
	check_encode(rstate, c);

	/* If we have a channel_update from b with a timestamp of 1574216665: */
	c->half[1].bcast.index = 1;
	c->half[1].bcast.timestamp = 1574216665;
	printf("0x%16"PRIx64"\n", sketchval_encode(rstate->current_blockheight, c));
	check_encode(rstate, c);

	/* Finally, a node_announcement from b with a timestamp of 1574216692: */
	c->nodes[1]->bcast.index = 1;
	c->nodes[1]->bcast.timestamp = 1574216692;
	printf("0x%16"PRIx64"\n", sketchval_encode(rstate->current_blockheight, c));
	check_encode(rstate, c);

	/* Now we put it in the minisketch. */
	minisketch_add_uint64(rstate->minisketch, sketchval_encode(rstate->current_blockheight, c));
	minisketch_serialize(rstate->minisketch, ser);
	printf("%s\n", tal_hex(tmpctx, ser));

	rstate->current_blockheight = 128;
	printf("0x%16"PRIx64"\n", sketchval_encode(rstate->current_blockheight, c));
	check_encode(rstate, c);

	/* Now, consider variations, with same timestamps */
	print_sketchval(rstate, 128, &n1, &n2, "101x32767x63");
	print_sketchval(rstate, 4194304, &n1, &n2, "4194304x32767x63");
	/* These are long form */
	print_sketchval(rstate, 101, &n1, &n2, "101x32768x63");
	print_sketchval(rstate, 101, &n1, &n2, "101x32767x64");
	print_sketchval(rstate, 101, &n1, &n2, "101x2097151x4095");
	print_sketchval(rstate, 4194303, &n1, &n2, "4194303x2097151x4095");

	/* These must be encoded in raw form. */
	assert(!print_sketchval(rstate, 101, &n1, &n2, "101x2097151x4096"));
	assert(!print_sketchval(rstate, 101, &n1, &n2, "101x2097152x4095"));
	assert(!print_sketchval(rstate, 4194304, &n1, &n2, "4194304x2097152x4095"));
	assert(!print_sketchval(rstate, 4194304, &n1, &n2, "4194304x2097151x4096"));

	tal_free(tmpctx);
	secp256k1_context_destroy(secp256k1_ctx);
	return 0;
}
#else
int main(void)
{
	return 0;
}
#endif /* !EXPERIMENTAL_FEATURES */
