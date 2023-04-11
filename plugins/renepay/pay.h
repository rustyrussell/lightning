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
	struct chan_extra_map chan_extra_map;
};

/* Set in init */
extern struct pay_plugin *pay_plugin;

struct payment {
	/* Inside pay_plugin->payments list */
	struct list_node list;

	/* The command, and our owner (needed for timer func) */
	struct command *cmd;

	/* We promised this in pay() output */
	struct timeabs start_time;

	/* Localmods to apply to gossip_map for our own use. */
	struct gossmap_localmods *local_gossmods;

	/* invstring (bolt11 or bolt12) */
	const char *invstr;

	/* How much, what, where */
	struct node_id dest;
	struct sha256 payment_hash;
	struct amount_msat amount;
	u32 final_cltv;

	/* Total sent, including fees. */
	struct amount_msat total_sent;
	/* Total that is delivering (i.e. without fees) */
	struct amount_msat total_delivering;

	/* payment_secret, if specified by invoice. */
	struct secret *payment_secret;
	/* Payment metadata, if specified by invoice. */
	const u8 *payment_metadata;

	/* Description and labels, if any. */
	const char *description, *label;

	/* Penalty for CLTV delays */
	double delay_feefactor;

	/* Limits on what routes we'll accept. */
	struct amount_msat maxspend;
	unsigned int maxdelay;
	struct timeabs stop_time;

	/* Channels we decided to disable for various reasons. */
	struct short_channel_id *disabled;

	/* Chatty description of attempts. */
	const char **paynotes;

	/* Groupid, so listpays() can group them back together */
	u64 groupid;
	u64 next_partid;
	/* If this is paying a local offer, this is the one (sendpay ensures we
	 * don't pay twice for single-use offers) */
	struct sha256 *local_offer_id;

	/* Timers. */
	struct plugin_timer *rexmit_timer;

#if DEVELOPER
	/* Disable the use of shadow route ? */
	double use_shadow;
#endif
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
