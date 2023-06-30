#ifndef LIGHTNING_PLUGINS_RENEPAY_PAY_FLOW_H
#define LIGHTNING_PLUGINS_RENEPAY_PAY_FLOW_H
#include <ccan/short_types/short_types.h>
#include <plugins/renepay/payment.h>
#include <plugins/renepay/debug.h>

/* This is like a struct flow, but independent of gossmap, and contains
 * all we need to actually send the part payment. */
struct pay_flow {
	/* So we can be an independent object for callbacks. */
	struct payment * payment;

	/* This flow belongs to some attempt. */
	int attempt;
	
	/* Part id for RPC interface */
	u64 partid;
	/* The series of channels and nodes to traverse. */
	struct short_channel_id *path_scids;
	struct node_id *path_nodes;
	int *path_dirs;
	/* CLTV delays for each hop */
	u32 *cltv_delays;
	/* The amounts at each step */
	struct amount_msat *amounts;
	/* Probability estimate (0-1) */
	double success_prob;
};

struct pay_flow **get_payflows(struct renepay * renepay,
			       struct amount_msat amount,
			       struct amount_msat feebudget,
			       bool unlikely_ok,
			       bool is_entire_payment,
			       char const ** err_msg);

void commit_htlc_payflow(
		struct chan_extra_map *chan_extra_map,
		const struct pay_flow *flow);

void remove_htlc_payflow(
		struct pay_flow *flow,
		struct chan_extra_map *chan_extra_map);

const char *flow_path_to_str(const tal_t *ctx, const struct pay_flow *flow);

const char* fmt_payflows(const tal_t *ctx,
			 struct pay_flow ** flows);

/* How much does this flow deliver to destination? */
struct amount_msat payflow_delivered(const struct pay_flow *flow);

/* Removes amounts from payment and frees flow pointer. 
 * A possible destructor for flow would remove HTLCs from the
 * uncertainty_network and remove the flow from any data structure. */
void payflow_fail(struct pay_flow *flow);

#endif /* LIGHTNING_PLUGINS_RENEPAY_PAY_FLOW_H */
