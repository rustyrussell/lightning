#include "config.h"
#include <assert.h>
#include <common/gossmap.h>
#include <math.h>
#include <plugins/renepay/flow.h>

#ifndef SUPERVERBOSE
#define SUPERVERBOSE(...)
#else
#define SUPERVERBOSE_ENABLED 1
#endif

/* Helper to access the half chan at flow index idx */
const struct half_chan *flow_edge(const struct flow *flow, size_t idx)
{
	assert(idx < tal_count(flow->path));
	return &flow->path[idx]->half[flow->dirs[idx]];
}

/* Assuming a uniform distribution, what is the chance this f gets through?
 *
 * This is the "(c_e + 1 − f_e) / (c_e + 1)" in the paper.
 */
static double edge_probability(const struct capacity_range *cap,
			       struct amount_msat f)
{
	struct amount_msat range_plus_one, numerator;

	/* If f is <= known minimum, probility is 1. */
	if (!amount_msat_sub(&f, f, cap->min))
		return 1.0;

	/* +1 because the number of elements in the range is min-max + 1 */
	if (!amount_msat_sub(&range_plus_one, cap->max, cap->min))
		abort();
	if (!amount_msat_add(&range_plus_one, range_plus_one, AMOUNT_MSAT(1)))
		abort();

	/* If f > capacity, probability is 0 */
	if (!amount_msat_sub(&numerator, range_plus_one, f))
		return 0.0;

	return amount_msat_ratio(numerator, range_plus_one);
}

bool flow_path_eq(const struct gossmap_chan **path1,
		  const int *dirs1,
		  const struct gossmap_chan **path2,
		  const int *dirs2)
{
	if (tal_count(path1) != tal_count(path2))
		return false;
	for (size_t i = 0; i < tal_count(path1); i++) {
		if (path1[i] != path2[i])
			return false;
		if (dirs1[i] != dirs2[i])
			return false;
	}
	return true;
}

/* Add this to the flow. */
void flow_add(struct flow *flow,
	      const struct gossmap *gossmap,
	      const struct capacity_range *capacities,
	      struct amount_msat *current_flows,
	      struct amount_msat additional)
{
	struct amount_msat delivered = flow->amounts[tal_count(flow->amounts)-1];

	/* Remove original from current_flows */
	for (int i = tal_count(flow->path) - 1; i >= 0; i--) {
		size_t chan_idx = gossmap_chan_idx(gossmap, flow->path[i]);

		if (!amount_msat_sub(&current_flows[chan_idx],
				     current_flows[chan_idx],
				     delivered))
			abort();
		if (!amount_msat_add_fee(&delivered,
					 flow_edge(flow, i)->base_fee,
					 flow_edge(flow, i)->proportional_fee))
			abort();
	}

	/* Add in new amount */
	if (!amount_msat_add(&delivered,
			     flow->amounts[tal_count(flow->amounts)-1], additional))
		abort();

	/* Recalc probability and fees, adjust current_flows */
	flow->amounts = tal_free(flow->amounts);
	flow_complete(flow, gossmap, capacities, current_flows, delivered);
}

/* From the paper:
 * −log((c_e + 1 − f_e) / (c_e + 1)) + μ f_e fee(e)
 */
double flow_edge_cost(const struct gossmap *gossmap,
		      const struct gossmap_chan *c, int dir,
		      const struct capacity_range *chan_cap,
		      struct amount_msat prev_flow,
		      struct amount_msat f,
		      double mu,
		      double basefee_penalty,
		      double delay_riskfactor)
{
	double prob, effective_feerate;
	double certainty_term, feerate_term;

#ifdef SUPERVERBOSE_ENABLED
	struct short_channel_id scid
		= gossmap_chan_scid(gossmap, c);
	SUPERVERBOSE("flow_edge_cost %s/%i, cap %"PRIu64"-%"PRIu64", prev_flow=%"PRIu64", f=%"PRIu64", mu=%f, basefee_penalty=%f, delay_riskfactor=%f: ",
		     type_to_string(tmpctx, struct short_channel_id, &scid),
		     dir,
		     chan_cap->min.millisatoshis, chan_cap->max.millisatoshis,
		     prev_flow.millisatoshis, f.millisatoshis,
		     mu, basefee_penalty, delay_riskfactor);
#endif

	/* Probability depends on any previous flows, too! */
	if (!amount_msat_add(&prev_flow, prev_flow, f))
		abort();
	prob = edge_probability(chan_cap, prev_flow);
	if (prob == 0) {
		SUPERVERBOSE(" INFINITE\n");
		return FLOW_INF_COST;
	}

	certainty_term = -log(prob);

	/* This is in parts-per-million */
	effective_feerate = (c->half[dir].proportional_fee
			     + c->half[dir].base_fee * basefee_penalty)
		/ 1000000.0;

	/* Feerate term includes delay factor */
	feerate_term = (mu
			* (f.millisatoshis /* Raw: costfn */
			   * effective_feerate
			   + c->half[dir].delay * delay_riskfactor));

	SUPERVERBOSE(" %f + %f = %f\n",
		     certainty_term, feerate_term,
		     certainty_term + feerate_term);
	return certainty_term + feerate_term;
}

/* Helper function to fill in amounts and success_prob for flow */
void flow_complete(struct flow *flow,
		   const struct gossmap *gossmap,
		   const struct capacity_range *capacities,
		   struct amount_msat *current_flows,
		   struct amount_msat delivered)
{
	flow->success_prob = 1.0;
	flow->amounts = tal_arr(flow, struct amount_msat, tal_count(flow->path));
	for (int i = tal_count(flow->path) - 1; i >= 0; i--) {
		size_t chan_idx = gossmap_chan_idx(gossmap, flow->path[i]);
		const struct capacity_range *cap = &capacities[chan_idx];

		flow->amounts[i] = delivered;
		if (!amount_msat_add(&current_flows[chan_idx],
				     current_flows[chan_idx],
				     flow->amounts[i]))
			abort();
		flow->success_prob
			*= edge_probability(cap, current_flows[chan_idx]);
		if (!amount_msat_add_fee(&delivered,
					 flow_edge(flow, i)->base_fee,
					 flow_edge(flow, i)->proportional_fee))
			abort();
	}
}

struct capacity_range *flow_capacity_init(const tal_t *ctx,
					  struct gossmap *gossmap)
{
	/* min = 0 */
	struct capacity_range *capacities
		= tal_arrz(ctx, struct capacity_range,
			   gossmap_max_chan_idx(gossmap));

	for (struct gossmap_chan *c = gossmap_first_chan(gossmap);
	     c;
	     c = gossmap_next_chan(gossmap, c)) {
		struct amount_sat cap_sat;
		struct capacity_range *cap
			= &capacities[gossmap_chan_idx(gossmap, c)];

		/* Any problems? max = 0 */
		if (!gossmap_chan_get_capacity(gossmap, c, &cap_sat)
		    || !amount_sat_to_msat(&cap->max, cap_sat))
			cap->max = AMOUNT_MSAT(0);
	}

	return capacities;
}

#ifndef SUPERVERBOSE_ENABLED
#undef SUPERVERBOSE
#endif
