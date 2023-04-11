#ifndef LIGHTNING_PLUGINS_RENEPAY_MCF_H
#define LIGHTNING_PLUGINS_RENEPAY_MCF_H
#include <ccan/bitmap/bitmap.h>

/**
 * minflow - API for min cost flow function(s).
 * @ctx: context to allocate returned flows from
 * @gossmap: the gossip map
 * @source: the source to start from
 * @target: the target to pay
 * @chan_extra_map: hashtable of extra per-channel information
 * @disabled: NULL, or a bitmap by channel index of channels not to use.
 * @amount: the amount we want to reach @target
 * @frugality: how important fees are compared to certainty (0.1 = certain, 10 = fees)
 * @delay_feefactor: how important delays are compared to fees
 *
 * @delay_feefactor converts 1 block delay into msat, as if it were an additional
 * fee.  So if a CTLV delay on a node is 5 blocks, that's treated as if it
 * were a fee of 5 * @delay_feefactor.
 *
 * @mu converts fees to add it to the uncertainty term.
 *
 * Return a series of subflows which deliver amount to target, or NULL.
 */
struct flow **minflow(const tal_t *ctx,
		      struct gossmap *gossmap,
		      const struct gossmap_node *source,
		      const struct gossmap_node *target,
		      struct chan_extra_map *chan_extra_map,
		      const bitmap *disabled,
		      struct amount_msat amount,
		      double frugality,
		      double delay_feefactor);
#endif /* LIGHTNING_PLUGINS_RENEPAY_MCF_H */
