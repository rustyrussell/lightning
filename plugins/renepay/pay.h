#ifndef LIGHTNING_PLUGINS_RENEPAY_PAY_H
#define LIGHTNING_PLUGINS_RENEPAY_PAY_H
#include "config.h"
#include <ccan/list/list.h>
#include <common/node_id.h>
#include <plugins/libplugin.h>
#include <plugins/renepay/flow.h>

#define MAX_NUM_ATTEMPTS 10

// TODO(eduardo): Test ideas
// - make a payment to a node that is hidden behind private channels, check that
// private channels are removed from the gossmap and chan_extra_map
// - one payment route hangs, and the rest keep waiting, eventually all MPP
// should timeout and we retry excluding the unresponsive path (are we able to
// identify it?)
// - a particular route fails because fees are wrong, we update the gossip
// information and redo the path.
// - a MPP in which several parts have a common intermediary node 
// 	source -MANY- o -MANY- dest
// - a MPP in which several parts have a common intermediary channel
// 	source -MANY- o--o -MANY- dest
// - a payment with a direct channel to the destination

enum payment_status {
        PAYMENT_PENDING, PAYMENT_SUCCESS, PAYMENT_FAIL,
	PAYMENT_MPP_TIMEOUT
};

/* Our convenient global data, here in one place. */
struct pay_plugin {
	/* From libplugin */
	struct plugin *plugin;

	/* Public key of this node. */
	struct node_id my_id;

	/* Map of gossip. */
	struct gossmap *gossmap;

	/* Settings for maxdelay */
	unsigned int maxdelay_default;

	/* Offers support */
	bool exp_offers;

	/* All the struct payment */
	struct list_head payments;

	/* Per-channel metadata: some persists between payments */
	struct chan_extra_map *chan_extra_map;
};

/* Set in init */
extern struct pay_plugin *pay_plugin;

/* Data only kept while the payment is being processed. */
struct active_payment
{
	/* The command, and our owner (needed for timer func) */
	struct command *cmd;

	/* Localmods to apply to gossip_map for our own use. */
	bool localmods_applied;
	struct gossmap_localmods *local_gossmods;
	
	/* Channels we decided to disable for various reasons. */
	struct short_channel_id *disabled;

	/* Timers. */
	struct plugin_timer *rexmit_timer;
	
	/* Keep track of the number of attempts. */
	int last_attempt;
	
	/* Root to destroy pending flows */
	tal_t *all_flows;
	
	/* Groupid, so listpays() can group them back together */
	u64 groupid;
	
	/* Used in get_payflows to set ids to each pay_flow. */
	u64 next_partid;
};

struct payment {
	/* Chatty description of attempts. */
	const char **paynotes;
	
	/* Total sent, including fees. */
	struct amount_msat total_sent;
	
	/* Total that is delivering (i.e. without fees) */
	struct amount_msat total_delivering;
	
	/* invstring (bolt11 or bolt12) */
	const char *invstr;
	
	/* How much, what, where */
	struct amount_msat amount;
	struct node_id dest;
	struct sha256 payment_hash;
	
	
	/* Limits on what routes we'll accept. */
	struct amount_msat maxspend;
	
	// TODO(eduardo): check out how this is used by get_payflows.
	unsigned int maxdelay;
	
	/* We promised this in pay() output */
	struct timeabs start_time;
	
	// TODO(eduardo): notice that stop_time = start_time + 60sec by default.
	// On the other hand I am assuming that HTLCs are removed when a payment
	// request fails or succeeds. A fail could happen when some other part
	// of a MPP payment fails and the current part times-out. My question is
	// what determines this time-out of the MPP part, is it encoded in the
	// HTLCs themselves, thus measured in block numbers or is there another
	// timer somewhere?
	// If this time is in seconds, it would be desirable to be less than
	// 30sec so that we can try at least two different MPP before the
	// payment expires. On the other hand if HTLCs expire only after a
	// certain block number, then we should keep track of these events in the
	// uncertainty network somehow even after the payment terminates.
	struct timeabs stop_time;
	
	/* Payment preimage, in case of success. */
	const struct preimage *preimage;
	
	/* payment_secret, if specified by invoice. */
	// TODO(eduardo): isn't the preimage the payment secret?
	struct secret *payment_secret;
	
	/* Payment metadata, if specified by invoice. */
	const u8 *payment_metadata;
	
	/* To know if the last attempt failed, succeeded or is it pending. */
	enum payment_status status;	

	// TODO(eduardo):
	// 1. what is this?
	// 2. what is it used for?
	// 3. notice that we don't use it.
	u32 final_cltv;

	/* Inside pay_plugin->payments list */
	struct list_node list;

	/* Description and labels, if any. */
	const char *description, *label;

	
	/* Penalty for CLTV delays */
	double delay_feefactor;
	
	/* Penalty for base fee */
	double base_fee_penalty;
	
	/* With these the effective linear fee cost is computed as
	 * 
	 * linear fee cost = 
	 * 	millionths 
	 * 	+ base_fee* base_fee_penalty
	 * 	+delay*delay_feefactor;
	 * */

	/* The minimum acceptable prob. of success */
	double min_prob_success;
	
	/* Conversion from prob. cost to millionths */
	double prob_cost_factor;
	/* linear prob. cost = 
	 * 	- prob_cost_factor * log prob. */


	/* If this is paying a local offer, this is the one (sendpay ensures we
	 * don't pay twice for single-use offers) */
	// TODO(eduardo): this is not being used!
	struct sha256 *local_offer_id;

	/* DEVELOPER allows disabling shadow route */
	bool use_shadow;
	
	/* Data used while the payment is being processed. */
	struct active_payment *active_payment;
};

int payment_current_attempt(const struct payment *p);

void paynote(struct payment *p, const char *fmt, ...)
	PRINTF_FMT(2,3);


/* Accumulate or panic on overflow */
#define amount_msat_accumulate(dst, src) \
	amount_msat_accumulate_((dst), (src), stringify(dst), stringify(src))
#define amount_msat_reduce(dst, src) \
	amount_msat_reduce_((dst), (src), stringify(dst), stringify(src))

void amount_msat_accumulate_(struct amount_msat *dst,
			     struct amount_msat src,
			     const char *dstname,
			     const char *srcname);
void amount_msat_reduce_(struct amount_msat *dst,
			 struct amount_msat src,
			 const char *dstname,
			 const char *srcname);

#endif /* LIGHTNING_PLUGINS_RENEPAY_PAY_H */
