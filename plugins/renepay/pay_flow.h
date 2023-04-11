#ifndef LIGHTNING_PLUGINS_RENEPAY_PAY_FLOW_H
#define LIGHTNING_PLUGINS_RENEPAY_PAY_FLOW_H
#include <ccan/short_types/short_types.h>

/* This is like a struct flow, but independent of gossmap, and contains
 * all we need to actually send the part payment. */
struct pay_flow {
	/* So we can be an independent object for callbacks. */
	struct payment *payment;

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

struct pay_flow **get_payflows(struct payment *p,
			       struct amount_msat amount,
			       struct amount_msat feebudget,
			       bool unlikely_ok,
			       bool is_entire_payment);

#endif /* LIGHTNING_PLUGINS_RENEPAY_PAY_FLOW_H */
