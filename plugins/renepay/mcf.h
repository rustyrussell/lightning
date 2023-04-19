#ifndef LIGHTNING_PLUGINS_RENEPAY_MCF_H
#define LIGHTNING_PLUGINS_RENEPAY_MCF_H
#include <ccan/bitmap/bitmap.h>
#include <common/amount.h>
#include <common/gossmap.h>

struct chan_extra_map;

enum {
	RENEPAY_ERR_OK,
	// No feasible flow found, either there is not enough known liquidity (or capacity) 
	// in the channels to complete the payment
	RENEPAY_ERR_NOFEASIBLEFLOW,
	// There is at least one feasible flow, but the the cheapest solution that we
	// found is too expensive, we return the result anyways.
	RENEPAY_ERR_NOCHEAPFLOW
};

/* In this module, internally a flow is non-negative integer function on the
 * arcs that satisfies the capacity constraints; outside, a flow is seen as one
 * amount sent through a sequence of directed channels. */
struct flow_path
{
	struct gossmap_chan **path;
	int *dirs;
	struct amount_sat amount;
};


/**
 * optimal_payment_flow - API for min cost flow function(s).
 * @ctx: context to allocate returned flows from
 * @gossmap: the gossip map
 * @source: the source to start from
 * @target: the target to pay
 * @chan_extra_map: hashtable of extra per-channel information
 * @disabled: NULL, or a bitmap by channel index of channels not to use.
 * @amount: the amount we want to reach @target
 * @max_fee_ppm: the maximum ppm allowed in fees
 * @delay_feefactor: how important delays are compared to fees
 * @errorcode: error code in case of failure
 *
 * @delay_feefactor converts 1 block delay into msat, as if it were an additional
 * fee.  So if a CTLV delay on a node is 5 blocks, that's treated as if it
 * were a fee of 5 * @delay_feefactor.
 *
 * @mu converts fees to add it to the uncertainty term.
 *
 * Return a series of subflows which deliver amount to target, or NULL.
 */
struct flow **optimal_payment_flow(
                      const tal_t *ctx,
		      struct gossmap *gossmap,
		      const struct gossmap_node *source,
		      const struct gossmap_node *target,
		      struct chan_extra_map *chan_extra_map,
		      const bitmap *disabled,
		      struct amount_msat amount,
		      double max_fee_ppm,
		      double delay_feefactor,
		      int *errorcode);
#endif /* LIGHTNING_PLUGINS_RENEPAY_MCF_H */
