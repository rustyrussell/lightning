#ifndef LIGHTNING_PLUGINS_RENEPAY_PAY_H
#define LIGHTNING_PLUGINS_RENEPAY_PAY_H
#include "config.h"
#include <ccan/list/list.h>
#include <common/node_id.h>
#include <plugins/renepay/flow.h>

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

struct payment {
	/* Inside pay_plugin->payments list */
	// TODO(eduardo): is it used?
	struct list_node list;

	/* The command, and our owner (needed for timer func) */
	// TODO(eduardo): is it used?
	struct command *cmd;

	/* We promised this in pay() output */
	// TODO(eduardo): is it used?
	struct timeabs start_time;

	/* Localmods to apply to gossip_map for our own use. */
	// TODO(eduardo): is it used?
	struct gossmap_localmods *local_gossmods;

	/* invstring (bolt11 or bolt12) */
	// TODO(eduardo): is it used?
	const char *invstr;

	/* How much, what, where */
	// TODO(eduardo): is it used?
	struct node_id dest;
	struct sha256 payment_hash;
	// TODO(eduardo): is it used?
	struct amount_msat amount;
	
	// TODO(eduardo): is it used?
	u32 final_cltv;

	/* Total sent, including fees. */
	struct amount_msat total_sent;
	/* Total that is delivering (i.e. without fees) */
	struct amount_msat total_delivering;

	/* payment_secret, if specified by invoice. */
	// TODO(eduardo): is it used?
	struct secret *payment_secret;
	/* Payment metadata, if specified by invoice. */
	// TODO(eduardo): is it used?
	const u8 *payment_metadata;

	/* Description and labels, if any. */
	// TODO(eduardo): is it used?
	// TODO(eduardo): could be NULL
	const char *description, *label;

	/* Penalty for CLTV delays */
	// TODO(eduardo): is it used?
	double delay_feefactor;
	
	/* Penalty for base fee */
	// TODO(eduardo): is it used?
	double base_fee_penalty;
	
	/* linear fee cost = 
	 * 	millionths 
	 * 	+ base_fee* base_fee_penalty
	 * 	+delay*delay_feefactor;*/

	/* Limits on what routes we'll accept. */
	// TODO(eduardo): is it used?
	struct amount_msat maxspend;
	// TODO(eduardo): is it used?
	unsigned int maxdelay;
	// TODO(eduardo): is it used?
	struct timeabs stop_time;
	
	/* The minimum acceptable prob. of success */
	// TODO(eduardo): is it used?
	double min_prob_success;
	
	/* linear prob. cost = 
	 * 	- prob_cost_factor * log prob. */
	
	/* Conversion from prob. cost to millionths */
	// TODO(eduardo): is it used?
	double prob_cost_factor;

	/* Channels we decided to disable for various reasons. */
	// TODO(eduardo): is it used?
	struct short_channel_id *disabled;

	/* Chatty description of attempts. */
	// TODO(eduardo): is it used?
	const char **paynotes;

	/* Groupid, so listpays() can group them back together */
	// TODO(eduardo): is it used?
	u64 groupid;
	// TODO(eduardo): is it used?
	u64 next_partid;
	/* If this is paying a local offer, this is the one (sendpay ensures we
	 * don't pay twice for single-use offers) */
	// TODO(eduardo): is it used?
	// TODO(eduardo): could be NULL
	struct sha256 *local_offer_id;

	/* Timers. */
	// TODO(eduardo): is it used?
	struct plugin_timer *rexmit_timer;

	/* DEVELOPER allows disabling shadow route */
	// TODO(eduardo): is it used?
	bool use_shadow;
};

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
