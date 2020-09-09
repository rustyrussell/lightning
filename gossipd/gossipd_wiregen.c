/* This file was generated by generate-wire.py */
/* Do not modify this file! Modify the .csv file it was generated from. */
/* Original template can be found at tools/gen/impl_template */

#include <gossipd/gossipd_wiregen.h>
#include <assert.h>
#include <ccan/array_size/array_size.h>
#include <ccan/mem/mem.h>
#include <ccan/tal/str/str.h>
#include <common/utils.h>
#include <stdio.h>

#ifndef SUPERVERBOSE
#define SUPERVERBOSE(...)
#endif


const char *gossipd_wire_name(int e)
{
	static char invalidbuf[sizeof("INVALID ") + STR_MAX_CHARS(e)];

	switch ((enum gossipd_wire)e) {
	case WIRE_GOSSIPD_INIT: return "WIRE_GOSSIPD_INIT";
	case WIRE_GOSSIPD_DEV_SET_TIME: return "WIRE_GOSSIPD_DEV_SET_TIME";
	case WIRE_GOSSIPD_GETNODES_REQUEST: return "WIRE_GOSSIPD_GETNODES_REQUEST";
	case WIRE_GOSSIPD_GETNODES_REPLY: return "WIRE_GOSSIPD_GETNODES_REPLY";
	case WIRE_GOSSIPD_GETROUTE_REQUEST: return "WIRE_GOSSIPD_GETROUTE_REQUEST";
	case WIRE_GOSSIPD_GETROUTE_REPLY: return "WIRE_GOSSIPD_GETROUTE_REPLY";
	case WIRE_GOSSIPD_GETCHANNELS_REQUEST: return "WIRE_GOSSIPD_GETCHANNELS_REQUEST";
	case WIRE_GOSSIPD_GETCHANNELS_REPLY: return "WIRE_GOSSIPD_GETCHANNELS_REPLY";
	case WIRE_GOSSIPD_PING: return "WIRE_GOSSIPD_PING";
	case WIRE_GOSSIPD_PING_REPLY: return "WIRE_GOSSIPD_PING_REPLY";
	case WIRE_GOSSIPD_DEV_SET_MAX_SCIDS_ENCODE_SIZE: return "WIRE_GOSSIPD_DEV_SET_MAX_SCIDS_ENCODE_SIZE";
	case WIRE_GOSSIPD_GET_STRIPPED_CUPDATE: return "WIRE_GOSSIPD_GET_STRIPPED_CUPDATE";
	case WIRE_GOSSIPD_GET_STRIPPED_CUPDATE_REPLY: return "WIRE_GOSSIPD_GET_STRIPPED_CUPDATE_REPLY";
	case WIRE_GOSSIPD_LOCAL_CHANNEL_CLOSE: return "WIRE_GOSSIPD_LOCAL_CHANNEL_CLOSE";
	case WIRE_GOSSIPD_GET_TXOUT: return "WIRE_GOSSIPD_GET_TXOUT";
	case WIRE_GOSSIPD_GET_TXOUT_REPLY: return "WIRE_GOSSIPD_GET_TXOUT_REPLY";
	case WIRE_GOSSIPD_PAYMENT_FAILURE: return "WIRE_GOSSIPD_PAYMENT_FAILURE";
	case WIRE_GOSSIPD_OUTPOINT_SPENT: return "WIRE_GOSSIPD_OUTPOINT_SPENT";
	case WIRE_GOSSIPD_DEV_SUPPRESS: return "WIRE_GOSSIPD_DEV_SUPPRESS";
	case WIRE_GOSSIPD_DEV_MEMLEAK: return "WIRE_GOSSIPD_DEV_MEMLEAK";
	case WIRE_GOSSIPD_DEV_MEMLEAK_REPLY: return "WIRE_GOSSIPD_DEV_MEMLEAK_REPLY";
	case WIRE_GOSSIPD_DEV_COMPACT_STORE: return "WIRE_GOSSIPD_DEV_COMPACT_STORE";
	case WIRE_GOSSIPD_DEV_COMPACT_STORE_REPLY: return "WIRE_GOSSIPD_DEV_COMPACT_STORE_REPLY";
	case WIRE_GOSSIPD_GET_INCOMING_CHANNELS: return "WIRE_GOSSIPD_GET_INCOMING_CHANNELS";
	case WIRE_GOSSIPD_GET_INCOMING_CHANNELS_REPLY: return "WIRE_GOSSIPD_GET_INCOMING_CHANNELS_REPLY";
	case WIRE_GOSSIPD_NEW_BLOCKHEIGHT: return "WIRE_GOSSIPD_NEW_BLOCKHEIGHT";
	}

	snprintf(invalidbuf, sizeof(invalidbuf), "INVALID %i", e);
	return invalidbuf;
}

bool gossipd_wire_is_defined(u16 type)
{
	switch ((enum gossipd_wire)type) {
	case WIRE_GOSSIPD_INIT:;
	case WIRE_GOSSIPD_DEV_SET_TIME:;
	case WIRE_GOSSIPD_GETNODES_REQUEST:;
	case WIRE_GOSSIPD_GETNODES_REPLY:;
	case WIRE_GOSSIPD_GETROUTE_REQUEST:;
	case WIRE_GOSSIPD_GETROUTE_REPLY:;
	case WIRE_GOSSIPD_GETCHANNELS_REQUEST:;
	case WIRE_GOSSIPD_GETCHANNELS_REPLY:;
	case WIRE_GOSSIPD_PING:;
	case WIRE_GOSSIPD_PING_REPLY:;
	case WIRE_GOSSIPD_DEV_SET_MAX_SCIDS_ENCODE_SIZE:;
	case WIRE_GOSSIPD_GET_STRIPPED_CUPDATE:;
	case WIRE_GOSSIPD_GET_STRIPPED_CUPDATE_REPLY:;
	case WIRE_GOSSIPD_LOCAL_CHANNEL_CLOSE:;
	case WIRE_GOSSIPD_GET_TXOUT:;
	case WIRE_GOSSIPD_GET_TXOUT_REPLY:;
	case WIRE_GOSSIPD_PAYMENT_FAILURE:;
	case WIRE_GOSSIPD_OUTPOINT_SPENT:;
	case WIRE_GOSSIPD_DEV_SUPPRESS:;
	case WIRE_GOSSIPD_DEV_MEMLEAK:;
	case WIRE_GOSSIPD_DEV_MEMLEAK_REPLY:;
	case WIRE_GOSSIPD_DEV_COMPACT_STORE:;
	case WIRE_GOSSIPD_DEV_COMPACT_STORE_REPLY:;
	case WIRE_GOSSIPD_GET_INCOMING_CHANNELS:;
	case WIRE_GOSSIPD_GET_INCOMING_CHANNELS_REPLY:;
	case WIRE_GOSSIPD_NEW_BLOCKHEIGHT:;
	      return true;
	}
	return false;
}





/* WIRE: GOSSIPD_INIT */
/* Initialize the gossip daemon. */
u8 *towire_gossipd_init(const tal_t *ctx, const struct chainparams *chainparams, const struct feature_set *our_features, const struct node_id *id, const u8 rgb[3], const u8 alias[32], const struct wireaddr *announcable, u32 *dev_gossip_time, bool dev_fast_gossip, bool dev_fast_gossip_prune)
{
	u16 num_announcable = tal_count(announcable);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_INIT);
	towire_chainparams(&p, chainparams);
	towire_feature_set(&p, our_features);
	towire_node_id(&p, id);
	towire_u8_array(&p, rgb, 3);
	towire_u8_array(&p, alias, 32);
	towire_u16(&p, num_announcable);
	for (size_t i = 0; i < num_announcable; i++)
		towire_wireaddr(&p, announcable + i);
	if (!dev_gossip_time)
		towire_bool(&p, false);
	else {
		towire_bool(&p, true);
		towire_u32(&p, *dev_gossip_time);
	}
	towire_bool(&p, dev_fast_gossip);
	towire_bool(&p, dev_fast_gossip_prune);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_init(const tal_t *ctx, const void *p, const struct chainparams **chainparams, struct feature_set **our_features, struct node_id *id, u8 rgb[3], u8 alias[32], struct wireaddr **announcable, u32 **dev_gossip_time, bool *dev_fast_gossip, bool *dev_fast_gossip_prune)
{
	u16 num_announcable;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_INIT)
		return false;
 	fromwire_chainparams(&cursor, &plen, chainparams);
 	*our_features = fromwire_feature_set(ctx, &cursor, &plen);
 	fromwire_node_id(&cursor, &plen, id);
 	fromwire_u8_array(&cursor, &plen, rgb, 3);
 	fromwire_u8_array(&cursor, &plen, alias, 32);
 	num_announcable = fromwire_u16(&cursor, &plen);
 	// 2nd case announcable
	*announcable = num_announcable ? tal_arr(ctx, struct wireaddr, num_announcable) : NULL;
	for (size_t i = 0; i < num_announcable; i++)
		fromwire_wireaddr(&cursor, &plen, *announcable + i);
 	if (!fromwire_bool(&cursor, &plen))
		*dev_gossip_time = NULL;
	else {
		*dev_gossip_time = tal(ctx, u32);
		**dev_gossip_time = fromwire_u32(&cursor, &plen);
	}
 	*dev_fast_gossip = fromwire_bool(&cursor, &plen);
 	*dev_fast_gossip_prune = fromwire_bool(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_DEV_SET_TIME */
/* In developer mode */
u8 *towire_gossipd_dev_set_time(const tal_t *ctx, u32 dev_gossip_time)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_DEV_SET_TIME);
	towire_u32(&p, dev_gossip_time);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_dev_set_time(const void *p, u32 *dev_gossip_time)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_DEV_SET_TIME)
		return false;
 	*dev_gossip_time = fromwire_u32(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GETNODES_REQUEST */
/* Pass JSON-RPC getnodes call through */
u8 *towire_gossipd_getnodes_request(const tal_t *ctx, const struct node_id *id)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GETNODES_REQUEST);
	if (!id)
		towire_bool(&p, false);
	else {
		towire_bool(&p, true);
		towire_node_id(&p, id);
	}

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_getnodes_request(const tal_t *ctx, const void *p, struct node_id **id)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GETNODES_REQUEST)
		return false;
 	if (!fromwire_bool(&cursor, &plen))
		*id = NULL;
	else {
		*id = tal(ctx, struct node_id);
		fromwire_node_id(&cursor, &plen, *id);
	}
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GETNODES_REPLY */
u8 *towire_gossipd_getnodes_reply(const tal_t *ctx, const struct gossip_getnodes_entry **nodes)
{
	u32 num_nodes = tal_count(nodes);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GETNODES_REPLY);
	towire_u32(&p, num_nodes);
	for (size_t i = 0; i < num_nodes; i++)
		towire_gossip_getnodes_entry(&p, nodes[i]);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_getnodes_reply(const tal_t *ctx, const void *p, struct gossip_getnodes_entry ***nodes)
{
	u32 num_nodes;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GETNODES_REPLY)
		return false;
 	num_nodes = fromwire_u32(&cursor, &plen);
 	// 2nd case nodes
	*nodes = num_nodes ? tal_arr(ctx, struct gossip_getnodes_entry *, num_nodes) : NULL;
	for (size_t i = 0; i < num_nodes; i++)
		(*nodes)[i] = fromwire_gossip_getnodes_entry(*nodes, &cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GETROUTE_REQUEST */
/* Pass JSON-RPC getroute call through */
u8 *towire_gossipd_getroute_request(const tal_t *ctx, const struct node_id *source, const struct node_id *destination, struct amount_msat msatoshi, u64 riskfactor_millionths, u32 final_cltv, u64 fuzz_millionths, const struct exclude_entry **excluded, u32 max_hops)
{
	u16 num_excluded = tal_count(excluded);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GETROUTE_REQUEST);
	/* Source defaults to "us" */
	if (!source)
		towire_bool(&p, false);
	else {
		towire_bool(&p, true);
		towire_node_id(&p, source);
	}
	towire_node_id(&p, destination);
	towire_amount_msat(&p, msatoshi);
	towire_u64(&p, riskfactor_millionths);
	towire_u32(&p, final_cltv);
	towire_u64(&p, fuzz_millionths);
	towire_u16(&p, num_excluded);
	for (size_t i = 0; i < num_excluded; i++)
		towire_exclude_entry(&p, excluded[i]);
	towire_u32(&p, max_hops);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_getroute_request(const tal_t *ctx, const void *p, struct node_id **source, struct node_id *destination, struct amount_msat *msatoshi, u64 *riskfactor_millionths, u32 *final_cltv, u64 *fuzz_millionths, struct exclude_entry ***excluded, u32 *max_hops)
{
	u16 num_excluded;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GETROUTE_REQUEST)
		return false;
 	/* Source defaults to "us" */
	if (!fromwire_bool(&cursor, &plen))
		*source = NULL;
	else {
		*source = tal(ctx, struct node_id);
		fromwire_node_id(&cursor, &plen, *source);
	}
 	fromwire_node_id(&cursor, &plen, destination);
 	*msatoshi = fromwire_amount_msat(&cursor, &plen);
 	*riskfactor_millionths = fromwire_u64(&cursor, &plen);
 	*final_cltv = fromwire_u32(&cursor, &plen);
 	*fuzz_millionths = fromwire_u64(&cursor, &plen);
 	num_excluded = fromwire_u16(&cursor, &plen);
 	// 2nd case excluded
	*excluded = num_excluded ? tal_arr(ctx, struct exclude_entry *, num_excluded) : NULL;
	for (size_t i = 0; i < num_excluded; i++)
		(*excluded)[i] = fromwire_exclude_entry(*excluded, &cursor, &plen);
 	*max_hops = fromwire_u32(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GETROUTE_REPLY */
u8 *towire_gossipd_getroute_reply(const tal_t *ctx, const struct route_hop **hops)
{
	u16 num_hops = tal_count(hops);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GETROUTE_REPLY);
	towire_u16(&p, num_hops);
	for (size_t i = 0; i < num_hops; i++)
		towire_route_hop(&p, hops[i]);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_getroute_reply(const tal_t *ctx, const void *p, struct route_hop ***hops)
{
	u16 num_hops;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GETROUTE_REPLY)
		return false;
 	num_hops = fromwire_u16(&cursor, &plen);
 	// 2nd case hops
	*hops = num_hops ? tal_arr(ctx, struct route_hop *, num_hops) : NULL;
	for (size_t i = 0; i < num_hops; i++)
		(*hops)[i] = fromwire_route_hop(*hops, &cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GETCHANNELS_REQUEST */
u8 *towire_gossipd_getchannels_request(const tal_t *ctx, const struct short_channel_id *short_channel_id, const struct node_id *source, const struct short_channel_id *prev)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GETCHANNELS_REQUEST);
	if (!short_channel_id)
		towire_bool(&p, false);
	else {
		towire_bool(&p, true);
		towire_short_channel_id(&p, short_channel_id);
	}
	if (!source)
		towire_bool(&p, false);
	else {
		towire_bool(&p, true);
		towire_node_id(&p, source);
	}
	if (!prev)
		towire_bool(&p, false);
	else {
		towire_bool(&p, true);
		towire_short_channel_id(&p, prev);
	}

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_getchannels_request(const tal_t *ctx, const void *p, struct short_channel_id **short_channel_id, struct node_id **source, struct short_channel_id **prev)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GETCHANNELS_REQUEST)
		return false;
 	if (!fromwire_bool(&cursor, &plen))
		*short_channel_id = NULL;
	else {
		*short_channel_id = tal(ctx, struct short_channel_id);
		fromwire_short_channel_id(&cursor, &plen, *short_channel_id);
	}
 	if (!fromwire_bool(&cursor, &plen))
		*source = NULL;
	else {
		*source = tal(ctx, struct node_id);
		fromwire_node_id(&cursor, &plen, *source);
	}
 	if (!fromwire_bool(&cursor, &plen))
		*prev = NULL;
	else {
		*prev = tal(ctx, struct short_channel_id);
		fromwire_short_channel_id(&cursor, &plen, *prev);
	}
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GETCHANNELS_REPLY */
u8 *towire_gossipd_getchannels_reply(const tal_t *ctx, bool complete, const struct gossip_getchannels_entry **nodes)
{
	u32 num_channels = tal_count(nodes);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GETCHANNELS_REPLY);
	towire_bool(&p, complete);
	towire_u32(&p, num_channels);
	for (size_t i = 0; i < num_channels; i++)
		towire_gossip_getchannels_entry(&p, nodes[i]);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_getchannels_reply(const tal_t *ctx, const void *p, bool *complete, struct gossip_getchannels_entry ***nodes)
{
	u32 num_channels;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GETCHANNELS_REPLY)
		return false;
 	*complete = fromwire_bool(&cursor, &plen);
 	num_channels = fromwire_u32(&cursor, &plen);
 	// 2nd case nodes
	*nodes = num_channels ? tal_arr(ctx, struct gossip_getchannels_entry *, num_channels) : NULL;
	for (size_t i = 0; i < num_channels; i++)
		(*nodes)[i] = fromwire_gossip_getchannels_entry(*nodes, &cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_PING */
/* Ping/pong test.  Waits for a reply if it expects one. */
u8 *towire_gossipd_ping(const tal_t *ctx, const struct node_id *id, u16 num_pong_bytes, u16 len)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_PING);
	towire_node_id(&p, id);
	towire_u16(&p, num_pong_bytes);
	towire_u16(&p, len);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_ping(const void *p, struct node_id *id, u16 *num_pong_bytes, u16 *len)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_PING)
		return false;
 	fromwire_node_id(&cursor, &plen, id);
 	*num_pong_bytes = fromwire_u16(&cursor, &plen);
 	*len = fromwire_u16(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_PING_REPLY */
u8 *towire_gossipd_ping_reply(const tal_t *ctx, const struct node_id *id, bool sent, u16 totlen)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_PING_REPLY);
	towire_node_id(&p, id);
	/* False if id in gossip_ping was unknown. */
	towire_bool(&p, sent);
	/* 0 == no pong expected */
	towire_u16(&p, totlen);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_ping_reply(const void *p, struct node_id *id, bool *sent, u16 *totlen)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_PING_REPLY)
		return false;
 	fromwire_node_id(&cursor, &plen, id);
 	/* False if id in gossip_ping was unknown. */
	*sent = fromwire_bool(&cursor, &plen);
 	/* 0 == no pong expected */
	*totlen = fromwire_u16(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_DEV_SET_MAX_SCIDS_ENCODE_SIZE */
/* Set artificial maximum reply_channel_range size.  Master->gossipd */
u8 *towire_gossipd_dev_set_max_scids_encode_size(const tal_t *ctx, u32 max)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_DEV_SET_MAX_SCIDS_ENCODE_SIZE);
	towire_u32(&p, max);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_dev_set_max_scids_encode_size(const void *p, u32 *max)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_DEV_SET_MAX_SCIDS_ENCODE_SIZE)
		return false;
 	*max = fromwire_u32(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GET_STRIPPED_CUPDATE */
/* Given a short_channel_id */
u8 *towire_gossipd_get_stripped_cupdate(const tal_t *ctx, const struct short_channel_id *channel_id)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GET_STRIPPED_CUPDATE);
	towire_short_channel_id(&p, channel_id);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_get_stripped_cupdate(const void *p, struct short_channel_id *channel_id)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GET_STRIPPED_CUPDATE)
		return false;
 	fromwire_short_channel_id(&cursor, &plen, channel_id);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GET_STRIPPED_CUPDATE_REPLY */
u8 *towire_gossipd_get_stripped_cupdate_reply(const tal_t *ctx, const u8 *stripped_update)
{
	u16 stripped_update_len = tal_count(stripped_update);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GET_STRIPPED_CUPDATE_REPLY);
	towire_u16(&p, stripped_update_len);
	towire_u8_array(&p, stripped_update, stripped_update_len);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_get_stripped_cupdate_reply(const tal_t *ctx, const void *p, u8 **stripped_update)
{
	u16 stripped_update_len;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GET_STRIPPED_CUPDATE_REPLY)
		return false;
 	stripped_update_len = fromwire_u16(&cursor, &plen);
 	// 2nd case stripped_update
	*stripped_update = stripped_update_len ? tal_arr(ctx, u8, stripped_update_len) : NULL;
	fromwire_u8_array(&cursor, &plen, *stripped_update, stripped_update_len);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_LOCAL_CHANNEL_CLOSE */
/* gossipd->master: we're closing this channel. */
u8 *towire_gossipd_local_channel_close(const tal_t *ctx, const struct short_channel_id *short_channel_id)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_LOCAL_CHANNEL_CLOSE);
	towire_short_channel_id(&p, short_channel_id);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_local_channel_close(const void *p, struct short_channel_id *short_channel_id)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_LOCAL_CHANNEL_CLOSE)
		return false;
 	fromwire_short_channel_id(&cursor, &plen, short_channel_id);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GET_TXOUT */
/* Gossipd->master get this tx output please. */
u8 *towire_gossipd_get_txout(const tal_t *ctx, const struct short_channel_id *short_channel_id)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GET_TXOUT);
	towire_short_channel_id(&p, short_channel_id);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_get_txout(const void *p, struct short_channel_id *short_channel_id)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GET_TXOUT)
		return false;
 	fromwire_short_channel_id(&cursor, &plen, short_channel_id);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GET_TXOUT_REPLY */
/* master->gossipd here is the output */
u8 *towire_gossipd_get_txout_reply(const tal_t *ctx, const struct short_channel_id *short_channel_id, struct amount_sat satoshis, const u8 *outscript)
{
	u16 len = tal_count(outscript);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GET_TXOUT_REPLY);
	towire_short_channel_id(&p, short_channel_id);
	towire_amount_sat(&p, satoshis);
	towire_u16(&p, len);
	towire_u8_array(&p, outscript, len);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_get_txout_reply(const tal_t *ctx, const void *p, struct short_channel_id *short_channel_id, struct amount_sat *satoshis, u8 **outscript)
{
	u16 len;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GET_TXOUT_REPLY)
		return false;
 	fromwire_short_channel_id(&cursor, &plen, short_channel_id);
 	*satoshis = fromwire_amount_sat(&cursor, &plen);
 	len = fromwire_u16(&cursor, &plen);
 	// 2nd case outscript
	*outscript = len ? tal_arr(ctx, u8, len) : NULL;
	fromwire_u8_array(&cursor, &plen, *outscript, len);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_PAYMENT_FAILURE */
/* master->gossipd an htlc failed with this onion error. */
u8 *towire_gossipd_payment_failure(const tal_t *ctx, const u8 *error)
{
	u16 len = tal_count(error);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_PAYMENT_FAILURE);
	towire_u16(&p, len);
	towire_u8_array(&p, error, len);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_payment_failure(const tal_t *ctx, const void *p, u8 **error)
{
	u16 len;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_PAYMENT_FAILURE)
		return false;
 	len = fromwire_u16(&cursor, &plen);
 	// 2nd case error
	*error = len ? tal_arr(ctx, u8, len) : NULL;
	fromwire_u8_array(&cursor, &plen, *error, len);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_OUTPOINT_SPENT */
/* master -> gossipd: a potential funding outpoint was spent */
u8 *towire_gossipd_outpoint_spent(const tal_t *ctx, const struct short_channel_id *short_channel_id)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_OUTPOINT_SPENT);
	towire_short_channel_id(&p, short_channel_id);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_outpoint_spent(const void *p, struct short_channel_id *short_channel_id)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_OUTPOINT_SPENT)
		return false;
 	fromwire_short_channel_id(&cursor, &plen, short_channel_id);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_DEV_SUPPRESS */
/* master -> gossipd: stop gossip timers. */
u8 *towire_gossipd_dev_suppress(const tal_t *ctx)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_DEV_SUPPRESS);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_dev_suppress(const void *p)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_DEV_SUPPRESS)
		return false;
	return cursor != NULL;
}

/* WIRE: GOSSIPD_DEV_MEMLEAK */
/* master -> gossipd: do you have a memleak? */
u8 *towire_gossipd_dev_memleak(const tal_t *ctx)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_DEV_MEMLEAK);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_dev_memleak(const void *p)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_DEV_MEMLEAK)
		return false;
	return cursor != NULL;
}

/* WIRE: GOSSIPD_DEV_MEMLEAK_REPLY */
u8 *towire_gossipd_dev_memleak_reply(const tal_t *ctx, bool leak)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_DEV_MEMLEAK_REPLY);
	towire_bool(&p, leak);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_dev_memleak_reply(const void *p, bool *leak)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_DEV_MEMLEAK_REPLY)
		return false;
 	*leak = fromwire_bool(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_DEV_COMPACT_STORE */
/* master -> gossipd: please rewrite the gossip_store */
u8 *towire_gossipd_dev_compact_store(const tal_t *ctx)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_DEV_COMPACT_STORE);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_dev_compact_store(const void *p)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_DEV_COMPACT_STORE)
		return false;
	return cursor != NULL;
}

/* WIRE: GOSSIPD_DEV_COMPACT_STORE_REPLY */
/* gossipd -> master: ok */
u8 *towire_gossipd_dev_compact_store_reply(const tal_t *ctx, bool success)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_DEV_COMPACT_STORE_REPLY);
	towire_bool(&p, success);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_dev_compact_store_reply(const void *p, bool *success)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_DEV_COMPACT_STORE_REPLY)
		return false;
 	*success = fromwire_bool(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GET_INCOMING_CHANNELS */
/* master -> gossipd: get route_info for our incoming channels */
u8 *towire_gossipd_get_incoming_channels(const tal_t *ctx)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GET_INCOMING_CHANNELS);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_get_incoming_channels(const void *p)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GET_INCOMING_CHANNELS)
		return false;
	return cursor != NULL;
}

/* WIRE: GOSSIPD_GET_INCOMING_CHANNELS_REPLY */
/* gossipd -> master: here they are. */
u8 *towire_gossipd_get_incoming_channels_reply(const tal_t *ctx, const struct route_info *public_route_info, const bool *public_deadends, const struct route_info *private_route_info, const bool *private_deadends)
{
	u16 num_public = tal_count(public_deadends);
	u16 num_private = tal_count(private_deadends);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_GET_INCOMING_CHANNELS_REPLY);
	towire_u16(&p, num_public);
	for (size_t i = 0; i < num_public; i++)
		towire_route_info(&p, public_route_info + i);
	for (size_t i = 0; i < num_public; i++)
		towire_bool(&p, public_deadends[i]);
	towire_u16(&p, num_private);
	for (size_t i = 0; i < num_private; i++)
		towire_route_info(&p, private_route_info + i);
	for (size_t i = 0; i < num_private; i++)
		towire_bool(&p, private_deadends[i]);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_get_incoming_channels_reply(const tal_t *ctx, const void *p, struct route_info **public_route_info, bool **public_deadends, struct route_info **private_route_info, bool **private_deadends)
{
	u16 num_public;
	u16 num_private;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_GET_INCOMING_CHANNELS_REPLY)
		return false;
 	num_public = fromwire_u16(&cursor, &plen);
 	// 2nd case public_route_info
	*public_route_info = num_public ? tal_arr(ctx, struct route_info, num_public) : NULL;
	for (size_t i = 0; i < num_public; i++)
		fromwire_route_info(&cursor, &plen, *public_route_info + i);
 	// 2nd case public_deadends
	*public_deadends = num_public ? tal_arr(ctx, bool, num_public) : NULL;
	for (size_t i = 0; i < num_public; i++)
		(*public_deadends)[i] = fromwire_bool(&cursor, &plen);
 	num_private = fromwire_u16(&cursor, &plen);
 	// 2nd case private_route_info
	*private_route_info = num_private ? tal_arr(ctx, struct route_info, num_private) : NULL;
	for (size_t i = 0; i < num_private; i++)
		fromwire_route_info(&cursor, &plen, *private_route_info + i);
 	// 2nd case private_deadends
	*private_deadends = num_private ? tal_arr(ctx, bool, num_private) : NULL;
	for (size_t i = 0; i < num_private; i++)
		(*private_deadends)[i] = fromwire_bool(&cursor, &plen);
	return cursor != NULL;
}

/* WIRE: GOSSIPD_NEW_BLOCKHEIGHT */
/* master -> gossipd: blockheight increased. */
u8 *towire_gossipd_new_blockheight(const tal_t *ctx, u32 blockheight)
{
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_GOSSIPD_NEW_BLOCKHEIGHT);
	towire_u32(&p, blockheight);

	return memcheck(p, tal_count(p));
}
bool fromwire_gossipd_new_blockheight(const void *p, u32 *blockheight)
{
	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_GOSSIPD_NEW_BLOCKHEIGHT)
		return false;
 	*blockheight = fromwire_u32(&cursor, &plen);
	return cursor != NULL;
}
// SHA256STAMP:a494fd8a3f5ae31d113f387445744dd19490149be92d83f9bf61cc952457ad8f
