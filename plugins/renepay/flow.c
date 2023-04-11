#include "config.h"
#include <assert.h>
#include <ccan/asort/asort.h>
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
static double edge_probability(struct amount_msat min, struct amount_msat max,
			       struct amount_msat f)
{
	struct amount_msat range_plus_one, numerator;

	/* If f is <= known minimum, probability is 1. */
	if (!amount_msat_sub(&f, f, min))
		return 1.0;

	/* +1 because the number of elements in the range is min-max + 1 */
	if (!amount_msat_sub(&range_plus_one, max, min))
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

static void destroy_chan_extra(struct chan_extra *ce,
			       struct chan_extra_map *chan_extra_map)
{
	chan_extra_map_del(chan_extra_map, ce);
}

/* Returns either NULL, or an entry from the hash */
struct chan_extra_half *get_chan_extra_half(const struct gossmap *gossmap,
					    struct chan_extra_map *chan_extra_map,
					    const struct gossmap_chan *chan,
					    int dir)
{
	struct chan_extra *ce;

	ce = chan_extra_map_get(chan_extra_map, gossmap_chan_scid(gossmap, chan));
	if (!ce)
		return NULL;
	return &ce->half[dir];
}

struct chan_extra_half *new_chan_extra_half(const struct gossmap *gossmap,
					    struct chan_extra_map *chan_extra_map,
					    const struct gossmap_chan *chan,
					    int dir,
					    struct amount_msat capacity)
{
	struct chan_extra *ce = tal(chan_extra_map, struct chan_extra);

	ce->scid = gossmap_chan_scid(gossmap, chan);
	for (size_t i = 0; i <= 1; i++) {
		ce->half[i].num_htlcs = 0;
		ce->half[i].htlc_total = AMOUNT_MSAT(0);
		ce->half[i].known_min = AMOUNT_MSAT(0);
		ce->half[i].known_max = capacity;
	}
	/* Remove self from map when done */
	chan_extra_map_add(chan_extra_map, ce);
	tal_add_destructor2(ce, destroy_chan_extra, chan_extra_map);
	return &ce->half[dir];
}

void remove_completed_flow(const struct gossmap *gossmap,
			   struct chan_extra_map *chan_extra_map,
			   struct flow *flow)
{
	for (size_t i = 0; i < tal_count(flow->path); i++) {
		struct chan_extra_half *h = get_chan_extra_half(gossmap, chan_extra_map,
								flow->path[i],
								flow->dirs[i]);
		if (!amount_msat_sub(&h->htlc_total, h->htlc_total, flow->amounts[i]))
			abort();
		if (h->num_htlcs == 0)
			abort();
		h->num_htlcs--;
	}
}

/* Add this to the flow. */
void flow_add(struct flow *flow,
	      const struct gossmap *gossmap,
	      struct chan_extra_map *chan_extra_map,
	      struct amount_msat additional)
{
	struct amount_msat delivered;

	/* Add in new amount */
	if (!amount_msat_add(&delivered,
			     flow->amounts[tal_count(flow->amounts)-1], additional))
		abort();

	/* Remove original from current_flows */
	remove_completed_flow(gossmap, chan_extra_map, flow);

	/* Recalc probability and fees, adjust chan_extra_map entries */
	flow_complete(flow, gossmap, chan_extra_map, delivered);
}

/* From the paper:
 * −log((c_e + 1 − f_e) / (c_e + 1)) + μ f_e fee(e)
 */
double flow_edge_cost(const struct gossmap *gossmap,
		      const struct gossmap_chan *c, int dir,
		      const struct amount_msat known_min,
		      const struct amount_msat known_max,
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
		     known_min.millisatoshis, known_max.millisatoshis,
		     prev_flow.millisatoshis, f.millisatoshis,
		     mu, basefee_penalty, delay_riskfactor);
#endif

	/* Probability depends on any previous flows, too! */
	if (!amount_msat_add(&prev_flow, prev_flow, f))
		abort();
	prob = edge_probability(known_min, known_max, prev_flow);
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
		   struct chan_extra_map *chan_extra_map,
		   struct amount_msat delivered)
{
	flow->success_prob = 1.0;
	flow->amounts = tal_arr(flow, struct amount_msat, tal_count(flow->path));
	for (int i = tal_count(flow->path) - 1; i >= 0; i--) {
		struct chan_extra_half *h;

		h = get_chan_extra_half(gossmap, chan_extra_map,
					flow->path[i], flow->dirs[i]);
		if (!h) {
			struct amount_sat cap;
			struct amount_msat cap_msat;

			if (!gossmap_chan_get_capacity(gossmap,
						       flow->path[i], &cap))
				cap = AMOUNT_SAT(0);
			if (!amount_sat_to_msat(&cap_msat, cap))
				abort();
			h = new_chan_extra_half(gossmap, chan_extra_map,
						flow->path[i], flow->dirs[i],
						cap_msat);
		}

		flow->amounts[i] = delivered;
		if (!amount_msat_add(&h->htlc_total, h->htlc_total, delivered))
			abort();
		h->num_htlcs++;
		flow->success_prob
			*= edge_probability(h->known_min, h->known_max,
					    h->htlc_total);
		if (!amount_msat_add_fee(&delivered,
					 flow_edge(flow, i)->base_fee,
					 flow_edge(flow, i)->proportional_fee))
			abort();
	}
}

static int cmp_amount_msat(const struct amount_msat *a,
			   const struct amount_msat *b,
			   void *unused)
{
	if (amount_msat_less(*a, *b))
		return -1;
	if (amount_msat_greater(*a, *b))
		return 1;
	return 0;
}

static int cmp_amount_sat(const struct amount_sat *a,
			  const struct amount_sat *b,
			  void *unused)
{
	if (amount_sat_less(*a, *b))
		return -1;
	if (amount_sat_greater(*a, *b))
		return 1;
	return 0;
}

/* Get median feerates and capacities. */
static void get_medians(const struct gossmap *gossmap,
			struct amount_msat amount,
			struct amount_msat *median_capacity,
			struct amount_msat *median_fee)
{
	size_t num_caps, num_fees;
	struct amount_sat *caps;
	struct amount_msat *fees;

	caps = tal_arr(tmpctx, struct amount_sat,
		       gossmap_max_chan_idx(gossmap));
	fees = tal_arr(tmpctx, struct amount_msat,
		       gossmap_max_chan_idx(gossmap) * 2);
	num_caps = num_fees = 0;

	for (struct gossmap_chan *c = gossmap_first_chan(gossmap);
	     c;
	     c = gossmap_next_chan(gossmap, c)) {

		/* If neither feerate is set, it's not useful */
		if (!gossmap_chan_set(c, 0) && !gossmap_chan_set(c, 1))
			continue;

		/* Insufficient capacity?  Not useful */
		if (!gossmap_chan_get_capacity(gossmap, c, &caps[num_caps]))
			continue;
		if (amount_msat_greater_sat(amount, caps[num_caps]))
			continue;
		num_caps++;

		for (int dir = 0; dir <= 1; dir++) {
			if (!gossmap_chan_set(c, dir))
				continue;
			if (!amount_msat_fee(&fees[num_fees],
					     amount,
					     c->half[dir].base_fee,
					     c->half[dir].proportional_fee))
				continue;
			num_fees++;
		}
	}

	asort(caps, num_caps, cmp_amount_sat, NULL);
	/* If there are no channels, it doesn't really matter, but
	 * this avoids div by 0 */
	if (!num_caps)
		*median_capacity = amount;
	else if (!amount_sat_to_msat(median_capacity, caps[num_caps / 2]))
		abort();

	asort(fees, num_fees, cmp_amount_msat, NULL);
	if (!num_caps)
		*median_fee = AMOUNT_MSAT(0);
	else
		*median_fee = fees[num_fees / 2];
}

double derive_mu(const struct gossmap *gossmap,
		 struct amount_msat amount,
		 double frugality)
{
	struct amount_msat median_capacity, median_fee;
	double cap_plus_one;

	get_medians(gossmap, amount, &median_capacity, &median_fee);

	cap_plus_one = median_capacity.millisatoshis + 1; /* Raw: derive_mu */
	return -log((cap_plus_one - amount.millisatoshis) /* Raw: derive_mu */
		    / cap_plus_one)
		* frugality
		/* +1 in case median fee is zero... */
		/ (median_fee.millisatoshis + 1); /* Raw: derive_mu */
}

#ifndef SUPERVERBOSE_ENABLED
#undef SUPERVERBOSE
#endif
