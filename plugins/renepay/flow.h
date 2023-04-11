#ifndef LIGHTNING_PLUGINS_RENEPAY_FLOW_H
#define LIGHTNING_PLUGINS_RENEPAY_FLOW_H
#include <common/amount.h>

/* This is the known range of capacities, indexed by gossmap_chan_idx.
 *
 * Note that we should track this independently in both directions, but we
 * don't: we rarely consider both directions of a channel in a single payment.
 */
struct capacity_range {
	struct amount_msat min, max;
};

/* An actual partial flow. */
struct flow {
	/* The series of channels to traverse. */
	const struct gossmap_chan **path;
	/* The directions to traverse. */
	int *dirs;
	/* Amounts for this flow (fees mean this shrinks). */
	struct amount_msat *amounts;
	/* Probability of success (0-1) */
	double success_prob;
};

/* Create a capacity range for this graph */
struct capacity_range *flow_capacity_init(const tal_t *ctx,
					  struct gossmap *gossmap);

/* Helper to access the half chan at flow index idx */
const struct half_chan *flow_edge(const struct flow *flow, size_t idx);

/* Path comparison helper */
bool flow_path_eq(const struct gossmap_chan **path1,
		  const int *dirs1,
		  const struct gossmap_chan **path2,
		  const int *dirs2);

/* Add this to the completed flow. */
void flow_add(struct flow *flow,
	      const struct gossmap *gossmap,
	      const struct capacity_range *capacities,
	      struct amount_msat *current_flows,
	      struct amount_msat additional);

/* A big number, meaning "don't bother" (not infinite, since you may add) */
#define FLOW_INF_COST 100000000.0

/* Cost function to send @f msat through @c in direction @dir,
 * given we already have a flow of prev_flow. */
double flow_edge_cost(const struct gossmap *gossmap,
		      const struct gossmap_chan *c, int dir,
		      const struct capacity_range *chan_cap,
		      struct amount_msat prev_flow,
		      struct amount_msat f,
		      double mu,
		      double basefee_penalty,
		      double delay_riskfactor);

/* Function to fill in amounts and success_prob for flow, and add to
 * current_flows[] */
void flow_complete(struct flow *flow,
		   const struct gossmap *gossmap,
		   const struct capacity_range *capacities,
		   struct amount_msat *current_flows,
		   struct amount_msat delivered);

#endif /* LIGHTNING_PLUGINS_RENEPAY_FLOW_H */
