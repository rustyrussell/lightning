#ifndef LIGHTNING_PLUGINS_RENEPAY_FLOW_H
#define LIGHTNING_PLUGINS_RENEPAY_FLOW_H
#include "config.h"
#include <bitcoin/short_channel_id.h>
#include <ccan/htable/htable_type.h>
#include <common/amount.h>

/* Any implementation needs to keep some data on channels which are
 * in-use (or about which we have extra information).  We use a hash
 * table here, since most channels are not in use. */
struct chan_extra {
	struct short_channel_id scid;

	struct chan_extra_half {
		/* How many htlcs we've directed through it */
		size_t num_htlcs;

		/* The total size of those HTLCs */
		struct amount_msat htlc_total;

		/* The known minimum / maximum capacity (if nothing known, 0/capacity */
		struct amount_msat known_min, known_max;
	} half[2];
};

static inline const struct short_channel_id
chan_extra_scid(const struct chan_extra *cd)
{
	return cd->scid;
}

static inline size_t hash_scid(const struct short_channel_id scid)
{
	/* scids cost money to generate, so simple hash works here */
	return (scid.u64 >> 32)
		^ (scid.u64 >> 16)
		^ scid.u64;
}

static inline bool chan_extra_eq_scid(const struct chan_extra *cd,
				      const struct short_channel_id scid)
{
	return short_channel_id_eq(&scid, &cd->scid);
}

HTABLE_DEFINE_TYPE(struct chan_extra,
		   chan_extra_scid, hash_scid, chan_extra_eq_scid,
		   chan_extra_map);

/* Helpers for chan_extra_map */
struct chan_extra_half *get_chan_extra_half_by_scid(struct chan_extra_map *chan_extra_map,
						    const struct short_channel_id scid,
						    int dir);

struct chan_extra_half *get_chan_extra_half_by_chan(const struct gossmap *gossmap,
						    struct chan_extra_map *chan_extra_map,
						    const struct gossmap_chan *chan,
						    int dir);

/* tal_free() this removes it from chan_extra_map */
struct chan_extra_half *new_chan_extra_half(struct chan_extra_map *chan_extra_map,
					    const struct short_channel_id scid,
					    int dir,
					    struct amount_msat capacity);

/* An actual partial flow. */
struct flow {
	/* The series of channels to traverse. */
	const struct gossmap_chan **path;
	/* The directions to traverse. */
	int *dirs;
	/* Amounts for this flow (fees mean this shrinks across path). */
	struct amount_msat *amounts;
	/* Probability of success (0-1) */
	double success_prob;
};

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
	      struct chan_extra_map *chan_extra_map,
	      struct amount_msat additional);

/* A big number, meaning "don't bother" (not infinite, since you may add) */
#define FLOW_INF_COST 100000000.0

/* Cost function to send @f msat through @c in direction @dir,
 * given we already have a flow of prev_flow. */
double flow_edge_cost(const struct gossmap *gossmap,
		      const struct gossmap_chan *c, int dir,
		      const struct amount_msat known_min,
		      const struct amount_msat known_max,
		      struct amount_msat prev_flow,
		      struct amount_msat f,
		      double mu,
		      double basefee_penalty,
		      double delay_riskfactor);

/* Function to fill in amounts and success_prob for flow, and add to
 * chan_extra_map */
void flow_complete(struct flow *flow,
		   const struct gossmap *gossmap,
		   struct chan_extra_map *chan_extra_map,
		   struct amount_msat delivered);

/* Once flow is completed, this can remove it from the extra_map */
void remove_completed_flow(const struct gossmap *gossmap,
			   struct chan_extra_map *chan_extra_map,
			   struct flow *flow);

/*
 * mu (μ) is used as follows in the cost function:
 *
 *     -log((c_e + 1 - f_e) / (c_e + 1)) + μ fee
 *
 * This raises the question of how to set mu?  The left term is a
 * logrithmic failure probability, the right term is the fee in
 * millisats.
 *
 * We want a more "usable" measure of frugality (fr), where fr = 1
 * means that the two terms are roughly equal, and fr < 1 means the
 * probability is more important, and fr > 1 means the fee is more
 * important.
 *
 * For this we take the current payment amount and the median channel
 * capacity and feerates:
 *
 * -log((median_cap + 1 - f_e) / (median_cap + 1)) = μ (1/fr) median_fee
 *
 * => μ = -log((median_cap + 1 - f_e) / (median_cap + 1)) * fr / median_fee
 *
 * But this is slightly too naive!  If we're trying to make a payment larger
 * than the median, this is undefined; and grows hugely when we're near the median.
 * In fact, it should be "the median of all channels larger than the amount",
 * which is what we calculate here.
 *
 * Turns out that in the real network:
 * - median_cap = 1250800000
 * - median_feerate = 51
 *
 * And the log term at the 10th percentile capacity is about 0.125 of the median,
 * and at the 90th percentile capacity the log term is about 12.5 the value at the median.
 *
 * In other words, we expose a simple frugality knob with reasonable
 * range 0.1 (don't care about fees) to 10 (fees before probability),
 * and generate our μ from there.
 */
double derive_mu(const struct gossmap *gossmap,
		 struct amount_msat amount,
		 double frugality);

#endif /* LIGHTNING_PLUGINS_RENEPAY_FLOW_H */
