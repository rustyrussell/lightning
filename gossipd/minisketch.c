#include "config.h"
#include <assert.h>
#include <ccan/array_size/array_size.h>
#include <ccan/bitops/bitops.h>
#include <common/status.h>
#include <common/type_to_string.h>
#include <common/wire_error.h>
#include <gossipd/gossipd.h>
#include <gossipd/queries.h>
#include <gossipd/routing.h>
#include <gossipd/seeker.h>
#include <gossipd/minisketch.h>
#include <wire/gen_peer_wire.h>

#if EXPERIMENTAL_FEATURES
#ifndef MINISKETCH_CAPACITY
#define MINISKETCH_CAPACITY 1024
#endif

void init_minisketch(struct routing_state *rstate)
{
	rstate->minisketch = minisketch_create(64, 0, MINISKETCH_CAPACITY);
	rstate->sketch_entries = 0;
	rstate->sketch_cupdate_timestamps = 0;
	rstate->sketch_nannounce_timestamps = 0;
}

void destroy_minisketch(struct routing_state *rstate)
{
	minisketch_destroy(rstate->minisketch);
}

static void apply_bits(u64 *val, size_t *bitoff, u64 v, size_t bits)
{
	/* v must not be greater than bits long */
	assert((v & ~((1ULL << bits)-1)) == 0);
	*val |= (v << *bitoff);
	*bitoff += bits;
}

/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
 *
 * 1. if there is no `channel_update` or `node_announcement`, the encoding is N
 *    all 1 bits.
 * 2. otherwise, the encoding is `timestamp % ((2^N) - 1)`.
 */
static u64 ts_encode(bool defined, u32 timestamp, size_t bits)
{
	if (!defined)
		return (1 << bits) - 1;
	return timestamp % ((1 << bits) - 1);
}

static u64 update_timestamp_trunc(const struct half_chan *hc, size_t bits)
{
	return ts_encode(is_halfchan_defined(hc), hc->bcast.timestamp, bits);
}

static u64 node_timestamp_trunc(const struct node *node, size_t bits)
{
	return ts_encode(node->bcast.index != 0, node->bcast.timestamp, bits);
}

static u64 sketchval_encode(u32 current_blockheight,
			    const struct chan *chan)
{
	u64 val = 0;
	size_t bitoff = 0, n1, n2, n3;
	bool long_form;

	/* FIXME: Regenerate every time blockheight passed a power of 2! */
	if (current_blockheight == 0)
		return 0;

	assert(short_channel_id_blocknum(&chan->scid)
	       <= current_blockheight);

	/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
	 * - If the transaction index is less than 32768 and the output index
	 *   is less than 64:
	 *   - MUST encode it using short-form.
	 * - Otherwise, if the transaction index is less than 2097152 and the
         *   output index is less than 4096, and `block_number` is less than
         *   4194304:
         *   - MUST encode it using long-form.
	 * - Otherwise:
	 *   - MUST add it to the `raw` field.
	 */
	if (short_channel_id_txnum(&chan->scid) < 32768
	    && short_channel_id_outnum(&chan->scid) < 64) {
		/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
		 * 3. For short-form encoding:
		 *    1. The next lowest N2=15 bits encode the transaction index
		 *       within the block.
		 *    2. The next lowest N3=6 bits encode the output index.
		 */
		n2 = 15;
		n3 = 6;
		long_form = false;
	} else if (short_channel_id_txnum(&chan->scid) < 2097152
		   && short_channel_id_outnum(&chan->scid) < 4096
		   && current_blockheight < 4194304) {
		/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
		 * 4. For long-form encoding:
		 *     1. The next lowest N2=21 bits encode the transaction
		 *        index within the block.
		 *     2. The next lowest N3=12 bits encode the output index.
		 */
		n2 = 21;
		n3 = 12;
		long_form = true;
	} else
		/* Can't fit, use raw encoding, not minisketch */
		return 0;

	/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
	 *
	 * 1. The LSB is 0 for short-form, 1 for long-form.
	 */
	apply_bits(&val, &bitoff, long_form, 1);

	/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
	 *
	 * 2. The next lowest N1 bits encode the block height.  N1 is the minimum
	 * number of bits to encode `block_height`, eg. 591617 gives N1 of 20
	 * bits.
	 */
	n1 = bitops_hs32(current_blockheight) + 1;
	apply_bits(&val, &bitoff, short_channel_id_blocknum(&chan->scid), n1);
	apply_bits(&val, &bitoff, short_channel_id_txnum(&chan->scid), n2);
	apply_bits(&val, &bitoff, short_channel_id_outnum(&chan->scid), n3);

	size_t bits;
	/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
	 *
	 * 1. (63 - N1 - N2 - N3 + 3)/4 bits for `node_1`s last `channel_update`.
	 * 2. (63 - N1 - N2 - N3 + 2)/4 bits for `node_2`s last `channel_update`.
	 * 3. (63 - N1 - N2 - N3 + 1)/4 bits for `node_1`s last
	 *    `node_announcement`.
	 * 4. (63 - N1 - N2 - N3)/4 bits for `node_2`s last `node_announcement`.
	 */
	bits = (63 - n1 - n2 - n3 + 3)/4;
	apply_bits(&val, &bitoff,
		   update_timestamp_trunc(&chan->half[0], bits), bits);
	bits = (63 - n1 - n2 - n3 + 2)/4;
	apply_bits(&val, &bitoff,
		   update_timestamp_trunc(&chan->half[1], bits), bits);
	bits = (63 - n1 - n2 - n3 + 1)/4;
	apply_bits(&val, &bitoff,
		   node_timestamp_trunc(chan->nodes[0], bits), bits);
	bits = (63 - n1 - n2 - n3)/4;
	apply_bits(&val, &bitoff,
		   node_timestamp_trunc(chan->nodes[1], bits), bits);
	assert(bitoff == 64);

	return val;
}

static u64 pull_bits(u64 *val, size_t *bitoff, size_t bits)
{
	u64 ret;

	ret = *val;
	ret &= ((u64)1 << bits) - 1;
	*val >>= bits;
	*bitoff += bits;
	return ret;
}

/* Some scids or blockheight are obviously invalid */
static bool sketchval_decode(u32 blockheight,
			     u64 entry,
			     struct short_channel_id *scid,
			     u32 *cupdate_ts1, u32 *cupdate_ts1_bits,
			     u32 *cupdate_ts2, u32 *cupdate_ts2_bits,
			     u32 *nannounce_ts1, u32 *nannounce_ts1_bits,
			     u32 *nannounce_ts2, u32 *nannounce_ts2_bits)
{
	size_t bitoff = 0, n1, n2, n3;
	u32 scid_blk, scid_tx, scid_out;
	bool long_form;

	if (blockheight == 0)
		return false;

	/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
	 *
	 * 1. The LSB is 0 for short-form, 1 for long-form.
	 */
	long_form = pull_bits(&entry, &bitoff, 1);
	if (!long_form) {
		/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
		 * 3. For short-form encoding:
		 *    1. The next lowest N2=15 bits encode the transaction index
		 *       within the block.
		 *    2. The next lowest N3=6 bits encode the output index.
		 */
		n2 = 15;
		n3 = 6;
	} else {
		/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
		 * 4. For long-form encoding:
		 *     1. The next lowest N2=21 bits encode the transaction
		 *        index within the block.
		 *     2. The next lowest N3=12 bits encode the output index.
		 */
		n2 = 21;
		n3 = 12;
	}

	/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
	 *
	 * 2. The next lowest N1 bits encode the block height.  N1 is the minimum
	 * number of bits to encode `block_height`, eg. 591617 gives N1 of 20
	 * bits.
	 */
	n1 = bitops_hs32(blockheight) + 1;
	if (scid) {
		scid_blk = pull_bits(&entry, &bitoff, n1);
		scid_tx = pull_bits(&entry, &bitoff, n2);
		scid_out = pull_bits(&entry, &bitoff, n3);

		/* This is plainly invalid */
		if (scid_blk == 0)
			return false;

		/* This should never fail */
		if (!mk_short_channel_id(scid, scid_blk, scid_tx, scid_out))
			return false;
	} else
		pull_bits(&entry, &bitoff, n1 + n2 + n3);

	/* BOLT-690b2a5fb895e45a3e12c6cf547f0568dca87162 #7:
	 *
	 * 1. (63 - N1 - N2 - N3 + 3)/4 bits for `node_1`s last `channel_update`.
	 * 2. (63 - N1 - N2 - N3 + 2)/4 bits for `node_2`s last `channel_update`.
	 * 3. (63 - N1 - N2 - N3 + 1)/4 bits for `node_1`s last
	 *    `node_announcement`.
	 * 4. (63 - N1 - N2 - N3)/4 bits for `node_2`s last `node_announcement`.
	 */
	*cupdate_ts1_bits = (63 - n1 - n2 - n3 + 3)/4;
	*cupdate_ts1 = pull_bits(&entry, &bitoff, *cupdate_ts1_bits);
	*cupdate_ts2_bits = (63 - n1 - n2 - n3 + 2)/4;
	*cupdate_ts2 = pull_bits(&entry, &bitoff, *cupdate_ts2_bits);
	*nannounce_ts1_bits = (63 - n1 - n2 - n3 + 1)/4;
	*nannounce_ts1 = pull_bits(&entry, &bitoff, *nannounce_ts1_bits);
	*nannounce_ts2_bits = (63 - n1 - n2 - n3)/4;
	*nannounce_ts2 = pull_bits(&entry, &bitoff, *nannounce_ts2_bits);
	assert(bitoff == 64);

	return true;
}

static void sub_sketchval(struct routing_state *rstate, u64 sketchval)
{
	u32 cupdate_ts1, cupdate_ts1_bits,
		cupdate_ts2, cupdate_ts2_bits,
		nannounce_ts1, nannounce_ts1_bits,
		nannounce_ts2, nannounce_ts2_bits;

	if (!sketchval_decode(rstate->current_blockheight, sketchval,
			      NULL,
			      &cupdate_ts1, &cupdate_ts1_bits,
			      &cupdate_ts2, &cupdate_ts2_bits,
			      &nannounce_ts1, &nannounce_ts1_bits,
			      &nannounce_ts2, &nannounce_ts2_bits))
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "Invalid sub sketchval 0x%"PRIx64" block %u",
			      sketchval, rstate->current_blockheight);
	if (cupdate_ts1 != (1 << cupdate_ts1_bits)-1)
		rstate->sketch_cupdate_timestamps--;
	if (cupdate_ts2 != (1 << cupdate_ts2_bits)-1)
		rstate->sketch_cupdate_timestamps--;
	if (nannounce_ts1 != (1 << nannounce_ts1_bits)-1)
		rstate->sketch_nannounce_timestamps--;
	if (nannounce_ts2 != (1 << nannounce_ts2_bits)-1)
		rstate->sketch_nannounce_timestamps--;
	rstate->sketch_entries--;
	minisketch_add_uint64(rstate->minisketch, sketchval);
}

static void add_sketchval(struct routing_state *rstate, u64 sketchval)
{
	u32 cupdate_ts1, cupdate_ts1_bits,
		cupdate_ts2, cupdate_ts2_bits,
		nannounce_ts1, nannounce_ts1_bits,
		nannounce_ts2, nannounce_ts2_bits;

	if (!sketchval_decode(rstate->current_blockheight, sketchval,
			      NULL,
			      &cupdate_ts1, &cupdate_ts1_bits,
			      &cupdate_ts2, &cupdate_ts2_bits,
			      &nannounce_ts1, &nannounce_ts1_bits,
			      &nannounce_ts2, &nannounce_ts2_bits))
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "Invalid add sketchval 0x%"PRIx64" block %u",
			      sketchval, rstate->current_blockheight);
	if (cupdate_ts1 != (1 << cupdate_ts1_bits)-1)
		rstate->sketch_cupdate_timestamps++;
	if (cupdate_ts2 != (1 << cupdate_ts2_bits)-1)
		rstate->sketch_cupdate_timestamps++;
	if (nannounce_ts1 != (1 << nannounce_ts1_bits)-1)
		rstate->sketch_nannounce_timestamps++;
	if (nannounce_ts2 != (1 << nannounce_ts2_bits)-1)
		rstate->sketch_nannounce_timestamps++;
	rstate->sketch_entries++;
	minisketch_add_uint64(rstate->minisketch, sketchval);
}

/* Insert in sketch for the first time */
void add_to_sketch(struct routing_state *rstate, struct chan *chan)
{
	assert(is_chan_public(chan));
	assert(chan->sketch_val == 0);

	/* This can happen on load from store: future short_channel_id */
	if (short_channel_id_blocknum(&chan->scid) > rstate->current_blockheight)
		return;

	chan->sketch_val = sketchval_encode(rstate->current_blockheight, chan);
	if (chan->sketch_val != 0)
		add_sketchval(rstate, chan->sketch_val);
}

/* Update sketch if it's in already. */
void update_sketch(struct routing_state *rstate, struct chan *chan)
{
	assert(is_chan_public(chan));
	if (chan->sketch_val == 0)
		return;
	sub_sketchval(rstate, chan->sketch_val);
	chan->sketch_val = sketchval_encode(rstate->current_blockheight, chan);
	assert(chan->sketch_val);
	add_sketchval(rstate, chan->sketch_val);
}

void remove_from_sketch(struct routing_state *rstate, struct chan *chan)
{
	if (chan->sketch_val != 0)
		sub_sketchval(rstate, chan->sketch_val);
}

/* When we get a new node_announcement, all of its channels'
 * sketch_vals change */
void update_node_sketches(struct routing_state *rstate, const struct node *node)
{
	struct chan_map_iter i;
	struct chan *chan;

	for (chan = first_chan(node, &i); chan; chan = next_chan(node, &i)) {
		if (is_chan_public(chan))
			update_sketch(rstate, chan);
	}
}

static void check_minisketch(struct routing_state *rstate, u32 prev_blockheight)
{
	u32 entries = 0, cupdate = 0, nannounce = 0;
	u64 idx;
	ssize_t decode;
	struct minisketch *m = minisketch_create(64, 0, MINISKETCH_CAPACITY);
	u64 leftover[MINISKETCH_CAPACITY];

	/* Now iterate through all channels and re-add them */
	for (struct chan *chan = uintmap_first(&rstate->chanmap, &idx);
	     chan;
	     chan = uintmap_after(&rstate->chanmap, &idx)) {
		if (!is_chan_public(chan))
			continue;

		u64 sketch_val = sketchval_encode(prev_blockheight, chan);
		if (!sketch_val)
			continue;

		entries++;
		if (is_halfchan_defined(&chan->half[0]))
			cupdate++;
		if (is_halfchan_defined(&chan->half[1]))
			cupdate++;
		if (chan->nodes[0]->bcast.index != 0)
			nannounce++;
		if (chan->nodes[1]->bcast.index != 0)
			nannounce++;
		minisketch_add_uint64(m, sketch_val);
	}

	if (minisketch_merge(m, rstate->minisketch) != MINISKETCH_CAPACITY)
		abort();

	decode = minisketch_decode(m, ARRAY_SIZE(leftover), leftover);
	assert(decode >= 0);
	for (size_t i = 0; i < decode; i++) {
		struct short_channel_id scid;
		u32 cupdate_ts1, cupdate_ts1_bits,
			cupdate_ts2, cupdate_ts2_bits,
			nannounce_ts1, nannounce_ts1_bits,
			nannounce_ts2, nannounce_ts2_bits;

		sketchval_decode(rstate->current_blockheight, leftover[i],
				 &scid,
				 &cupdate_ts1, &cupdate_ts1_bits,
				 &cupdate_ts2, &cupdate_ts2_bits,
				 &nannounce_ts1, &nannounce_ts1_bits,
				 &nannounce_ts2, &nannounce_ts2_bits);

		status_broken("minisketch %s: %s",
			      type_to_string(tmpctx, struct short_channel_id,
					     &scid),
			      get_channel(rstate, &scid)
			      ? "not in sketch" : "is in sketch");
	}
	assert(decode == 0);
	assert(entries == rstate->sketch_entries);
	assert(cupdate == rstate->sketch_cupdate_timestamps);
	assert(nannounce == rstate->sketch_nannounce_timestamps);

	minisketch_destroy(m);
}

/* This is slow, but nobody cares. */
void minisketch_blockheight_changed(struct routing_state *rstate,
				    u32 prev_blockheight)
{
	u64 idx;

	check_minisketch(rstate, prev_blockheight);

	/* We need to regen if number of bits for blockheight changes */
	if (prev_blockheight
	    && (bitops_hs32(prev_blockheight)
		== bitops_hs32(rstate->current_blockheight)))
		return;

	/* Throw away old minisketch state */
	minisketch_destroy(rstate->minisketch);
	init_minisketch(rstate);

	/* Now iterate through all channels and re-add them */
	for (struct chan *chan = uintmap_first(&rstate->chanmap, &idx);
	     chan;
	     chan = uintmap_after(&rstate->chanmap, &idx)) {
		if (is_chan_public(chan)) {
			chan->sketch_val = 0;
			add_to_sketch(rstate, chan);
		}
	}
}

const u8 *handle_gossip_set(struct peer *peer, const u8 *gossip_set)
{
	struct routing_state *rstate = peer->daemon->rstate;
	struct bitcoin_blkid chain_hash;
	u32 number_of_channels, number_of_channel_updates, number_of_node_announcements, block_number;
	u8 *sketch;
	struct raw_scid_info *raw;
	struct short_channel_id *excluded;
	struct minisketch *m;
	size_t capacity;
	ssize_t num_left;
	u64 leftover[MINISKETCH_CAPACITY];
	struct node_id *nodes = tal_arr(tmpctx, struct node_id, 0);

	if (!fromwire_gossip_set(tmpctx, gossip_set, &chain_hash,
				 &number_of_channels,
				 &number_of_channel_updates,
				 &number_of_node_announcements,
				 &block_number,
				 &sketch,
				 &raw, &excluded)) {
		return towire_errorfmt(peer, NULL, "bad gossip_set msg %s",
				       tal_hex(tmpctx, gossip_set));
	}

	capacity = tal_bytelen(sketch) / sizeof(u64);
	if (tal_bytelen(sketch) % sizeof(u64) != 0 || capacity == 0)
		return towire_errorfmt(peer, NULL, "bad sketch length %zu",
				       tal_bytelen(sketch));

	/* Incompatible block heights?  We ignore and wait. */
	if (rstate->current_blockheight == 0
	    || bitops_hs32(block_number) != bitops_hs32(rstate->current_blockheight)) {
		status_peer_debug(&peer->id,
				  "minisketch: ignoring block_number %u vs %u",
				  block_number, rstate->current_blockheight);
		return NULL;
	}

	m = minisketch_create(64, 0, capacity);
	minisketch_deserialize(m, sketch);

	/* We want to subtract excluded scids: this is equivalent to adding them
	 * to their set. */
	for (size_t i = 0; i < tal_count(excluded); i++) {
		struct chan *c = get_channel(rstate, &excluded[i]);
		if (c && c->sketch_val)
			minisketch_add_uint64(m, c->sketch_val);
	}

	minisketch_merge(m, rstate->minisketch);
	num_left = minisketch_decode(m, ARRAY_SIZE(leftover), leftover);
	status_peer_debug(&peer->id,
			  "minisketch: %zi left merging capacity %zu excl %zu",
			  num_left, capacity, tal_count(excluded));

	/* FIXME: Do something! */
	if (num_left < 0)
		goto out;

	for (size_t i = 0; i < num_left; i++) {
		struct short_channel_id scid;
		u32 cu_ts[2], cu_ts_bits[2], na_ts[2], na_ts_bits[2];
		struct chan *c;

		sketchval_decode(rstate->current_blockheight, leftover[i],
				 &scid,
				 &cu_ts[0], &cu_ts_bits[0],
				 &cu_ts[1], &cu_ts_bits[1],
				 &na_ts[0], &na_ts_bits[0],
				 &na_ts[1], &na_ts_bits[1]);

		c = get_channel(rstate, &scid);
		if (!c)
			add_unknown_scid(peer->daemon->seeker, &scid, peer);
		else {
			/* FIXME: Should we query this anyway to disguise the
			 * fact that we know of it? */
			if (!is_chan_public(c))
				continue;

			/* This will be in set *twice*: once with our timestamps,
			 * once with theirs. */
			if (leftover[i] == c->sketch_val)
				continue;

			/* FIXME: Only send to them if it's changed since their
			 * last sketch! */
			for (i = 0; i < 2; i++) {
				if (update_timestamp_trunc(&c->half[i],
							   cu_ts_bits[i])
				    != cu_ts[i])
					queue_peer_from_store(peer,
							      &c->half[i].bcast);
				if (node_timestamp_trunc(c->nodes[i],
							 na_ts_bits[i])
				    != na_ts[i])
					tal_arr_expand(&nodes, c->nodes[i]->id);
			}
		}
	}

	uniquify_node_ids(&nodes);
	for (size_t i = 0; i < tal_count(nodes); i++) {
		struct node *n = get_node(rstate, &nodes[i]);
		if (n && n->bcast.index)
			queue_peer_from_store(peer, &n->bcast);
	}

out:
	minisketch_destroy(m);
	return NULL;
}
#endif /* EXPERIMENTAL_FEATURES */
