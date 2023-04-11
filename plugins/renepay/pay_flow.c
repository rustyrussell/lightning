/* Routines to get suitable pay_flow array from pay constraints */
#include <common/gossmap.h>
#include <common/type_to_string.h>
#include <common/utils.h>
#include <errno.h>
#include <plugins/libplugin.h>
#include <plugins/renepay/mcf.h>
#include <plugins/renepay/pay.h>
#include <plugins/renepay/pay_flow.h>

/* Calculates delays and converts to scids.  Frees flows.  Caller is responsible
 * for removing resultings flows from the chan_extra_map. */
static struct pay_flow **flows_to_pay_flows(struct payment *payment,
					    struct gossmap *gossmap,
					    struct flow **flows STEALS,
					    u32 final_cltv,
					    u64 *next_partid)
{
	struct pay_flow **pay_flows
		= tal_arr(payment, struct pay_flow *, tal_count(flows));

	for (size_t i = 0; i < tal_count(flows); i++) {
		const struct flow *f = flows[i];
		struct pay_flow *pf = tal(pay_flows, struct pay_flow);
		size_t plen = tal_count(f->path);

		pay_flows[i] = pf;
		pf->payment = payment;
		pf->partid = (*next_partid)++;
		/* Convert gossmap_chan into scids and nodes */
		pf->path_scids = tal_arr(pf, struct short_channel_id, plen);
		pf->path_nodes = tal_arr(pf, struct node_id, plen);
		for (size_t j = 0; j < plen; j++) {
			struct gossmap_node *n;
			n = gossmap_nth_node(gossmap, f->path[j], !f->dirs[j]);
			gossmap_node_get_id(gossmap, n, &pf->path_nodes[j]);
			pf->path_scids[j]
				= gossmap_chan_scid(gossmap, f->path[j]);
		}

		/* Calculate cumulative delays (backwards) */
		pf->cltv_delays = tal_arr(pf, u32, plen);
		pf->cltv_delays[plen-1] = final_cltv;
		for (int j = (int)plen-2; j >= 0; j--) {
			pf->cltv_delays[j] = pf->cltv_delays[j+1]
				+ f->path[j]->half[f->dirs[j]].delay;
		}
		pf->amounts = tal_steal(pf, f->amounts);
		pf->path_dirs = tal_steal(pf, f->dirs);
		pf->success_prob = f->success_prob;
	}
	tal_free(flows);

	return pay_flows;
}

static bitmap *make_disabled_bitmap(const tal_t *ctx,
				    const struct gossmap *gossmap,
				    const struct short_channel_id *scids)
{
	bitmap *disabled
		= tal_arrz(ctx, bitmap,
			   BITMAP_NWORDS(gossmap_max_chan_idx(gossmap)));

	for (size_t i = 0; i < tal_count(scids); i++) {
		struct gossmap_chan *c = gossmap_find_chan(gossmap, &scids[i]);
		if (c)
			bitmap_set_bit(disabled, gossmap_chan_idx(gossmap, c));
	}
	return disabled;
}

static double flows_probability(struct flow **flows)
{
	double prob = 1.0;

	for (size_t i = 0; i < tal_count(flows); i++)
		prob *= flows[i]->success_prob;

	return prob;
}

static struct amount_msat flows_fee(struct flow **flows)
{
	struct amount_msat fee = AMOUNT_MSAT(0);

	for (size_t i = 0; i < tal_count(flows); i++) {
		struct amount_msat this_fee;
		size_t n = tal_count(flows[i]->amounts);

		if (!amount_msat_sub(&this_fee,
				     flows[i]->amounts[0],
				     flows[i]->amounts[n-1]))
			abort();
		amount_msat_accumulate(&fee, this_fee);
	}
	return fee;
}

static u64 flow_delay(const struct flow *flow)
{
	u64 delay = 0;
	for (size_t i = 0; i < tal_count(flow->path); i++)
		delay += flow->path[i]->half[flow->dirs[i]].delay;
	return delay;
}

static u64 flows_worst_delay(struct flow **flows)
{
	u64 maxdelay = 0;
	for (size_t i = 0; i < tal_count(flows); i++) {
		u64 delay = flow_delay(flows[i]);
		if (delay > maxdelay)
			maxdelay = delay;
	}
	return maxdelay;
}

/* FIXME: If only path has channels marked disabled, we should try... */
static bool disable_htlc_violations_oneflow(struct payment *p,
					    const struct flow *flow,
					    const struct gossmap *gossmap,
					    bitmap *disabled)
{
	bool disabled_some = false;

	for (size_t i = 0; i < tal_count(flow->path); i++) {
		const struct half_chan *h = &flow->path[i]->half[flow->dirs[i]];
		struct short_channel_id scid;
		const char *reason;

		if (!h->enabled)
			reason = "channel_update said it was disabled";
		else if (amount_msat_greater_fp16(flow->amounts[i], h->htlc_max))
			reason = "htlc above maximum";
		else if (amount_msat_less_fp16(flow->amounts[i], h->htlc_min))
			reason = "htlc below minimum";
		else
			continue;

		scid = gossmap_chan_scid(gossmap, flow->path[i]);
		paynote(p, "...disabling channel %s: %s",
			type_to_string(tmpctx, struct short_channel_id, &scid),
			reason);

		/* Add this for future searches for this payment. */
		tal_arr_expand(&p->disabled, scid);
		/* Add to existing bitmap */
		bitmap_set_bit(disabled,
			       gossmap_chan_idx(gossmap, flow->path[i]));
		disabled_some = true;
	}
	return disabled_some;
}

/* If we can't use one of these flows because we hit limits, we disable that
 * channel for future searches and return false */
static bool disable_htlc_violations(struct payment *p,
				    struct flow **flows,
				    const struct gossmap *gossmap,
				    bitmap *disabled)
{
	bool disabled_some = false;

	/* We continue through all of them, to disable many at once. */
	for (size_t i = 0; i < tal_count(flows); i++) {
		disabled_some |= disable_htlc_violations_oneflow(p, flows[i],
								 gossmap,
								 disabled);
	}
	return disabled_some;
}

/* We're not using these flows after all: clean up chan_extra_map */
static void remove_flows(const struct gossmap *gossmap,
			 struct chan_extra_map *chan_extra_map,
			 struct flow **flows)
{
	for (size_t i = 0; i < tal_count(flows); i++)
		remove_completed_flow(gossmap, chan_extra_map, flows[i]);
}

/* Get some payment flows to get this amount to destination, or NULL. */
struct pay_flow **get_payflows(struct payment *p,
			       struct amount_msat amount,
			       struct amount_msat feebudget,
			       bool unlikely_ok)
{
	double frugality = 1.0;
	bool was_too_expensive = false;
	bitmap *disabled;
	struct pay_flow **pay_flows;
	const struct gossmap_node *src, *dst;

	if (!gossmap_refresh(pay_plugin->gossmap, NULL))
		plugin_err(pay_plugin->plugin, "Failed to refresh gossmap: %s",
			   strerror(errno));

	gossmap_apply_localmods(pay_plugin->gossmap, p->local_gossmods);
	disabled = make_disabled_bitmap(tmpctx, pay_plugin->gossmap, p->disabled);
	src = gossmap_find_node(pay_plugin->gossmap, &pay_plugin->my_id);
	if (!src) {
		paynote(p, "We don't have any channels?");
		goto fail;
	}
	dst = gossmap_find_node(pay_plugin->gossmap, &p->dest);
	if (!src) {
		paynote(p, "No trace of destination in network gossip");
		goto fail;
	}

	for (;;) {
		struct flow **flows;
		double prob;
		struct amount_msat fee;
		u64 delay;
		bool too_unlikely, too_expensive, too_delayed;

		/* Note!  This actually puts flows in chan_extra_map, so
		 * flows must be removed if not used! */
		flows = minflow(tmpctx, pay_plugin->gossmap, src, dst,
				&pay_plugin->chan_extra_map, disabled,
				amount,
				frugality,
				p->delay_feefactor);
		if (!flows) {
			paynote(p, "Failed to find any paths for %s",
				type_to_string(tmpctx,
					       struct amount_msat,
					       &amount));
			goto fail;
		}

		/* Are we unhappy? */
		prob = flows_probability(flows);
		fee = flows_fee(flows);
		delay = flows_worst_delay(flows) + p->final_cltv;

		too_unlikely = (prob < 0.01);
		if (too_unlikely)
			paynote(p, "Flows too unlikely, P() = %f%%", prob * 100);

		too_expensive = amount_msat_greater(fee, feebudget);
		if (too_expensive)
			paynote(p, "Flows too expensive, fee = %s (max %s)",
				type_to_string(tmpctx, struct amount_msat, &fee),
				type_to_string(tmpctx, struct amount_msat, &feebudget));

		too_delayed = (delay > p->maxdelay);
		if (too_delayed) {
			paynote(p, "Flows too delayed, delay = %"PRIu64" (max %u)",
				delay, p->maxdelay);
			/* FIXME: What is a sane limit? */
			if (p->delay_feefactor > 1000) {
				paynote(p, "Giving up!");
				goto fail_path;
			}

			p->delay_feefactor *= 2;
			paynote(p, "Doubling delay_feefactor to %f",
				p->delay_feefactor);
		}

		/* too expensive vs too unlikely is a tradeoff... */
		if (too_expensive) {
			if (too_unlikely && !unlikely_ok) {
				paynote(p, "Giving up!");
				goto fail_path;
			}

			/* Try increasing frugality? */
			if (frugality >= 32) {
				paynote(p, "Still too expensive, giving up!");
				goto fail_path;
			}
			frugality *= 2;
			was_too_expensive = true;
			paynote(p, "... retry with frugality increased to %f",
				frugality);
			goto retry;
		} else if (too_unlikely) {
			/* Are we bouncing between "too expensive" and
			 * "too unlikely"? */
			if (was_too_expensive) {
				paynote(p, "Bouncing between expensive and unlikely");
				if (!unlikely_ok)
					goto fail_path;
				paynote(p, "... picking unlikely");
				goto seems_ok;
			}

			/* Try decreasing frugality? */
			if (frugality < 0.01) {
				paynote(p, "Still too unlikely, giving up!");
				goto fail_path;
			}
			frugality /= 2;
			paynote(p, "... retry with frugality reduced to %f",
				frugality);
			goto retry;
		}

	seems_ok:
		/* Now we check for min/max htlc violations, and
		 * excessive htlc counts.  It would be more efficient
		 * to do this inside minflow(), but the diagnostics here
		 * are far better, since we can report min/max which
		 * *actually* made us reconsider. */
		if (disable_htlc_violations(p, flows, pay_plugin->gossmap,
					    disabled))
			goto retry;

		/* OK, we are happy with these flows: convert to
		 * pay_flows to outlive the current gossmap. */
		pay_flows = flows_to_pay_flows(p, pay_plugin->gossmap,
					       flows, p->final_cltv,
					       &p->next_partid);
		break;

	retry:
		remove_flows(pay_plugin->gossmap, &pay_plugin->chan_extra_map,
			     flows);
		continue;
	fail_path:
		remove_flows(pay_plugin->gossmap, &pay_plugin->chan_extra_map,
			     flows);
		goto fail;
	}

out:
	/* Always remove our local mods (routehints) so others can use
	 * gossmap */
	gossmap_remove_localmods(pay_plugin->gossmap, p->local_gossmods);
	return pay_flows;

fail:
	pay_flows = NULL;
	goto out;
}

