#include "../routing.c"
#include "../gossip_store.c"
#include "../broadcast.c"
#include <stdio.h>

void status_fmt(enum log_level level UNUSED, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_channel_announcement */
bool fromwire_channel_announcement(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, secp256k1_ecdsa_signature *node_signature_1 UNNEEDED, secp256k1_ecdsa_signature *node_signature_2 UNNEEDED, secp256k1_ecdsa_signature *bitcoin_signature_1 UNNEEDED, secp256k1_ecdsa_signature *bitcoin_signature_2 UNNEEDED, u8 **features UNNEEDED, struct bitcoin_blkid *chain_hash UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, struct node_id *node_id_1 UNNEEDED, struct node_id *node_id_2 UNNEEDED, struct pubkey *bitcoin_key_1 UNNEEDED, struct pubkey *bitcoin_key_2 UNNEEDED)
{ fprintf(stderr, "fromwire_channel_announcement called!\n"); abort(); }
/* Generated stub for fromwire_channel_update */
bool fromwire_channel_update(const void *p UNNEEDED, secp256k1_ecdsa_signature *signature UNNEEDED, struct bitcoin_blkid *chain_hash UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, u32 *timestamp UNNEEDED, u8 *message_flags UNNEEDED, u8 *channel_flags UNNEEDED, u16 *cltv_expiry_delta UNNEEDED, struct amount_msat *htlc_minimum_msat UNNEEDED, u32 *fee_base_msat UNNEEDED, u32 *fee_proportional_millionths UNNEEDED)
{ fprintf(stderr, "fromwire_channel_update called!\n"); abort(); }
/* Generated stub for fromwire_channel_update_option_channel_htlc_max */
bool fromwire_channel_update_option_channel_htlc_max(const void *p UNNEEDED, secp256k1_ecdsa_signature *signature UNNEEDED, struct bitcoin_blkid *chain_hash UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, u32 *timestamp UNNEEDED, u8 *message_flags UNNEEDED, u8 *channel_flags UNNEEDED, u16 *cltv_expiry_delta UNNEEDED, struct amount_msat *htlc_minimum_msat UNNEEDED, u32 *fee_base_msat UNNEEDED, u32 *fee_proportional_millionths UNNEEDED, struct amount_msat *htlc_maximum_msat UNNEEDED)
{ fprintf(stderr, "fromwire_channel_update_option_channel_htlc_max called!\n"); abort(); }
/* Generated stub for fromwire_gossipd_local_add_channel */
bool fromwire_gossipd_local_add_channel(const void *p UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, struct node_id *remote_node_id UNNEEDED, struct amount_sat *satoshis UNNEEDED)
{ fprintf(stderr, "fromwire_gossipd_local_add_channel called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_channel_announcement */
bool fromwire_gossip_store_channel_announcement(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, u8 **announcement UNNEEDED, struct amount_sat *satoshis UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_channel_announcement called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_channel_delete */
bool fromwire_gossip_store_channel_delete(const void *p UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_channel_delete called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_channel_update */
bool fromwire_gossip_store_channel_update(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, u8 **update UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_channel_update called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_local_add_channel */
bool fromwire_gossip_store_local_add_channel(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, u8 **local_add UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_local_add_channel called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_node_announcement */
bool fromwire_gossip_store_node_announcement(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, u8 **announcement UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_node_announcement called!\n"); abort(); }
/* Generated stub for fromwire_node_announcement */
bool fromwire_node_announcement(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, secp256k1_ecdsa_signature *signature UNNEEDED, u8 **features UNNEEDED, u32 *timestamp UNNEEDED, struct node_id *node_id UNNEEDED, u8 rgb_color[3] UNNEEDED, u8 alias[32] UNNEEDED, u8 **addresses UNNEEDED)
{ fprintf(stderr, "fromwire_node_announcement called!\n"); abort(); }
/* Generated stub for fromwire_peektype */
int fromwire_peektype(const u8 *cursor UNNEEDED)
{ fprintf(stderr, "fromwire_peektype called!\n"); abort(); }
/* Generated stub for fromwire_wireaddr */
bool fromwire_wireaddr(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct wireaddr *addr UNNEEDED)
{ fprintf(stderr, "fromwire_wireaddr called!\n"); abort(); }
/* Generated stub for onion_type_name */
const char *onion_type_name(int e UNNEEDED)
{ fprintf(stderr, "onion_type_name called!\n"); abort(); }
/* Generated stub for sanitize_error */
char *sanitize_error(const tal_t *ctx UNNEEDED, const u8 *errmsg UNNEEDED,
		     struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "sanitize_error called!\n"); abort(); }
/* Generated stub for status_failed */
void status_failed(enum status_failreason code UNNEEDED,
		   const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "status_failed called!\n"); abort(); }
/* Generated stub for towire_errorfmt */
u8 *towire_errorfmt(const tal_t *ctx UNNEEDED,
		    const struct channel_id *channel UNNEEDED,
		    const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "towire_errorfmt called!\n"); abort(); }
/* Generated stub for towire_gossipd_local_add_channel */
u8 *towire_gossipd_local_add_channel(const tal_t *ctx UNNEEDED, const struct short_channel_id *short_channel_id UNNEEDED, const struct node_id *remote_node_id UNNEEDED, struct amount_sat satoshis UNNEEDED)
{ fprintf(stderr, "towire_gossipd_local_add_channel called!\n"); abort(); }
/* Generated stub for towire_gossip_store_channel_announcement */
u8 *towire_gossip_store_channel_announcement(const tal_t *ctx UNNEEDED, const u8 *announcement UNNEEDED, struct amount_sat satoshis UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_channel_announcement called!\n"); abort(); }
/* Generated stub for towire_gossip_store_channel_delete */
u8 *towire_gossip_store_channel_delete(const tal_t *ctx UNNEEDED, const struct short_channel_id *short_channel_id UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_channel_delete called!\n"); abort(); }
/* Generated stub for towire_gossip_store_channel_update */
u8 *towire_gossip_store_channel_update(const tal_t *ctx UNNEEDED, const u8 *update UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_channel_update called!\n"); abort(); }
/* Generated stub for towire_gossip_store_local_add_channel */
u8 *towire_gossip_store_local_add_channel(const tal_t *ctx UNNEEDED, const u8 *local_add UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_local_add_channel called!\n"); abort(); }
/* Generated stub for towire_gossip_store_node_announcement */
u8 *towire_gossip_store_node_announcement(const tal_t *ctx UNNEEDED, const u8 *announcement UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_node_announcement called!\n"); abort(); }
/* Generated stub for update_peers_broadcast_index */
void update_peers_broadcast_index(struct list_head *peers UNNEEDED, u32 offset UNNEEDED)
{ fprintf(stderr, "update_peers_broadcast_index called!\n"); abort(); }
/* Generated stub for wire_type_name */
const char *wire_type_name(int e UNNEEDED)
{ fprintf(stderr, "wire_type_name called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

#if DEVELOPER
/* Generated stub for memleak_remove_htable */
void memleak_remove_htable(struct htable *memtable UNNEEDED, const struct htable *ht UNNEEDED)
{ fprintf(stderr, "memleak_remove_htable called!\n"); abort(); }
/* Generated stub for memleak_remove_intmap_ */
void memleak_remove_intmap_(struct htable *memtable UNNEEDED, const struct intmap *m UNNEEDED)
{ fprintf(stderr, "memleak_remove_intmap_ called!\n"); abort(); }
#endif

static void node_id_from_privkey(const struct privkey *p, struct node_id *id)
{
	struct pubkey k;
	pubkey_from_privkey(p, &k);
	node_id_from_pubkey(id, &k);
}

#define NUM_NODES (ROUTING_MAX_HOPS + 1)

/* We create an arrangement of nodes, each node N connected to N+1 and
 * to node 1.  The cost for each N to N+1 route is 1, for N to 1 is
 * 2^N.  That means it's always cheapest to go the longer route */
int main(void)
{
	setup_locale();

	struct routing_state *rstate;
	struct node_id ids[NUM_NODES];
	struct chan **route;
	struct amount_msat last_fee;

	secp256k1_ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY
						 | SECP256K1_CONTEXT_SIGN);
	setup_tmpctx();

	for (size_t i = 0; i < NUM_NODES; i++) {
		struct privkey tmp;
		memset(&tmp, i+1, sizeof(tmp));
		node_id_from_privkey(&tmp, &ids[i]);
	}
	/* We are node 0 */
	rstate = new_routing_state(tmpctx, NULL, &ids[0], 0, NULL, NULL, NULL);

	for (size_t i = 0; i < NUM_NODES; i++) {
		struct chan *chan;
		struct half_chan *hc;
		struct short_channel_id scid;

		new_node(rstate, &ids[i]);

		if (i == 0)
			continue;
		if (!mk_short_channel_id(&scid, i, i-1, 0))
			abort();
		chan = new_chan(rstate, &scid, &ids[i], &ids[i-1],
				AMOUNT_SAT(1000000));

		hc = &chan->half[node_id_idx(&ids[i-1], &ids[i])];
		hc->bcast.index = 1;
		hc->base_fee = 1;
		hc->proportional_fee = 0;
		hc->delay = 0;
		hc->channel_flags = node_id_idx(&ids[i-1], &ids[i]);
		hc->htlc_minimum = AMOUNT_MSAT(0);
		hc->htlc_maximum = AMOUNT_MSAT(1000000 * 1000);
		SUPERVERBOSE("Joining %s to %s, fee %u",
			     type_to_string(tmpctx, struct node_id, &ids[i-1]),
			     type_to_string(tmpctx, struct node_id, &ids[i]),
			     (int)hc->base_fee);

		if (i <= 2)
			continue;
		if (!mk_short_channel_id(&scid, i, 1, 0))
			abort();
		chan = new_chan(rstate, &scid, &ids[i], &ids[1],
				AMOUNT_SAT(1000000));
		hc = &chan->half[node_id_idx(&ids[1], &ids[i])];
		hc->bcast.index = 1;
		hc->base_fee = 1 << i;
		hc->proportional_fee = 0;
		hc->delay = 0;
		hc->channel_flags = node_id_idx(&ids[1], &ids[i]);
		hc->htlc_minimum = AMOUNT_MSAT(0);
		hc->htlc_maximum = AMOUNT_MSAT(1000000 * 1000);
		SUPERVERBOSE("Joining %s to %s, fee %u",
			     type_to_string(tmpctx, struct node_id, &ids[1]),
			     type_to_string(tmpctx, struct node_id, &ids[i]),
			     (int)hc->base_fee);
	}

	for (size_t i = ROUTING_MAX_HOPS; i > 1; i--) {
		struct amount_msat fee;
		SUPERVERBOSE("%s -> %s:",
			     type_to_string(tmpctx, struct node_id, &ids[0]),
			     type_to_string(tmpctx, struct node_id, &ids[NUM_NODES-1]));

		route = find_route(tmpctx, rstate, &ids[0], &ids[NUM_NODES-1],
				   AMOUNT_MSAT(1000), 0, 0.0, NULL,
				   i, &fee);
		assert(route);
		/* FIXME: dijkstra ignores maximum length requirement! */
		if (only_dijkstra) {
			assert(tal_count(route) == NUM_NODES-1);
			continue;
		}
		assert(tal_count(route) == i);
		if (i != ROUTING_MAX_HOPS)
			assert(amount_msat_greater(fee, last_fee));
		last_fee = fee;
	}

	tal_free(tmpctx);
	secp256k1_context_destroy(secp256k1_ctx);
	return 0;
}
