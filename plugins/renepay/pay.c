#include "config.h"
#include <ccan/array_size/array_size.h>
#include <ccan/cast/cast.h>
#include <ccan/htable/htable_type.h>
#include <ccan/tal/str/str.h>
#include <common/bolt11.h>
#include <common/bolt12_merkle.h>
#include <common/gossmap.h>
#include <common/json_param.h>
#include <common/json_stream.h>
#include <common/memleak.h>
#include <common/pseudorand.h>
#include <common/type_to_string.h>
#include <errno.h>
#include <plugins/libplugin.h>
#include <plugins/renepay/pay.h>
#include <plugins/renepay/debug.h>
#include <plugins/renepay/pay_flow.h>

/* Set in init */
struct pay_plugin *pay_plugin;


static void renepay_cleanup(struct payment *p);

// TODO(eduardo): check if knowledge is updated after payment success
// TODO(eduardo): how do we fail a payment

void paynote(struct payment *p, const char *fmt, ...)
{
	va_list ap;
	const char *str;

	va_start(ap, fmt);
	str = tal_vfmt(p->paynotes, fmt, ap);
	va_end(ap);
	tal_arr_expand(&p->paynotes, str);
	plugin_log(pay_plugin->plugin, LOG_DBG, "%s", str);
}

void amount_msat_accumulate_(struct amount_msat *dst,
			     struct amount_msat src,
			     const char *dstname,
			     const char *srcname)
{
	if (amount_msat_add(dst, *dst, src))
		return;
	plugin_err(pay_plugin->plugin, "Overflow adding %s (%s) into %s (%s)",
		   srcname, type_to_string(tmpctx, struct amount_msat, &src),
		   dstname, type_to_string(tmpctx, struct amount_msat, dst));
}

void amount_msat_reduce_(struct amount_msat *dst,
			 struct amount_msat src,
			 const char *dstname,
			 const char *srcname)
{
	if (amount_msat_sub(dst, *dst, src))
		return;
	plugin_err(pay_plugin->plugin, "Underflow subtracting %s (%s) from %s (%s)",
		   srcname, type_to_string(tmpctx, struct amount_msat, &src),
		   dstname, type_to_string(tmpctx, struct amount_msat, dst));
}


#if DEVELOPER
static void memleak_mark(struct plugin *p, struct htable *memtable)
{
	memleak_scan_obj(memtable, pay_plugin);
	memleak_scan_htable(memtable, &pay_plugin->chan_extra_map->raw);
}
#endif

static const char *init(struct plugin *p,
			const char *buf UNUSED, const jsmntok_t *config UNUSED)
{
	size_t num_channel_updates_rejected;

	pay_plugin = tal(p, struct pay_plugin);
	pay_plugin->plugin = p;

	rpc_scan(p, "getinfo", take(json_out_obj(NULL, NULL, NULL)),
		 "{id:%}", JSON_SCAN(json_to_node_id, &pay_plugin->my_id));

	rpc_scan(p, "listconfigs",
		 take(json_out_obj(NULL, NULL, NULL)),
		 "{max-locktime-blocks:%,experimental-offers:%}",
		 JSON_SCAN(json_to_number, &pay_plugin->maxdelay_default),
		 JSON_SCAN(json_to_bool, &pay_plugin->exp_offers));

	list_head_init(&pay_plugin->payments);
	
	pay_plugin->chan_extra_map = tal(pay_plugin,struct chan_extra_map);
	chan_extra_map_init(pay_plugin->chan_extra_map);

	pay_plugin->gossmap = gossmap_load(pay_plugin,
					   GOSSIP_STORE_FILENAME,
					   &num_channel_updates_rejected);
	
	if (!pay_plugin->gossmap)
		plugin_err(p, "Could not load gossmap %s: %s",
			   GOSSIP_STORE_FILENAME, strerror(errno));
	if (num_channel_updates_rejected)
		plugin_log(p, LOG_DBG,
			   "gossmap ignored %zu channel updates",
			   num_channel_updates_rejected);
			   
	uncertainty_network_update(pay_plugin->gossmap,
				   pay_plugin->chan_extra_map);
#if DEVELOPER
	plugin_set_memleak_handler(p, memleak_mark);
#endif

	return NULL;
}

/* Create a entry for channel knowledge. */
// static struct chan_extra* chan_knowledge_new(
// 		struct gossmap* gossmap,
// 		struct chan_extra_map *chan_extra_map,
// 		struct short_channel_id scid)
// {
// 	struct chan_extra *chan = chan_extra_map_get(chan_extra_map,scid);
// 	
// 	return chan;
// }
		
// TODO(eduardo): remember that if [a,b] is fixed on (c,dir) then [cap-b,cap-a]
// is fixed on (c,!dir)
// TODO(eduardo): I think this function does too many things.  I would prefer to
// have finer grained control; like a function `chan_update_knowledge` that
// improves the knowledge and also checks the bounds. But we also need to have
// the possiblity to relax the knowledge in another function.
/* We know something about this channel!  Update it! */
// static void chan_update_knowledge(struct payment *p,
// 				 struct short_channel_id scid,
// 				 int dir,
// 				 const struct amount_msat *min_capacity,
// 				 const struct amount_msat *max_capacity)
// {
// 	plugin_log(pay_plugin->plugin,LOG_DBG,"Updating channel: %s [%s, %s]",
// 		type_to_string(tmpctx,struct short_channel_id, &scid),
// 		min_capacity ? type_to_string(tmpctx,struct amount_msat, min_capacity) : "-",
// 		max_capacity ? type_to_string(tmpctx,struct amount_msat, max_capacity) : "-");
// 	
// 	
// 	struct chan_extra_half *h;
// 
// 	/* This is assumed */
// 	if (min_capacity && max_capacity)
// 		assert(amount_msat_greater_eq(*max_capacity, *min_capacity));
// 	
// 	h = get_chan_extra_half_by_scid(pay_plugin->chan_extra_map, scid, dir);
// 	if (!h)
// 		h = new_chan_extra_half(pay_plugin->chan_extra_map, scid, dir,
// 					*max_capacity);
// 	if (min_capacity && amount_msat_greater(*min_capacity, h->known_min))
// 		h->known_min = *min_capacity;
// 	if (max_capacity && amount_msat_less(*max_capacity, h->known_max))
// 		h->known_max = *max_capacity;
// 
// 	/* If we min > max, it means our previous assumptions are wrong
// 	 * (i.e. things changed, or a we assumed full capacity for a routehint
// 	 *  which didn't have it!) */
// 	if (amount_msat_greater(h->known_min, h->known_max)) {
// 		plugin_log(pay_plugin->plugin, LOG_BROKEN,
// 			   "Updated %s capacity %s, now %s-%s! Resetting.",
// 			   type_to_string(tmpctx, struct short_channel_id, &scid),
// 			   min_capacity ? "min" : "max",
// 			   type_to_string(tmpctx, struct amount_msat, &h->known_min),
// 			   type_to_string(tmpctx, struct amount_msat, &h->known_max));
// 
// 		/* OK, assume *old* information is wrong: we can't have
// 		 * just set both, since we assert() those are correct. */
// 		if (min_capacity) {
// 			const struct gossmap_chan *c;
// 			struct amount_sat cap;
// 
// 			/* It might be a local channel; if we don't know better,
// 			 * we reset max to infinite */
// 			c = gossmap_find_chan(pay_plugin->gossmap, &scid);
// 			if (!c
// 			    || !gossmap_chan_get_capacity(pay_plugin->gossmap, c, &cap)
// 			    || !amount_sat_to_msat(&h->known_max, cap))
// 				h->known_max = p->maxspend;
// 			plugin_log(pay_plugin->plugin, LOG_UNUSUAL,
// 				   "... setting max to capacity (%s)",
// 				   type_to_string(tmpctx, struct amount_msat,
// 						  &h->known_max));
// 		} else {
// 			h->known_min = AMOUNT_MSAT(0);
// 			plugin_log(pay_plugin->plugin, LOG_UNUSUAL,
// 				   "... setting min to 0msat");
// 		}
// 	}
// }

static void add_hintchan(struct payment *p,
			  const struct node_id *src,
			  const struct node_id *dst,
			  u16 cltv_expiry_delta,
			  const struct short_channel_id scid,
			  u32 fee_base_msat,
			  u32 fee_proportional_millionths,
			  struct amount_msat amount)
{
	int dir = node_id_cmp(src, dst) < 0 ? 0 : 1;
	
	struct chan_extra *ce = chan_extra_map_get(pay_plugin->chan_extra_map,
						   scid);
	
	if(!ce)
	{
		/* this channel is not public, we don't know his capacity */
		ce = new_chan_extra(pay_plugin->chan_extra_map,
				    scid,
				    MAX_CAP);
		/* FIXME: features? */
		gossmap_local_addchan(p->local_gossmods, src, dst, &scid, NULL);
		gossmap_local_updatechan(p->local_gossmods,
					 &scid,
					 /* We assume any HTLC is allowed */
					 AMOUNT_MSAT(0), MAX_CAP,
					 fee_base_msat, fee_proportional_millionths,
					 cltv_expiry_delta,
					 true,
					 dir);
	}
	
	/* We know (assume!) something about this channel: that it has at
	 * sufficient capacity. */
	chan_extra_can_send(pay_plugin->chan_extra_map,scid,dir,amount);
}

/* Add routehints provided by bolt11 */
static void uncertainty_network_add_routehints(struct payment *p)
{
	struct bolt11 *b11;
	char *fail;

	b11 =
	    bolt11_decode(tmpctx, p->invstr, plugin_feature_set(p->cmd->plugin),
			  p->description, chainparams, &fail);
	if (b11 == NULL)
		plugin_log(pay_plugin->plugin, LOG_BROKEN,
				    "add_routehints: Invalid bolt11: %s", fail);
		

	for (size_t i = 0; i < tal_count(b11->routes); i++) {
		/* Each one, presumably, leads to the destination */
		const struct route_info *r = b11->routes[i];
		const struct node_id *end = & p->dest;
		for (int j = tal_count(r)-1; j >= 0; j--) {
			// TODO(eduardo): amount to send to the add_hintchan
			// should consider fees.
			
			add_hintchan(p, &r[j].pubkey, end,
				      r[j].cltv_expiry_delta,
				      r[j].short_channel_id,
				      r[j].fee_base_msat,
				      r[j].fee_proportional_millionths,
				      p->maxspend);
			end = &r[j].pubkey;
		}
	}
}

/* listpeerchannels gives us the certainty on local channels' capacity.  Of course,
 * this is racy and transient, but better than nothing! */
static bool update_uncertainty_network_from_listpeerchannels(
		struct plugin *plugin,
		struct payment *p,
		const char *buf,
		const jsmntok_t *toks)
{
	const jsmntok_t *channels, *channel;
	size_t i;

	if (json_get_member(buf, toks, "error"))
		goto malformed;

	channels = json_get_member(buf, toks, "channels");
	if (!channels)
		goto malformed;

	json_for_each_arr(i, channel, channels) {
		struct short_channel_id scid;
		const jsmntok_t *scidtok = json_get_member(buf, channel, "short_channel_id");
		/* If channel is still opening, this won't be there. 
		 * Also it won't be in the gossmap, so there is 
		 * no need to mark it as disabled. */
		if (!scidtok)
			continue;
		if (!json_to_short_channel_id(buf, scidtok, &scid))
			goto malformed;
	
		bool connected;
		if(!json_to_bool(buf,
				 json_get_member(buf,channel,"peer_connected"),
				 &connected))
			goto malformed;
	
		if (!connected) {
			paynote(p, "local channel %s disabled:"
				" peer disconnected",
				type_to_string(tmpctx,
					       struct short_channel_id,
					       &scid));
			tal_arr_expand(&p->disabled, scid);
			continue;
		}
		
		const jsmntok_t *spendabletok, *dirtok,*statetok, *totaltok, 
			*peeridtok;
		struct amount_msat spendable,capacity;
		int dir;
		
		const struct node_id src=pay_plugin->my_id;
		struct node_id dst;
		
		spendabletok = json_get_member(buf, channel, "spendable_msat");
		dirtok = json_get_member(buf, channel, "direction");
		statetok = json_get_member(buf, channel, "state");
		totaltok = json_get_member(buf, channel, "total_msat");
		peeridtok = json_get_member(buf,channel,"peer_id");
		
		if(spendabletok==NULL || dirtok==NULL || statetok==NULL || 
		   totaltok==NULL || peeridtok==NULL)
			goto malformed;
		if (!json_to_msat(buf, spendabletok, &spendable))
			goto malformed;
		if (!json_to_msat(buf, totaltok, &capacity))
			goto malformed;
		if (!json_to_int(buf, dirtok,&dir))
			goto malformed;
		if(!json_to_node_id(buf,peeridtok,&dst))
			goto malformed;
			
		/* Don't report opening/closing channels */
		if (!json_tok_streq(buf, statetok, "CHANNELD_NORMAL")) {
			tal_arr_expand(&p->disabled, scid);
			continue;
		}
		
		struct chan_extra *ce = chan_extra_map_get(pay_plugin->chan_extra_map,
							   scid);
			  
		if(!ce)
		{
			/* this channel is not public, but it belongs to us */
			ce = new_chan_extra(pay_plugin->chan_extra_map,
					    scid,
					    capacity);
			/* FIXME: features? */
			gossmap_local_addchan(p->local_gossmods, &src, &dst, &scid, NULL);
			gossmap_local_updatechan(p->local_gossmods,
						 &scid,
						 
						 /* TODO(eduardo): does it
						  * matter to consider HTLC
						  * limits in our own channel? */
						 AMOUNT_MSAT(0),capacity,
						 
						 /* fees = */0,0,
						 
						 /* TODO(eduardo): does it
						  * matter to set this delay? */
						 /*delay=*/0,
						 true,
						 dir);
		}
		
		/* We know min and max liquidity exactly now! */
		chan_extra_set_liquidity(pay_plugin->chan_extra_map,
					 scid,dir,spendable);
	}
	return true;

malformed:
	plugin_log(plugin, LOG_BROKEN,
		   "listpeerchannels malformed: %.*s",
		   json_tok_full_len(toks),
		   json_tok_full(buf, toks));
	return false;
}

/* How much did does this flow deliver to dest? */
static struct amount_msat flow_delivered(const struct pay_flow *flow)
{
	return flow->amounts[tal_count(flow->amounts)-1];
}

/* Mutual recursion */
static struct command_result *try_paying(struct command *cmd,
					 struct payment *p,
					 bool first_time);

static struct command_result *flow_failed(struct command *cmd,
					  const struct pay_flow *flow)
{
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling flow_failed");
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	struct payment *p = flow->payment;

	amount_msat_reduce(&p->total_delivering, flow_delivered(flow));
	amount_msat_reduce(&p->total_sent, flow->amounts[0]);

	/* If nothing outstanding, don't wait for timer! */
	if (amount_msat_eq(p->total_sent, AMOUNT_MSAT(0))) {
		p->rexmit_timer = tal_free(p->rexmit_timer);
		return try_paying(cmd, p, false);
	}

	/* Still waiting for timer... */
	return command_still_pending(cmd);
}

/* Happens when timer goes off, but also works to arm timer if nothing to do */
static void timer_kick(struct payment *p)
{
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	p->rexmit_timer = tal_free(p->rexmit_timer);

	/* Nothing has come back?  Re-arm timer. */
	if (amount_msat_eq(p->total_delivering, p->amount)) {
		// TODO(eduardo): what's the purpose of this?
		// p->rexmit_timer = plugin_timer(p->cmd->plugin,
		// 			       time_from_msec(250),
		// 			       timer_kick, p);
		return;
	}

	try_paying(p->cmd, p, false);
}

static struct command_result *waitsendpay_succeeded(struct command *cmd,
						    const char *buf,
						    const jsmntok_t *result,
						    struct pay_flow *flow)
{
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling waitsendpay_succeeded");
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	struct payment *p = flow->payment;
	struct json_stream *response = jsonrpc_stream_success(cmd);
	const jsmntok_t *preimagetok;

	preimagetok = json_get_member(buf, result, "payment_preimage");
	if (!preimagetok)
		plugin_err(cmd->plugin,
			   "Strange return from waitsendpay: %.*s",
			   json_tok_full_len(result),
			   json_tok_full(buf, result));

	/* Any one succeeding is success. */
	json_add_tok(response, "payment_preimage", preimagetok, buf);
	json_add_sha256(response, "payment_hash", &p->payment_hash);
	json_add_timeabs(response, "created_at", p->start_time);
	json_add_u32(response, "parts", p->next_partid);
	json_add_amount_msat_only(response, "amount_msat",
				  p->total_delivering);
	json_add_amount_msat_only(response, "amount_sent_msat",
				  p->total_sent);
	json_add_string(response, "status", "complete");
	json_add_node_id(response, "destination", &p->dest);
	renepay_cleanup(p);
	return command_finished(cmd, response);
}

static const char *flow_path_to_str(const tal_t *ctx, const struct pay_flow *flow)
{
	char *s = tal_strdup(ctx, "");
	for (size_t i = 0; i < tal_count(flow->path_scids); i++) {
		tal_append_fmt(&s, "-%s->",
			       type_to_string(tmpctx, struct short_channel_id,
					      &flow->path_scids[i]));
	}
	return s;
}

/* Sometimes we don't know exactly who to blame... */
static struct command_result *handle_unhandleable_error(struct payment *p,
							const struct pay_flow *flow,
							const char *what)
{
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	size_t n = tal_count(flow);

	/* We got a mangled reply.  We don't know who to penalize! */
	paynote(p, "%s on route %s", what, flow_path_to_str(tmpctx, flow));
	plugin_log(pay_plugin->plugin, LOG_BROKEN,
		   "%s on route %s",
		   what, flow_path_to_str(tmpctx, flow));

	if (n == 1)
		return command_fail(p->cmd, PAY_UNPARSEABLE_ONION,
				    "Got %s from the destination", what);

	/* FIXME: check chan_extra_map, since we might have succeeded though
	 * this node before? */

	/* Prefer a node not directly connected to either end. */
	if (n > 3) {
		/* us ->0-> ourpeer ->1-> rando ->2-> theirpeer ->3-> dest */
		n = 1 + pseudorand(n - 2);
	} else
		/* Assume it's not the destination */
		n = pseudorand(n-1);

	tal_arr_expand(&p->disabled, flow->path_scids[n]);
	paynote(p, "... eliminated %s",
		type_to_string(tmpctx, struct short_channel_id,
			       &flow->path_scids[n]));
	return NULL;
}

/* We hold onto the flow (and delete the timer) while we're waiting for
 * gossipd to receive the channel_update we got from the error. */
struct addgossip {
	struct short_channel_id scid;
	const struct pay_flow *flow;
};

static struct command_result *addgossip_done(struct command *cmd,
					     const char *buf,
					     const jsmntok_t *err,
					     struct addgossip *adg)
{
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	struct payment *p = adg->flow->payment;

	/* Release this: if it's the last flow we'll retry immediately */
	tal_free(adg);
	timer_kick(p);

	return command_still_pending(cmd);
}

static struct command_result *addgossip_failure(struct command *cmd,
						const char *buf,
						const jsmntok_t *err,
						struct addgossip *adg)

{
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	struct payment *p = adg->flow->payment;

	paynote(p, "addgossip failed, removing channel %s (%.*s)",
		type_to_string(tmpctx, struct short_channel_id, &adg->scid),
		err->end - err->start, buf + err->start);
	tal_arr_expand(&p->disabled, adg->scid);

	return addgossip_done(cmd, buf, err, adg);
}

// TODO(eduardo): what is this?
static struct command_result *submit_update(struct command *cmd,
					    const struct pay_flow *flow,
					    const u8 *update,
					    struct short_channel_id errscid)
{
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling submit_update");
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	struct payment *p = flow->payment;
	struct out_req *req;
	struct addgossip *adg = tal(cmd, struct addgossip);

	/* We need to stash scid in case this fails, and we need to hold flow so
	 * we don't get a rexmit before this is complete. */
	adg->scid = errscid;
	adg->flow = tal_steal(adg, flow);
	/* Disable re-xmit until this returns */
	p->rexmit_timer = tal_free(p->rexmit_timer);

	paynote(p, "... extracted channel_update, telling gossipd");
	plugin_log(pay_plugin->plugin, LOG_DBG, "(update = %s)", tal_hex(tmpctx, update));

	req = jsonrpc_request_start(pay_plugin->plugin, NULL, "addgossip",
				    addgossip_done,
				    addgossip_failure,
				    adg);
	json_add_hex_talarr(req->js, "message", update);
	return send_outreq(pay_plugin->plugin, req);
}

/* Fix up the channel_update to include the type if it doesn't currently have
 * one. See ElementsProject/lightning#1730 and lightningnetwork/lnd#1599 for the
 * in-depth discussion on why we break message parsing here... */
static u8 *patch_channel_update(const tal_t *ctx, u8 *channel_update TAKES)
{
	u8 *fixed;
	if (channel_update != NULL &&
	    fromwire_peektype(channel_update) != WIRE_CHANNEL_UPDATE) {
		/* This should be a channel_update, prefix with the
		 * WIRE_CHANNEL_UPDATE type, but isn't. Let's prefix it. */
		fixed = tal_arr(ctx, u8, 0);
		towire_u16(&fixed, WIRE_CHANNEL_UPDATE);
		towire(&fixed, channel_update, tal_bytelen(channel_update));
		if (taken(channel_update))
			tal_free(channel_update);
		return fixed;
	} else {
		return tal_dup_talarr(ctx, u8, channel_update);
	}
}

/* Return NULL if the wrapped onion error message has no channel_update field,
 * or return the embedded channel_update message otherwise. */
static u8 *channel_update_from_onion_error(const tal_t *ctx,
					   const char *buf,
					   const jsmntok_t *onionmsgtok)
{
	u8 *channel_update = NULL;
	struct amount_msat unused_msat;
	u32 unused32;
	u8 *onion_message = json_tok_bin_from_hex(tmpctx, buf, onionmsgtok);

	/* Identify failcodes that have some channel_update.
	 *
	 * TODO > BOLT 1.0: Add new failcodes when updating to a
	 * new BOLT version. */
	if (!fromwire_temporary_channel_failure(ctx,
						onion_message,
						&channel_update) &&
	    !fromwire_amount_below_minimum(ctx,
					   onion_message, &unused_msat,
					   &channel_update) &&
	    !fromwire_fee_insufficient(ctx,
		    		       onion_message, &unused_msat,
				       &channel_update) &&
	    !fromwire_incorrect_cltv_expiry(ctx,
		    			    onion_message, &unused32,
					    &channel_update) &&
	    !fromwire_expiry_too_soon(ctx,
		    		      onion_message,
				      &channel_update))
		/* No channel update. */
		return NULL;

	return patch_channel_update(ctx, take(channel_update));
}

static struct command_result *waitsendpay_failed(struct command *cmd,
						 const char *buf,
						 const jsmntok_t *err,
						 struct pay_flow *flow)
{
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling waitsendpay_failed");
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	debug_reply(buf,MYLOG);
	
	struct payment *p = flow->payment;
	u64 errcode;
	struct command_result *ret;
	const jsmntok_t *datatok, *msgtok, *errchantok, *erridxtok, *failcodetok, *rawoniontok;
	u32 onionerr, erridx;
	struct short_channel_id errscid;
	const u8 *update;

	if (!json_to_u64(buf, json_get_member(buf, err, "code"), &errcode))
		plugin_err(cmd->plugin, "Bad errcode from waitsendpay: %.*s",
			   json_tok_full_len(err), json_tok_full(buf, err));

	switch (errcode) {
	case PAY_UNPARSEABLE_ONION:
		debug_exec_branch(__LINE__,__PRETTY_FUNCTION__,MYLOG);
		ret = handle_unhandleable_error(p, flow, "unparsable onion reply");
		if (ret)
			return ret;
		goto done;
	case PAY_DESTINATION_PERM_FAIL:
		debug_exec_branch(__LINE__,__PRETTY_FUNCTION__,MYLOG);
		return command_fail(cmd, PAY_DESTINATION_PERM_FAIL,
				    "Got an final failure from destination");
	case PAY_TRY_OTHER_ROUTE:
		debug_exec_branch(__LINE__,__PRETTY_FUNCTION__,MYLOG);
		break;

	default:
		debug_exec_branch(__LINE__,__PRETTY_FUNCTION__,MYLOG);
		plugin_err(cmd->plugin,
			   "Unexpected errcode from waitsendpay: %.*s",
			   json_tok_full_len(err), json_tok_full(buf, err));
	}

	datatok = json_get_member(buf, err, "data");

	/* OK, we expect an onion error reply. */
	erridxtok = json_get_member(buf, datatok, "erring_index");
	if (!erridxtok) {
		ret = handle_unhandleable_error(p, flow, "Missing erring_index");
		if (ret)
			return ret;
		goto done;
	}
	json_to_u32(buf, erridxtok, &erridx);
	errchantok = json_get_member(buf, datatok, "erring_channel");
	json_to_short_channel_id(buf, errchantok, &errscid);
	if (!short_channel_id_eq(&errscid, &flow->path_scids[erridx]))
		plugin_err(pay_plugin->plugin, "Erring channel %u/%zu was %s not %s (path %s)",
			   erridx, tal_count(flow->path_scids),
			   type_to_string(tmpctx, struct short_channel_id, &errscid),
			   type_to_string(tmpctx, struct short_channel_id, &flow->path_scids[erridx]),
			   flow_path_to_str(tmpctx, flow));

	failcodetok = json_get_member(buf, datatok, "failcode");
	json_to_u32(buf, failcodetok, &onionerr);

	msgtok = json_get_member(buf, err, "message");
	rawoniontok = json_get_member(buf, datatok, "raw_message");
	
	// const char *scid_str = type_to_string(tmpctx,struct short_channel_id,&errscid);
	// debug(MYLOG,"failcode: %d, onion error: %s, err_idx: %d, scid: %s\n",
	// 	onionerr,onion_wire_name(onionerr),erridx,scid_str);
	
	paynote(p, "onion error %s from node #%u %s: %.*s",
		onion_wire_name(onionerr),
		erridx,
		type_to_string(tmpctx, struct short_channel_id, &errscid),
		msgtok->end - msgtok->start, buf + msgtok->start);

	/* All these parts succeeded, so we know something about min
	 * capacity! */
	for (size_t i = 0; i < erridx; i++)
	{
		chan_extra_can_send(pay_plugin->chan_extra_map,
				    flow->path_scids[i],
				    flow->path_dirs[i],
				    flow->amounts[i]);
	}
	switch ((enum onion_wire)onionerr) {
	/* These definitely mean eliminate channel */
	case WIRE_PERMANENT_CHANNEL_FAILURE:
	case WIRE_REQUIRED_CHANNEL_FEATURE_MISSING:
	/* FIXME: lnd returns this for disconnected peer, so don't disable perm! */
	case WIRE_UNKNOWN_NEXT_PEER:
	case WIRE_CHANNEL_DISABLED:
	/* These mean node is weird, but we eliminate channel here too */
	case WIRE_INVALID_REALM:
	case WIRE_TEMPORARY_NODE_FAILURE:
	case WIRE_PERMANENT_NODE_FAILURE:
	case WIRE_REQUIRED_NODE_FEATURE_MISSING:
	/* These shouldn't happen, but eliminate channel */
	case WIRE_INVALID_ONION_VERSION:
	case WIRE_INVALID_ONION_HMAC:
	case WIRE_INVALID_ONION_KEY:
	case WIRE_INVALID_ONION_PAYLOAD:
	case WIRE_INVALID_ONION_BLINDING:
	case WIRE_EXPIRY_TOO_FAR:
		paynote(p, "... so we're removing scid");
		tal_arr_expand(&p->disabled, errscid);
		goto done;

	/* These can be fixed (maybe) by applying the included channel_update */
	case WIRE_AMOUNT_BELOW_MINIMUM:
	case WIRE_FEE_INSUFFICIENT:
	case WIRE_INCORRECT_CLTV_EXPIRY:
	case WIRE_EXPIRY_TOO_SOON:
		/* FIXME: Check scid! */
		update = channel_update_from_onion_error(tmpctx, buf, rawoniontok);
		if (update)
			return submit_update(cmd, flow, update, errscid);

		paynote(p, "... missing an update, so we're removing scid");
		tal_arr_expand(&p->disabled, errscid);
		goto done;

	/* Insufficient funds! */
	case WIRE_TEMPORARY_CHANNEL_FAILURE: {
		/* OK new max is amount - 1 */
		struct amount_msat max_possible;
		if (!amount_msat_sub(&max_possible,
				     flow->amounts[erridx],
				     AMOUNT_MSAT(1)))
			max_possible = AMOUNT_MSAT(0);
		paynote(p, "... assuming that max capacity is %s",
			type_to_string(tmpctx, struct amount_msat,
				       &max_possible));
		chan_extra_cannot_send(pay_plugin->chan_extra_map,
				       flow->path_scids[erridx],
				       flow->path_dirs[erridx],
				       flow->amounts[erridx]);
		goto done;
	}

	/* We ignore this, assuming we want to keep going. */
	/* FIXME: We should probably pause until all parts returned, or at least
	 * extend rexmit timer! */
	case WIRE_MPP_TIMEOUT:
		paynote(p, "... will continue");
		goto done;

	/* These are from the final distination: fail */
	case WIRE_INCORRECT_OR_UNKNOWN_PAYMENT_DETAILS:
	case WIRE_FINAL_INCORRECT_CLTV_EXPIRY:
	case WIRE_FINAL_INCORRECT_HTLC_AMOUNT:
		paynote(p, "... fatal");
		return command_fail(cmd, PAY_DESTINATION_PERM_FAIL,
				    "Destination said %s: %.*s",
				    onion_wire_name(onionerr),
				    msgtok->end - msgtok->start,
				    buf + msgtok->start);
	}
	/* Unknown response? */
	if (erridx == tal_count(flow->path_nodes)) {
		paynote(p, "... fatal");
		return command_fail(cmd, PAY_DESTINATION_PERM_FAIL,
				    "Destination gave unknown error code %u: %.*s",
				    onionerr,
				    msgtok->end - msgtok->start,
				    buf + msgtok->start);
	}
	paynote(p, "... eliminating and continuing");
	plugin_log(pay_plugin->plugin, LOG_BROKEN,
		   "Node %s (%u/%zu) gave unknown error code %u",
		   type_to_string(tmpctx, struct node_id,
				  &flow->path_nodes[erridx]),
		   erridx, tal_count(flow->path_nodes),
		   onionerr);
	tal_arr_expand(&p->disabled, errscid);

done:
	return flow_failed(cmd, flow);
}

/* Once we've sent it, we immediate wait for reply. */
static struct command_result *flow_sent(struct command *cmd,
					const char *buf,
					const jsmntok_t *result,
					struct pay_flow *flow)
{
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling flow_sent");
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	struct payment *p = flow->payment;
	struct out_req *req;

	req = jsonrpc_request_start(cmd->plugin, cmd, "waitsendpay",
				    waitsendpay_succeeded,
				    waitsendpay_failed,
				    cast_const(struct pay_flow *, flow));
	json_add_sha256(req->js, "payment_hash", &p->payment_hash);
	json_add_u64(req->js, "groupid", p->groupid);
	json_add_u64(req->js, "partid", flow->partid);
	/* FIXME: We don't set timeout... */

	/* Now flow is owned by this request */
	tal_steal(req, flow);
	return send_outreq(cmd->plugin, req);
}

/* sendpay really only fails immediately in two ways:
 * 1. We screwed up and misused the API.
 * 2. The first peer is disconnected.
 */
static struct command_result *flow_sendpay_failed(struct command *cmd,
						  const char *buf,
						  const jsmntok_t *err,
						  struct pay_flow *flow)
{
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling flow_sendpay_failed");
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	struct payment *p = flow->payment;
	u64 errcode;
	const jsmntok_t *msg = json_get_member(buf, err, "message");

	if (!json_to_u64(buf, json_get_member(buf, err, "code"), &errcode))
		plugin_err(cmd->plugin, "Bad errcode from sendpay: %.*s",
			   json_tok_full_len(err), json_tok_full(buf, err));

	if (errcode != PAY_TRY_OTHER_ROUTE)
		plugin_err(cmd->plugin, "Strange error from sendpay: %.*s",
			   json_tok_full_len(err), json_tok_full(buf, err));

	paynote(p,
		"sendpay didn't like first hop, eliminated: %.*s",
		msg->end - msg->start, buf + msg->start);
	tal_arr_expand(&p->disabled, flow->path_scids[0]);
	return flow_failed(cmd, flow);
}

static struct command_result *
sendpay_flows(struct command *cmd,
	      struct payment *p,
	      struct pay_flow **flows STEALS)
{
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling sendpay_flows");
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	paynote(p, "Sending out batch of %zu payments", tal_count(flows));
	for (size_t i = 0; i < tal_count(flows); i++) {
		struct out_req *req;
		req = jsonrpc_request_start(cmd->plugin, cmd, "sendpay",
					    flow_sent, flow_sendpay_failed,
					    flows[i]);

		json_array_start(req->js, "route");
		for (size_t j = 0; j < tal_count(flows[i]->path_nodes); j++) {
			json_object_start(req->js, NULL);
			json_add_node_id(req->js, "id",
					 &flows[i]->path_nodes[j]);
			json_add_short_channel_id(req->js, "channel",
						  &flows[i]->path_scids[j]);
			json_add_amount_msat_only(req->js, "amount_msat",
						  flows[i]->amounts[j]);
			json_add_num(req->js, "direction",
						  flows[i]->path_dirs[j]);
			json_add_u32(req->js, "delay",
				     flows[i]->cltv_delays[j]);
			json_add_string(req->js,"style","tlv");
			json_object_end(req->js);
		}
		json_array_end(req->js);

		json_add_sha256(req->js, "payment_hash", &p->payment_hash);
		json_add_secret(req->js, "payment_secret", p->payment_secret);
		
		json_add_amount_msat_only(req->js, "amount_msat", p->amount);
			
		json_add_u64(req->js, "partid", flows[i]->partid);
		
		json_add_u64(req->js, "groupid", p->groupid);
		if (p->payment_metadata)
			json_add_hex_talarr(req->js, "payment_metadata",
					    p->payment_metadata);

		/* FIXME: We don't need these three for all payments! */
		if (p->label)
			json_add_string(req->js, "label", p->label);
		json_add_string(req->js, "bolt11", p->invstr);
		if (p->description)
			json_add_string(req->js, "description", p->description);
		
		// TODO(eduardo): remove this line
		debug_outreq(req,MYLOG);
		
		amount_msat_accumulate(&p->total_sent, flows[i]->amounts[0]);
		amount_msat_accumulate(&p->total_delivering,
				       flow_delivered(flows[i]));
		
		/* Flow now owned by request */
		tal_steal(req, flows[i]);
		
		send_outreq(cmd->plugin, req);
	}
	
	/* Get ready to process replies */
	// TODO(eduardo): how is the timer mechanics?
	timer_kick(p);
	
	// TODO(eduardo): what is this actually doing?
	return command_still_pending(cmd);
}

static struct command_result *try_paying(struct command *cmd,
					 struct payment *p,
					 bool first_time)
{
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling try_paying");
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	
	// TODO(eduardo): we should move on only if all rpc requests are done
	// (all threads join)
	
	struct amount_msat feebudget, fees_spent, remaining;
	struct pay_flow **pay_flows;

	if (time_after(time_now(), p->stop_time))
		return command_fail(cmd, PAY_STOPPED_RETRYING, "Timed out");

	/* Total feebudget  */
	if (!amount_msat_sub(&feebudget, p->maxspend, p->amount))
		abort();

	/* Fees spent so far */
	if (!amount_msat_sub(&fees_spent, p->total_sent, p->total_delivering))
		abort();

	/* Remaining fee budget. */
	if (!amount_msat_sub(&feebudget, feebudget, fees_spent))
		abort();

	/* How much are we still trying to send? */
	if (!amount_msat_sub(&remaining, p->amount, p->total_delivering))
		abort();

	/* We let this return an unlikely path, as it's better to try once
	 * than simply refuse.  Plus, models are not truth! */
	pay_flows = get_payflows(p, remaining, feebudget, first_time,
				 amount_msat_eq(p->total_delivering, AMOUNT_MSAT(0)));
	
	// TODO(eduardo): remove this line
	debug_payflows(pay_flows,MYLOG);
	
	if (!pay_flows)
	{
		renepay_cleanup(p);
		return command_fail(cmd, PAY_ROUTE_NOT_FOUND,
				    "Failed to find a route for %s with budget %s",
				    type_to_string(tmpctx, struct amount_msat,
						   &remaining),
				    type_to_string(tmpctx, struct amount_msat,
						   &feebudget));
	}
	/* Now begin making payments */
	return sendpay_flows(cmd, p, pay_flows);
}

static struct command_result *
listpeerchannels_done(struct command *cmd, const char *buf,
	       const jsmntok_t *result, struct payment *p)
{
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	if (!update_uncertainty_network_from_listpeerchannels(cmd->plugin, p, buf, result))
		return command_fail(cmd, LIGHTNINGD,
				    "listpeerchannels malformed: %.*s",
				    json_tok_full_len(result),
				    json_tok_full(buf, result));
	
	// So we have all localmods data, now we apply it. Only once per
	// payment.
	// TODO(eduardo): check that there won't be a prob. cost associated with
	// any gossmap local chan. The same way there aren't fees to pay for my
	// local channels.
	gossmap_apply_localmods(pay_plugin->gossmap,p->local_gossmods);
	return try_paying(cmd, p, true);
}

/* Either the payment succeeded or failed, we need to cleanup/set the plugin
 * into a valid state before the next payment. */
static void renepay_cleanup(struct payment *p)
{
	debug_call(__PRETTY_FUNCTION__,MYLOG);
	/* Always remove our local mods (routehints) so others can use
	 * gossmap. We do this only after the payment completes. */
	gossmap_remove_localmods(pay_plugin->gossmap,
				 p->local_gossmods);
				 
	// TODO(eduardo): can we really free the local_gossmods?
	// I don't thinkg they'll be needed afterwards
	tal_free(p->local_gossmods);
	// TODO(eduardo): Am I missing other things?
	// TODO(eduardo): When do we call this?
}

static void destroy_payment(struct payment *p)
{
	list_del_from(&pay_plugin->payments, &p->list);
}

static struct command_result *json_paystatus(struct command *cmd,
					     const char *buf,
					     const jsmntok_t *params)
{
	const char *invstring;
	struct json_stream *ret;
	struct payment *p;

	if (!param(cmd, buf, params,
		   p_opt("invstring", param_string, &invstring),
		   NULL))
		return command_param_failed();

	ret = jsonrpc_stream_success(cmd);
	json_array_start(ret, "paystatus");

	list_for_each(&pay_plugin->payments, p, list) {
		if (invstring && !streq(invstring, p->invstr))
			continue;

		json_object_start(ret, NULL);
		if (p->label != NULL)
			json_add_string(ret, "label", p->label);

		if (p->invstr)
			json_add_invstring(ret, p->invstr);
		json_add_amount_msat_only(ret, "amount_msat", p->amount);

		json_add_node_id(ret, "destination", &p->dest);
		json_array_start(ret, "notes");
		for (size_t i = 0; i < tal_count(p->paynotes); i++)
			json_add_string(ret, NULL, p->paynotes[i]);
		json_array_end(ret);
		json_object_end(ret);
	}
	json_array_end(ret);

	return command_finished(cmd, ret);
}

static struct command_result *json_pay(struct command *cmd,
				       const char *buf,
				       const jsmntok_t *params)
{
 	struct payment *p;
	
 	u64 invexpiry;
 	struct amount_msat *msat, *invmsat;
	struct amount_msat *maxfee;
	u64 *riskfactor_millionths;
 	u32 *maxdelay;
	u64 *base_fee_penalty, *prob_cost_factor;
	u64 *min_prob_success_millionths;
	unsigned int *retryfor;

#if DEVELOPER
	bool *use_shadow;
#endif

 	p = tal(cmd, struct payment);

	p->cmd = cmd;
	p->paynotes = tal_arr(p, const char *, 0);
	p->disabled = tal_arr(p, struct short_channel_id, 0);
	p->groupid = pseudorand_u64();
	
	// TODO(eduardo): If the partid starts counting from 1 we can do
	// parallel payments MPP. A single payment route is done with partid=0.
	// Can we assume that all payments are MPP, even in the case of an MPP
	// with a single route?
	p->next_partid = 1;
	p->total_sent = AMOUNT_MSAT(0);
	p->total_delivering = AMOUNT_MSAT(0);
	p->rexmit_timer = NULL;
 	p->local_gossmods = gossmap_localmods_new(p);
	
	if (!param(cmd, buf, params,
		   p_req("invstring", param_string, &p->invstr),
 		   p_opt("amount_msat", param_msat, &msat),
 		   p_opt("maxfee", param_msat, &maxfee),
 		   
		   // MCF parameters
		   p_opt_def("base_fee_penalty", param_millionths, &base_fee_penalty,10),
 		   p_opt_def("prob_cost_factor", param_millionths, &prob_cost_factor,10),
 		   
		   // TODO(eduardo): probability of success as a ppm parameter
		   // or a real number?
		   p_opt_def("min_prob_success", param_millionths, 
		   	&min_prob_success_millionths,100000), // default is 10%
	
		   p_opt_def("riskfactor", param_millionths,
			     &riskfactor_millionths, 1.0),

		   p_opt_def("maxdelay", param_number, &maxdelay,
			     /* We're initially called to probe usage, before init! */
			     pay_plugin ? pay_plugin->maxdelay_default : 0),

 		   p_opt_def("retry_for", param_number, &retryfor, 60),
 		   p_opt("localofferid", param_sha256, &p->local_offer_id),
 		   p_opt("description", param_string, &p->description),
 		   p_opt("label", param_string, &p->label),
#if DEVELOPER
		   p_opt_def("use_shadow", param_bool, &use_shadow, true),
#endif
		   NULL))
		return command_param_failed();

#if DEVELOPER
	p->use_shadow = *use_shadow;
	tal_free(use_shadow);
#else
	p->use_shadow = true;
#endif
 	
	
	plugin_log(pay_plugin->plugin,LOG_DBG,"Starting renepay");
	bool gossmap_changed = gossmap_refresh(pay_plugin->gossmap, NULL);
	
	if (pay_plugin->gossmap == NULL)
		plugin_err(pay_plugin->plugin, "Failed to refresh gossmap: %s",
			   strerror(errno));
	
	
	// TODO(eduardo): remove this
	debug_knowledge(pay_plugin->chan_extra_map,MYLOG);
	
	p->base_fee_penalty=*base_fee_penalty;
	p->prob_cost_factor= *prob_cost_factor;
	p->min_prob_success=*min_prob_success_millionths * 1e-6;
 	p->delay_feefactor = *riskfactor_millionths / 1000000.0;
	p->maxdelay = *maxdelay;
	
	tal_free(base_fee_penalty);
	tal_free(prob_cost_factor);
	tal_free(min_prob_success_millionths);
	tal_free(riskfactor_millionths);
	tal_free(maxdelay);

	p->start_time = time_now();
	p->stop_time = timeabs_add(p->start_time, time_from_sec(*retryfor));
	tal_free(retryfor);
	
	bool invstr_is_b11=false;
	if (!bolt12_has_prefix(p->invstr)) {
		struct bolt11 *b11;
		char *fail;

		b11 =
		    bolt11_decode(tmpctx, p->invstr, plugin_feature_set(cmd->plugin),
				  p->description, chainparams, &fail);
		if (b11 == NULL)
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "Invalid bolt11: %s", fail);
		
		invstr_is_b11=true;
		
		invmsat = b11->msat;
		invexpiry = b11->timestamp + b11->expiry;

		p->dest = b11->receiver_id;
		p->payment_hash = b11->payment_hash;
		p->payment_secret =
			tal_dup_or_null(p, struct secret, b11->payment_secret);
		if (b11->metadata)
			p->payment_metadata = tal_dup_talarr(p, u8, b11->metadata);
		else
			p->payment_metadata = NULL;
		
		
		p->final_cltv = b11->min_final_cltv_expiry;
		/* Sanity check */
		if (feature_offered(b11->features, OPT_VAR_ONION) &&
		    !b11->payment_secret)
			return command_fail(
			    cmd, JSONRPC2_INVALID_PARAMS,
			    "Invalid bolt11:"
			    " sets feature var_onion with no secret");

		/* BOLT #11:
		 * A reader:
		 *...
		 * - MUST check that the SHA2 256-bit hash in the `h` field
		 *   exactly matches the hashed description.
		 */
		if (!b11->description) {
			if (!b11->description_hash) {
				return command_fail(cmd,
						    JSONRPC2_INVALID_PARAMS,
						    "Invalid bolt11: missing description");
			}
			if (!p->description)
				return command_fail(cmd,
						    JSONRPC2_INVALID_PARAMS,
						    "bolt11 uses description_hash, but you did not provide description parameter");
		}
	} else {
		// TODO(eduardo): check this, compare with `pay`
		const struct tlv_invoice *b12;
		char *fail;
		b12 = invoice_decode(tmpctx, p->invstr, strlen(p->invstr),
				     plugin_feature_set(cmd->plugin),
				     chainparams, &fail);
		if (b12 == NULL)
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "Invalid bolt12: %s", fail);
		if (!pay_plugin->exp_offers)
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "experimental-offers disabled");

		if (!b12->offer_node_id)
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "invoice missing offer_node_id");
		if (!b12->invoice_payment_hash)
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "invoice missing payment_hash");
		if (!b12->invoice_created_at)
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "invoice missing created_at");
		if (b12->invoice_amount) {
			invmsat = tal(cmd, struct amount_msat);
			*invmsat = amount_msat(*b12->invoice_amount);
		} else
			invmsat = NULL;

		node_id_from_pubkey(&p->dest, b12->offer_node_id);
		p->payment_hash = *b12->invoice_payment_hash;
		if (b12->invreq_recurrence_counter && !p->label)
			return command_fail(
			    cmd, JSONRPC2_INVALID_PARAMS,
			    "recurring invoice requires a label");
		/* FIXME payment_secret should be signature! */
		{
			struct sha256 merkle;

			p->payment_secret = tal(p, struct secret);
			merkle_tlv(b12->fields, &merkle);
			memcpy(p->payment_secret, &merkle, sizeof(merkle));
			BUILD_ASSERT(sizeof(*p->payment_secret) ==
				     sizeof(merkle));
		}
		p->payment_metadata = NULL;
		/* FIXME: blinded paths! */
		p->final_cltv = 18;
		/* BOLT-offers #12:
		 * - if `relative_expiry` is present:
		 *   - MUST reject the invoice if the current time since
		 *     1970-01-01 UTC is greater than `created_at` plus
		 *     `seconds_from_creation`.
		 * - otherwise:
		 *   - MUST reject the invoice if the current time since
		 *     1970-01-01 UTC is greater than `created_at` plus
		 * 7200.
		 */
		if (b12->invoice_relative_expiry)
			invexpiry = *b12->invoice_created_at + *b12->invoice_relative_expiry;
		else
			invexpiry = *b12->invoice_created_at + BOLT12_DEFAULT_REL_EXPIRY;
	}
	
	if (node_id_eq(&pay_plugin->my_id, &p->dest))
		return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
				    "This payment is destined for ourselves. "
				    "Self-payments are not supported");

	/* It's now owned by the global plugin */
	list_add_tail(&pay_plugin->payments, &p->list);
	tal_add_destructor(p, destroy_payment);
	tal_steal(pay_plugin, p);
	
	// set the payment amount
	if (invmsat) {
		// amount is written in the invoice
		if (msat) {
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "amount_msat parameter unnecessary");
		}
		p->amount = *invmsat;
		tal_free(invmsat);
	} else {
		// amount is not written in the invoice
		if (!msat) {
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "amount_msat parameter required");
		}
		p->amount = *msat;
		tal_free(msat);
	}
	
	/* Default max fee is 5 sats, or 0.5%, whichever is *higher* */
	if (!maxfee) {
		struct amount_msat fee = amount_msat_div(p->amount, 200);
		if (amount_msat_less(fee, AMOUNT_MSAT(5000)))
			fee = AMOUNT_MSAT(5000);
		maxfee = tal_dup(tmpctx, struct amount_msat, &fee);
	}

	if (!amount_msat_add(&p->maxspend, p->amount, *maxfee)) {
		return command_fail(
			cmd, JSONRPC2_INVALID_PARAMS,
			"Overflow when computing fee budget, fee far too high.");
	}
	tal_free(maxfee);
	
	if (time_now().ts.tv_sec > invexpiry)
		return command_fail(cmd, PAY_INVOICE_EXPIRED, "Invoice expired");
	
	
	/* To construct the uncertainty network we need to perform the following
	 * steps: 
	 * 1. check that there is a 1-to-1 map between channels in gossmap
	 * and the uncertainty network. We call `uncertainty_network_update`
	 * 
	 * 2. add my local channels that could be private. 
	 * We call `update_uncertainty_network_from_listpeerchannels`.
	 * 
	 * 3. add hidden/private channels listed in the routehints.
	 * We call `uncertainty_network_add_routehints`.
	 * 
	 * 4. check the uncertainty network invariants.
	 * */
	if(gossmap_changed)
		uncertainty_network_update(pay_plugin->gossmap,
					   pay_plugin->chan_extra_map);
	
	
	// TODO(eduardo)
	// we should remove local_gossmods from the uncertainty network
	// after the payment is completed?
	// 
	// TODO(eduardo): are there route hints for B12?
	// Add any extra hidden channel revealed by the routehints to the uncertainty network.
	if(invstr_is_b11)
		uncertainty_network_add_routehints(p);
	
	if(!uncertainty_network_check_invariants(pay_plugin->chan_extra_map))
		plugin_log(pay_plugin->plugin, 
			   LOG_BROKEN,
			   "uncertainty network invariants are violated");
		
	
	struct out_req *req;

	/* Get local capacities... */
	req = jsonrpc_request_start(cmd->plugin, cmd, "listpeerchannels",
				    listpeerchannels_done,
				    listpeerchannels_done, p);
	return send_outreq(cmd->plugin, req);
}

static const struct plugin_command commands[] = {
	{
		"renepaystatus",
		"payment",
		"Detail status of attempts to pay {bolt11}, or all",
		"Covers both old payments and current ones.",
		json_paystatus
	},
	{
		"renepay",
		"payment",
		"Send payment specified by {invstring}",
		"Attempt to pay an invoice.",
		json_pay
	},
};

int main(int argc, char *argv[])
{
	setup_locale();
	plugin_main(argv, init, PLUGIN_RESTARTABLE, true, NULL, commands,
		    ARRAY_SIZE(commands), NULL, 0, NULL, 0, NULL, 0,
		    NULL);
}
