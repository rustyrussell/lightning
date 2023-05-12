#include "config.h"
#include <ccan/list/list.h>
#include <ccan/lqueue/lqueue.h>
#include <ccan/tal/tal.h>
#include <ccan/bitmap/bitmap.h>
#include <plugins/renepay/mcf.h>
#include <plugins/renepay/flow.h>
#include <plugins/renepay/dijkstra.h>
#include <common/type_to_string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

/* # Optimal payments
 * 
 * In this module we reduce the routing optimization problem to a linear
 * cost optimization problem and find a solution using MCF algorithms.
 * The optimization of the routing itself doesn't need a precise numerical
 * solution, since we can be happy near optimal results; e.g. paying 100 msat or
 * 101 msat for fees doesn't make any difference if we wish to deliver 1M sats.
 * On the other hand, we are now also considering Pickhard's 
 * [1] model to improve payment reliability,
 * hence our optimization moves to a 2D space: either we like to maximize the
 * probability of success of a payment or minimize the routing fees, or
 * alternatively we construct a function of the two that gives a good compromise.
 * 
 * Therefore from now own, the definition of optimal is a matter of choice.
 * To simplify the API of this module, we think the best way to state the
 * problem is: 
 * 
 * 	Find a routing solution that pays the least of fees while keeping
 * 	the probability of success above a certain value `min_probability`.
 * 
 *
 * # Fee Cost
 *
 * Routing fees is non-linear function of the payment flow x, that's true even 
 * without the base fee:
 * 
 * 	fee_msat = base_msat + floor(millionths*x_msat / 10^6)
 *
 * We approximate this fee into a linear function by computing a slope `c_fee` such
 * that:
 * 
 * 	fee_microsat = c_fee * x_sat
 * 	
 * Function `linear_fee_cost` computes `c_fee` based on the base and
 * proportional fees of a channel. 
 * The final product if microsat because if only
 * the proportional fee was considered we can have c_fee = millionths.
 * Moving to costs based in msats means we have to either truncate payments
 * below 1ksats or estimate as 0 cost for channels with less than 1000ppm.
 * 
 * TODO(eduardo): shall we build a linear cost function in msats?
 * 
 * # Probability cost
 * 
 * The probability of success P of the payment is the product of the prob. of
 * success of forwarding parts of the payment over all routing channels. This
 * problem is separable if we log it, and since we would like to increase P,
 * then we can seek to minimize -log(P), and that's our prob. cost function [1].
 * 
 * 	- log P = sum_{i} - log P_i
 * 
 * The probability of success `P_i` of sending some flow `x` on a channel with
 * liquidity l in the range a<=l<b is
 * 
 * 	P_{a,b}(x) = (b-x)/(b-a); for x > a
 * 		   = 1.         ; for x <= a
 * 
 * Notice that unlike the similar formula in [1], the one we propose does not
 * contain the quantization shot noise for counting states. The formula remains
 * valid independently of the liquidity units (sats or msats).
 * 
 * The cost associated to probability P is then -k log P, where k is some
 * constant. For k=1 we get the following table:
 *
 * 	prob | cost
 * 	-----------
 * 	0.01 | 4.6
 * 	0.02 | 3.9
 * 	0.05 | 3.0
 * 	0.10 | 2.3
 * 	0.20 | 1.6
 * 	0.50 | 0.69
 * 	0.80 | 0.22
 * 	0.90 | 0.10
 * 	0.95 | 0.05
 * 	0.98 | 0.02
 * 	0.99 | 0.01
 * 	
 * Clearly -log P(x) is non-linear; we try to linearize it piecewise:
 * split the channel into 4 arcs representing 4 liquidity regions:
 * 
 * 	arc_0 -> [0, a)
 * 	arc_1 -> [a, a+(b-a)*f1)
 * 	arc_2 -> [a+(b-a)*f1, a+(b-a)*f2)
 * 	arc_3 -> [a+(b-a)*f2, a+(b-a)*f3)
 * 
 * where f1 = 0.5, f2 = 0.8, f3 = 0.95;
 * We fill arc_0's capacity with complete certainty P=1, then if more flow is
 * needed we start filling the capacity in arc_1 until the total probability 
 * of success reaches P=0.5, then arc_2 until P=1-0.8=0.2, and finally arc_3 until 
 * P=1-0.95=0.05. We don't go further than 5% prob. of success per channel.
 
 * TODO(eduardo): this channel linearization is hard coded into
 * `CHANNEL_PIVOTS`, maybe we can parametrize this to take values from the config file.
 * 
 * With this choice, the slope of the linear cost function becomes:
 * 
 * 	m_0 = 0
 * 	m_1 = 1.38 k /(b-a)
 * 	m_2 = 3.05 k /(b-a)
 * 	m_3 = 9.24 k /(b-a)
 * 
 * Notice that one of the assumptions in [2] for the MCF problem is that flows
 * and the slope of the costs functions are integer numbers. The only way we
 * have at hand to make it so, is to choose a universal value of `k` that scales
 * up the slopes so that floor(m_i) is not zero for every arc.
 *
 * # Combine fee and prob. costs
 * 
 * We attempt to solve the original problem of finding the solution that
 * pays the least fees while keeping the prob. of success above a certain value,
 * by constructing a cost function which is a linear combination of fee and
 * prob. costs.
 * TODO(eduardo): investigate how this procedure is justified,
 * possibly with the use of Lagrange optimization theory.
 * 
 * At first, prob. and fee costs live in different dimensions, they cannot be
 * summed, it's like comparing apples and oranges.
 * However we propose to scale the prob. cost by a global factor k that
 * translates into the monetization of prob. cost.
 * 
 * k/1000, for instance, becomes the equivalent monetary cost
 * of increasing the probability of success by 0.1% for P~100%.
 * 
 * The input parameter `prob_cost_factor` in the function `minflow` is defined
 * as the PPM from the delivery amount `T` we are *willing to pay* to increase the
 * prob. of success by 0.1%:
 * 
 * 	k_microsat = floor(1000*prob_cost_factor * T_sat)
 * 	
 * Is this enough to make integer prob. cost per unit flow?
 * For `prob_cost_factor=10`; i.e. we pay 10ppm for increasing the prob. by
 * 0.1%, we get that 
 * 
 * 	-> any arc with (b-a) > 10000 T, will have zero prob. cost, which is
 * 	reasonable because even if all the flow passes through that arc, we get
 * 	a 1.3 T/(b-a) ~ 0.01% prob. of failure at most.
 * 	
 * 	-> if (b-a) ~ 10000 T, then the arc will have unit cost, or just that we
 * 	pay 1 microsat for every sat we send through this arc.
 * 	
 * 	-> it would be desirable to have a high proportional fee when (b-a)~T, 
 * 	because prob. of failure start to become very high.
 * 	In this case we get to pay 10000 microsats for every sat.
 * 
 * Once `k` is fixed then we can combine the linear prob. and fee costs, both
 * are in monetary units.
 * 
 * Note: with costs in microsats, because slopes represent ppm and flows are in
 * sats, then our integer bounds with 64 bits are such that we can move as many
 * as 10'000 BTC without overflow:
 * 
 * 	10^6 (max ppm) * 10^8 (sats per BTC) * 10^4 = 10^18
 *
 * # References
 * 
 * [1] Pickhardt and Richter, https://arxiv.org/abs/2107.05322
 * [2] R.K. Ahuja, T.L. Magnanti, and J.B. Orlin. Network Flows: 
 * Theory, Algorithms, and Applications. Prentice Hall, 1993.
 * 
 * 	
 * TODO(eduardo) it would be interesting to see:
 * how much do we pay for reliability?
 * Cost_fee(most reliable solution) - Cost_fee(cheapest solution)
 * 
 * TODO(eduardo): it would be interesting to see:
 * how likely is the most reliable path with respect to the cheapest?
 * Prob(reliable)/Prob(cheapest) = Exp(Cost_prob(cheapest)-Cost_prob(reliable))
 * 
 * */

#define PARTS_BITS 2
#define CHANNEL_PARTS (1 << PARTS_BITS)

// These are the probability intervals we use to decompose a channel into linear
// cost function arcs.
static const double CHANNEL_PIVOTS[]={0,0.5,0.8,0.95};

// how many bits for linearization parts plus 1 bit for the direction of the
// channel plus 1 bit for the dual representation.
static const size_t ARC_ADDITIONAL_BITS = PARTS_BITS + 2;

static const s64 INFINITE = INT64_MAX;
static const u32 INVALID_INDEX=0xffffffff;
static const s64 MU_MAX = 128;

/* Let's try this encoding of arcs:
 * Each channel `c` has two possible directions identified by a bit 
 * `half` or `!half`, and each one of them has to be
 * decomposed into 4 liquidity parts in order to
 * linearize the cost function, but also to solve MCF
 * problem we need to keep track of flows in the
 * residual network hence we need for each directed arc
 * in the network there must be another arc in the
 * opposite direction refered to as it's dual. In total
 * 1+2+1 additional bits of information:
 * 
 * 	(chan_idx)(half)(part)(dual)
 * 
 * That means, for each channel we need to store the
 * information of 16 arcs. If we implement a convex-cost
 * solver then we can reduce that number to size(half)size(dual)=4.
 * 
 * In the adjacency of a `node` we are going to store
 * the outgoing arcs. If we ever need to loop over the
 * incoming arcs then we will define a reverse adjacency
 * API. 
 * Then for each outgoing channel `(c,half)` there will
 * be 4 parts for the actual residual capacity, hence
 * with the dual bit set to 0:
 * 
 * 	(c,half,0,0)
 * 	(c,half,1,0)
 * 	(c,half,2,0)
 * 	(c,half,3,0)
 * 	
 * and also we need to consider the dual arcs
 * corresponding to the channel direction `(c,!half)`
 * (the dual has reverse direction):
 * 
 * 	(c,!half,0,1)
 * 	(c,!half,1,1)
 * 	(c,!half,2,1)
 * 	(c,!half,3,1)
 *  
 * These are the 8 outgoing arcs relative to `node` and
 * associated with channel `c`. The incoming arcs will
 * be:
 * 
 * 	(c,!half,0,0)
 * 	(c,!half,1,0)
 * 	(c,!half,2,0)
 * 	(c,!half,3,0)
 * 
 * 	(c,half,0,1)
 * 	(c,half,1,1)
 * 	(c,half,2,1)
 * 	(c,half,3,1)
 * 	
 * but they will be stored as outgoing arcs on the peer
 * node `next`.
 * 
 * I hope this will clarify my future self when I forget.
 * 
 * */
typedef union
{
	struct{
		u32 dual: 1;
		u32 part: PARTS_BITS;
		u32 chandir: 1;
		u32 chanidx: (32-1-PARTS_BITS-1);
	};
	u32 idx;
} arc_t;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct pay_parameters {
	/* The gossmap we are using */
	struct gossmap *gossmap;
	struct gossmap_node const*source;
	struct gossmap_node const*target;
	
	/* Extra information we intuited about the channels */
	struct chan_extra_map *chan_extra_map;
	
	/* Optional bitarray of disabled channels. */
	const bitmap *disabled;
	
	// how much we pay
	struct amount_msat amount;
	
	// channel linearization parameters
	double cap_fraction[CHANNEL_PARTS], 
	       cost_fraction[CHANNEL_PARTS];
	
	struct amount_msat max_fee;
	double min_probability;
	double delay_feefactor;
	double base_fee_penalty;
	u32 prob_cost_factor;
};

/* Representation of the linear MCF network. 
 * This contains the topology of the extended network (after linearization and
 * addition of arc duality).
 * This contains also the arc probability and linear fee cost, as well as
 * capacity; these quantities remain constant during MCF execution. */
struct linear_network
{
	u32 *arc_tail_node; 
	// notice that a tail node is not needed, 
	// because the tail of arc is the head of dual(arc)
	
	arc_t *node_adjacency_next_arc;
	arc_t *node_adjacency_first_arc;	
	
	// probability and fee cost associated to an arc
	s64 *arc_prob_cost, *arc_fee_cost;
	s64 *capacity;
	
	size_t max_num_arcs,max_num_nodes;
};

/* This is the structure that keeps track of the network properties while we
 * seek for a solution. */
struct residual_network {
	/* residual capacity on arcs */
	s64 *cap; 
	
	/* some combination of prob. cost and fee cost on arcs */
	s64 *cost;
	
	/* potential function on nodes */
	s64 *potential;
};

/* Helper function. 
 * Given an arc idx, return the dual's idx in the residual network. */
static arc_t arc_dual(arc_t arc)
{
	arc.dual ^= 1;
	return arc;
}
/* Helper function. */
static bool arc_is_dual(const arc_t arc)
{
	return arc.dual == 1;
}

/* Helper function. 
 * Given an arc of the network (not residual) give me the flow. */
static s64 get_arc_flow(
		const struct residual_network *network,
		const arc_t arc)
{
	assert(!arc_is_dual(arc));
	assert(arc_dual(arc).idx < tal_count(network->cap));
	return network->cap[ arc_dual(arc).idx ];
}

/* Helper function. 
 * Given an arc idx, return the node from which this arc emanates in the residual network. */
static u32 arc_tail(const struct linear_network *linear_network,
                    const arc_t arc)
{
	assert(arc.idx < tal_count(linear_network->arc_tail_node));
	return linear_network->arc_tail_node[ arc.idx ];
}
/* Helper function. 
 * Given an arc idx, return the node that this arc is pointing to in the residual network. */
static u32 arc_head(const struct linear_network *linear_network,
                    const arc_t arc)
{
	const arc_t dual = arc_dual(arc);
	assert(dual.idx < tal_count(linear_network->arc_tail_node));
	return linear_network->arc_tail_node[dual.idx];
}
		
/* Helper function. 
 * Given node idx `node`, return the idx of the first arc whose tail is `node`.
 * */
static arc_t node_adjacency_begin(
		const struct linear_network * linear_network,
		const u32 node)
{
	assert(node < tal_count(linear_network->node_adjacency_first_arc));
	return linear_network->node_adjacency_first_arc[node];
}

/* Helper function. 
 * Is this the end of the adjacency list. */
static bool node_adjacency_end(const arc_t arc)
{
	return arc.idx == INVALID_INDEX;
}

/* Helper function. 
 * Given node idx `node` and `arc`, returns the idx of the next arc whose tail is `node`. */
static arc_t node_adjacency_next(
		const struct linear_network *linear_network,
		const arc_t arc)
{
	assert(arc.idx < tal_count(linear_network->node_adjacency_next_arc));
	return linear_network->node_adjacency_next_arc[arc.idx];
}

/* Helper function.
 * Given a channel index, we should be able to deduce the arc id. */
static arc_t channel_idx_to_arc(
		const u32 chan_idx,
                int half,
		int part,
		int dual)
{
	arc_t arc;
	// arc.idx=0; // shouldn't be necessary, but valgrind complains of uninitialized field idx
	arc.dual=dual;
	arc.part=part;
	arc.chandir=half;
	arc.chanidx = chan_idx;
	return arc;
}

// TODO(eduardo): unit test this
/* Split a directed channel into parts with linear cost function. */
static void linearize_channel(
		const struct pay_parameters *params,
		const struct gossmap_chan *c,
		const int dir,
		s64 *capacity,
		s64 *cost)
{
	struct chan_extra_half *extra_half = get_chan_extra_half_by_chan_verify(
							params->gossmap,
							params->chan_extra_map,
							c,
							dir);
	
	s64 a = extra_half->known_min.millisatoshis/1000,
	    b = 1 + extra_half->known_max.millisatoshis/1000;
	
	capacity[0]=a;
	cost[0]=0;
	for(size_t i=1;i<CHANNEL_PARTS;++i)
	{
		capacity[i] = params->cap_fraction[i]*(b-a);
		
		cost[i] = params->cost_fraction[i]
		          *params->amount.millisatoshis
		          *params->prob_cost_factor*1.0/(b-a);
		
		// printf("channel part: %ld, cost_fraction: %lf, cost: %ld\n",
		// 	i,params->cost_fraction[i],cost[i]);
		// printf("prob_cost_factor: %d, amount_msat: %ld\n",
		// 	params->prob_cost_factor,params->amount.millisatoshis);
	}
}

static void alloc_residual_netork(
		const struct linear_network * linear_network,
		struct residual_network* residual_network)
{
	const size_t max_num_arcs = linear_network->max_num_arcs;
	const size_t max_num_nodes = linear_network->max_num_nodes;
	
	residual_network->cap = tal_arrz(residual_network,s64,max_num_arcs);
	residual_network->cost = tal_arrz(residual_network,s64,max_num_arcs);	
	residual_network->potential = tal_arrz(residual_network,s64,max_num_nodes);	
}
static void init_residual_netork(
		const struct linear_network * linear_network,
		struct residual_network* residual_network)
{
	const size_t max_num_arcs = linear_network->max_num_arcs;
	const size_t max_num_nodes = linear_network->max_num_nodes;
	for(u32 idx=0;idx<max_num_arcs;++idx)
	{
		arc_t arc = (arc_t){.idx=idx};
		
		if(arc_is_dual(arc))
			continue;
		
		arc_t dual = arc_dual(arc); 
		residual_network->cap[arc.idx]=linear_network->capacity[arc.idx];
		residual_network->cap[dual.idx]=0;
		
		residual_network->cost[arc.idx]=residual_network->cost[dual.idx]=0;
	}
	for(u32 i=0;i<max_num_nodes;++i)
	{
		residual_network->potential[i]=0;
	}
}

static void combine_cost_function(
		const struct linear_network* linear_network,
		struct residual_network *residual_network,
		s64 mu)
{
	// printf("Report on arcs cost\n");
	for(u32 arc_idx=0;arc_idx<linear_network->max_num_arcs;++arc_idx)
	{
		arc_t arc = (arc_t){.idx=arc_idx};
		if(arc_tail(linear_network,arc)==INVALID_INDEX)
			continue;
		
		const s64 pcost = linear_network->arc_prob_cost[arc_idx],
		          fcost = linear_network->arc_fee_cost[arc_idx];
			  
		const s64 combined = pcost==INFINITE || fcost==INFINITE ? INFINITE :
		                     mu*fcost + (MU_MAX-1-mu)*pcost;
			       
		residual_network->cost[arc_idx]
			= mu==0 ? pcost : 
			          (mu==(MU_MAX-1) ? fcost : combined);
		
		// printf("arc_idx: %d, comb. cost: %ld, res. cap: %ld\n",arc_idx,
		// 	residual_network->cost[arc_idx],
		// 	residual_network->cap[arc_idx]);
		// printf("cap: %ld, prob_cost: %ld, fee_cost: %ld, mu: %ld\n\n",
		// 	linear_network->capacity[arc_idx],
		// 	linear_network->arc_prob_cost[arc_idx],
		// 	linear_network->arc_fee_cost[arc_idx],
		// 	mu);
	}
	// printf("\n\n");
}

static void linear_network_add_adjacenct_arc(
		struct linear_network *linear_network,
		const u32 node_idx,
		const arc_t arc)
{
	assert(arc.idx < tal_count(linear_network->arc_tail_node));
	linear_network->arc_tail_node[arc.idx] = node_idx;
	
	assert(node_idx < tal_count(linear_network->node_adjacency_first_arc));
	const arc_t first_arc = linear_network->node_adjacency_first_arc[node_idx];

	assert(arc.idx < tal_count(linear_network->node_adjacency_next_arc));
	linear_network->node_adjacency_next_arc[arc.idx]=first_arc;
	
	assert(node_idx < tal_count(linear_network->node_adjacency_first_arc));
	linear_network->node_adjacency_first_arc[node_idx]=arc;
}
				

static void init_linear_network(
		const struct pay_parameters *params,
		struct linear_network *linear_network)
{
	const size_t max_num_chans = gossmap_max_chan_idx(params->gossmap);
	const size_t max_num_arcs = max_num_chans << ARC_ADDITIONAL_BITS;
	const size_t max_num_nodes = gossmap_max_node_idx(params->gossmap);
	
	linear_network->max_num_arcs = max_num_arcs;
	linear_network->max_num_nodes = max_num_nodes;
	
	linear_network->arc_tail_node = tal_arr(linear_network,u32,max_num_arcs);
	for(size_t i=0;i<tal_count(linear_network->arc_tail_node);++i)
		linear_network->arc_tail_node[i]=INVALID_INDEX;
	
	linear_network->node_adjacency_next_arc = tal_arr(linear_network,arc_t,max_num_arcs);
	for(size_t i=0;i<tal_count(linear_network->node_adjacency_next_arc);++i)
		linear_network->node_adjacency_next_arc[i].idx=INVALID_INDEX;
	
	linear_network->node_adjacency_first_arc = tal_arr(linear_network,arc_t,max_num_nodes);
	for(size_t i=0;i<tal_count(linear_network->node_adjacency_first_arc);++i)
		linear_network->node_adjacency_first_arc[i].idx=INVALID_INDEX;
	
	linear_network->arc_prob_cost = tal_arr(linear_network,s64,max_num_arcs);
	for(size_t i=0;i<tal_count(linear_network->arc_prob_cost);++i)
		linear_network->arc_prob_cost[i]=INFINITE;
	
	linear_network->arc_fee_cost = tal_arr(linear_network,s64,max_num_arcs);
	for(size_t i=0;i<tal_count(linear_network->arc_fee_cost);++i)
		linear_network->arc_fee_cost[i]=INFINITE;
	
	linear_network->capacity = tal_arrz(linear_network,s64,max_num_arcs);
	
	for(struct gossmap_node *node = gossmap_first_node(params->gossmap);
	    node;
	    node=gossmap_next_node(params->gossmap,node))
	{
		const u32 node_id = gossmap_node_idx(params->gossmap,node);
		
		for(size_t j=0;j<node->num_chans;++j)
		{
			
			
			int half;
			const struct gossmap_chan *c = gossmap_nth_chan(params->gossmap,
			                                                node, j, &half);
			
			// TODO(eduardo): do we check if the channel is public?
			if (!gossmap_chan_set(c,half))
				continue;
				
			const u32 chan_id = gossmap_chan_idx(params->gossmap, c);
			
			if (params->disabled && bitmap_test_bit(params->disabled,chan_id))
				continue;
				
			
			const struct gossmap_node *next = gossmap_nth_node(params->gossmap,
									   c,!half);
									   
			const u32 next_id = gossmap_node_idx(params->gossmap,next);
			
			if(node_id==next_id)
				continue;
			
			// `cost` is the word normally used to denote cost per
			// unit of flow in the context of MCF.
			s64 prob_cost[CHANNEL_PARTS], capacity[CHANNEL_PARTS];
			
			// split this channel direction to obtain the arcs
			// that are outgoing to `node`
			linearize_channel(params,c,half,capacity,prob_cost);
			
			const s64 fee_cost = linear_fee_cost(c,half,
						params->base_fee_penalty,
						params->delay_feefactor);
			
			// let's subscribe the 4 parts of the channel direction
			// (c,half), the dual of these guys will be subscribed
			// when the `i` hits the `next` node.
			for(size_t k=0;k<CHANNEL_PARTS;++k)
			{
				// if(capacity[k]==0)continue;
				
				arc_t arc = channel_idx_to_arc(chan_id,half,k,0);
				
				linear_network_add_adjacenct_arc(linear_network,node_id,arc);
				
				linear_network->capacity[arc.idx] = capacity[k];
				linear_network->arc_prob_cost[arc.idx] = prob_cost[k];
				
				linear_network->arc_fee_cost[arc.idx] = fee_cost;
				
				// + the respective dual
				arc_t dual = arc_dual(arc);
				
				linear_network_add_adjacenct_arc(linear_network,next_id,dual);
				
				linear_network->capacity[dual.idx] = 0;
				linear_network->arc_prob_cost[dual.idx] = -prob_cost[k];
				
				linear_network->arc_fee_cost[dual.idx] = -fee_cost;
			}
		}
	}
}

/* Simple queue to traverse the network. */
struct queue_data
{
	u32 idx;
	struct lqueue_link ql;
};

// TODO(eduardo): unit test this
/* Finds an admissible path from source to target, traversing arcs in the
 * residual network with capacity greater than 0. 
 * The path is encoded into prev, which contains the idx of the arcs that are
 * traversed. 
 * Returns RENEPAY_ERR_OK if the path exists. */
static int find_admissible_path(
		const struct linear_network *linear_network,
		const struct residual_network *residual_network,
                const u32 source,
		const u32 target,
		arc_t *prev)
{
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	int ret = RENEPAY_ERR_NOFEASIBLEFLOW;
	for(size_t i=0;i<tal_count(prev);++i)
		prev[i].idx=INVALID_INDEX;
	
	// The graph is dense, and the farthest node is just a few hops away,
	// hence let's BFS search.
	LQUEUE(struct queue_data,ql) myqueue = LQUEUE_INIT;
	struct queue_data *qdata;
	
	qdata = tal(this_ctx,struct queue_data);
	qdata->idx = source;
	lqueue_enqueue(&myqueue,qdata);
	
	while(!lqueue_empty(&myqueue))
	{
		qdata = lqueue_dequeue(&myqueue);
		u32 cur = qdata->idx;
	
		tal_free(qdata);
		
		if(cur==target)
		{
			ret = RENEPAY_ERR_OK;
			break;
		}
		
		for(arc_t arc = node_adjacency_begin(linear_network,cur);
		        !node_adjacency_end(arc);
			arc = node_adjacency_next(linear_network,arc))
		{
			// check if this arc is traversable
			if(residual_network->cap[arc.idx] <= 0)
				continue;
			
			u32 next = arc_head(linear_network,arc);
			
			assert(next < tal_count(prev));
			
			// if that node has been seen previously
			if(prev[next].idx!=INVALID_INDEX)
				continue;
			
			prev[next] = arc;
			
			qdata = tal(tmpctx,struct queue_data);
			qdata->idx = next;
			lqueue_enqueue(&myqueue,qdata);
		}
	}
	tal_free(this_ctx);	
	return ret;
}

/* Get the max amount of flow one can send from source to target along the path
 * encoded in `prev`. */
static s64 get_augmenting_flow(
		const struct linear_network* linear_network,
		const struct residual_network *residual_network,
	        const u32 source,
		const u32 target,
		const arc_t *prev)
{
	s64 flow = INFINITE;
	
	u32 cur = target;
	while(cur!=source)
	{
		assert(cur<tal_count(prev));
		const arc_t arc = prev[cur];
		flow = MIN(flow , residual_network->cap[arc.idx]);
		
		// we are traversing in the opposite direction to the flow,
		// hence the next node is at the tail of the arc.
		cur = arc_tail(linear_network,arc);
	}
	
	assert(flow<INFINITE && flow>0);
	return flow;
}

/* Augment a `flow` amount along the path defined by `prev`.*/
static void augment_flow(
		const struct linear_network *linear_network,
		struct residual_network *residual_network,
	        const u32 source,
		const u32 target,
		const arc_t *prev,
		s64 flow)
{
	u32 cur = target;
	
	while(cur!=source)
	{
		assert(cur < tal_count(prev));
		const arc_t arc = prev[cur];
		const arc_t dual = arc_dual(arc);
		
		assert(arc.idx < tal_count(residual_network->cap));
		assert(dual.idx < tal_count(residual_network->cap));
		
		residual_network->cap[arc.idx] -= flow;
		residual_network->cap[dual.idx] += flow;
		
		assert(residual_network->cap[arc.idx] >=0 );
		
		// we are traversing in the opposite direction to the flow,
		// hence the next node is at the tail of the arc.
		cur = arc_tail(linear_network,arc);
	}
}


// TODO(eduardo): unit test this
/* Finds any flow that satisfy the capacity and balance constraints of the
 * uncertainty network. For the balance function condition we have:
 * 	balance(source) = - balance(target) = amount
 * 	balance(node) = 0 , for every other node
 * Returns an error code if no feasible flow is found.
 * 
 * 13/04/2023 This implementation uses a simple augmenting path approach.
 * */
static int find_feasible_flow(
		const struct linear_network *linear_network,
		struct residual_network *residual_network,
		const u32 source, 
		const u32 target,
		s64 amount)
{
	assert(amount>=0);
	
	tal_t *this_ctx = tal(tmpctx,tal_t);
	int ret = RENEPAY_ERR_OK;
	
	/* path information 
	 * prev: is the id of the arc that lead to the node. */
	arc_t *prev = tal_arr(this_ctx,arc_t,linear_network->max_num_nodes);
	
	while(amount>0)
	{
		// find a path from source to target
		int err = find_admissible_path(
					linear_network,
					residual_network,source,target,prev);
		
		if(err!=RENEPAY_ERR_OK)
		{
			ret = RENEPAY_ERR_NOFEASIBLEFLOW;
			break;
		}
		
		// traverse the path and see how much flow we can send
		s64 delta = get_augmenting_flow(linear_network,
						residual_network,
						source,target,prev);
		
		// commit that flow to the path
		delta = MIN(amount,delta);
		augment_flow(linear_network,residual_network,source,target,prev,delta);
		
		assert(delta>0 && delta<=amount);
		amount -= delta;
	}
	
	tal_free(this_ctx);	
	return ret;
}

// TODO(eduardo): unit test this
/* Similar to `find_admissible_path` but use Dijkstra to optimize the distance
 * label. Stops when the target is hit. */
static int  find_optimal_path(
		const struct linear_network *linear_network,
		const struct residual_network* residual_network,
		const u32 source,
		const u32 target,
		arc_t *prev)
{
	tal_t *this_ctx = tal(tmpctx,tal_t);
	int ret = RENEPAY_ERR_NOFEASIBLEFLOW;
	
	bitmap *visited = tal_arrz(this_ctx, bitmap,
					BITMAP_NWORDS(linear_network->max_num_nodes));

	for(size_t i=0;i<tal_count(prev);++i)
		prev[i].idx=INVALID_INDEX;
	
	s64 const * const distance=dijkstra_distance_data();
	
	dijkstra_init();
	dijkstra_update(source,0);
	
	while(!dijkstra_empty())
	{
		u32 cur = dijkstra_top();
		dijkstra_pop();
		
		if(bitmap_test_bit(visited,cur))
			continue;
		
		bitmap_set_bit(visited,cur);
		
		if(cur==target)
		{
			ret = RENEPAY_ERR_OK;
			break;
		}
		
		for(arc_t arc = node_adjacency_begin(linear_network,cur);
		        !node_adjacency_end(arc);
			arc = node_adjacency_next(linear_network,arc))
		{
			// check if this arc is traversable
			if(residual_network->cap[arc.idx] <= 0)
				continue;
			
			u32 next = arc_head(linear_network,arc);
			
			s64 cij = residual_network->cost[arc.idx] 
					- residual_network->potential[cur] 
					+ residual_network->potential[next];
			
			// Dijkstra only works with non-negative weights
			assert(cij>=0);
			
			if(distance[next]<=distance[cur]+cij)
				continue;
			
			dijkstra_update(next,distance[cur]+cij);
			prev[next]=arc;
		}
	}
	tal_free(this_ctx);	
	return ret;
}
    
/* Set zero flow in the residual network. */
static void zero_flow(
		const struct linear_network *linear_network,
		struct residual_network *residual_network)
{
	for(u32 node=0;node<linear_network->max_num_nodes;++node)
	{
		residual_network->potential[node]=0;
		for(arc_t arc=node_adjacency_begin(linear_network,node);
			  !node_adjacency_end(arc);
			  arc = node_adjacency_next(linear_network,arc))
		{
			if(arc_is_dual(arc))continue;
			
			arc_t dual = arc_dual(arc);
			
			residual_network->cap[arc.idx] = linear_network->capacity[arc.idx];
			residual_network->cap[dual.idx] = 0;
		}
	}
}

// TODO(eduardo): unit test this
/* Starting from a feasible flow (satisfies the balance and capacity
 * constraints), find a solution that minimizes the network->cost function. 
 * 
 * TODO(eduardo) The MCF must be called several times until we get a good
 * compromise between fees and probabilities. Instead of re-computing the MCF at
 * each step, we might use the previous flow result, which is not optimal in the
 * current iteration but I might be not too far from the truth.
 * It comes to mind to use cycle cancelling. */
static int optimize_mcf(
		const struct linear_network *linear_network,
		struct residual_network *residual_network,
		const u32 source,
		const u32 target,
		const s64 amount)
{
	assert(amount>=0);
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	int ret = RENEPAY_ERR_OK;
	
	zero_flow(linear_network,residual_network);
	arc_t *prev = tal_arr(this_ctx,arc_t,linear_network->max_num_nodes);
	
	s64 const*const distance = dijkstra_distance_data();
	
	s64 remaining_amount = amount;
	
	while(remaining_amount>0)
	{
		int err = find_optimal_path(linear_network,residual_network,source,target,prev);
		if(err!=RENEPAY_ERR_OK)
		{
			// unexpected error
			ret = RENEPAY_ERR_NOFEASIBLEFLOW;
			break;
		}
		
		// traverse the path and see how much flow we can send
		s64 delta = get_augmenting_flow(linear_network,residual_network,source,target,prev);
		
		// commit that flow to the path
		delta = MIN(remaining_amount,delta);
		augment_flow(linear_network,residual_network,source,target,prev,delta);
		
		assert(delta>0 && delta<=remaining_amount);
		remaining_amount -= delta;
		
		// update potentials
		for(u32 n=0;n<linear_network->max_num_nodes;++n)
		{
			// see page 323 of Ahuja-Magnanti-Orlin
			residual_network->potential[n] -= MIN(distance[target],distance[n]);
			
			// Notice:
			// if node i is permanently labeled we have
			// 	d_i<=d_t 
			// which implies
			// 	MIN(d_i,d_t) = d_i
			// if node i is temporarily labeled we have
			// 	d_i>=d_t
			// which implies
			// 	MIN(d_i,d_t) = d_t
		}
	}
	tal_free(this_ctx);
	return ret;
}

// flow on directed channels
struct chan_flow
{
	s64 half[2];
};

/* Search in the network a path of positive flow until we reach a node with
 * positive balance. */
static u32 find_positive_balance(
		const struct gossmap *gossmap,
		const struct chan_flow *chan_flow,
		const u32 start_idx,
		const s64 *balance,
		
		struct gossmap_chan const** prev_chan,
		int *prev_dir,
		u32 *prev_idx)
{
	u32 final_idx = start_idx;
	
	/* TODO(eduardo)
	 * This is guaranteed to halt if there are no directed flow cycles. 
	 * There souldn't be any. In fact if cost is strickly
	 * positive, then flow cycles do not exist at all in the
	 * MCF solution. But if cost is allowed to be zero for
	 * some arcs, then we might have flow cyles in the final
	 * solution. We must somehow ensure that the MCF
	 * algorithm does not come up with spurious flow cycles. */
	while(balance[final_idx]<=0)
	{
		// printf("%s: node = %d\n",__PRETTY_FUNCTION__,final_idx);
		u32 updated_idx=INVALID_INDEX;
		struct gossmap_node *cur
			= gossmap_node_byidx(gossmap,final_idx);
		
		for(size_t i=0;i<cur->num_chans;++i)
		{
			int dir;
			struct gossmap_chan const *c 
				= gossmap_nth_chan(gossmap,
				                   cur,i,&dir);
			
			// TODO(eduardo): do we check if the channel is public?
			if (!gossmap_chan_set(c,dir))
				continue;
			
			const u32 c_idx = gossmap_chan_idx(gossmap,c);
			
			// follow the flow
			if(chan_flow[c_idx].half[dir]>0)
			{
				const struct gossmap_node *next
					= gossmap_nth_node(gossmap,c,!dir);
				u32 next_idx = gossmap_node_idx(gossmap,next);
				
				
				prev_dir[next_idx] = dir;
				prev_chan[next_idx] = c;
				prev_idx[next_idx] = final_idx;
				
				updated_idx = next_idx;
				break;
			}
		}
		
		assert(updated_idx!=INVALID_INDEX);
		assert(updated_idx!=final_idx);
		// printf("%s: balance[%d] = %ld\n",__PRETTY_FUNCTION__,
		//	updated_idx,balance[updated_idx]);
		
		final_idx = updated_idx;
	}
	return final_idx;
}

struct list_data
{
	struct list_node list;
	struct flow *flow_path;
};

// TODO(eduardo): check this
/* Given a flow in the residual network, build a set of payment flows in the
 * gossmap that corresponds to this flow. */		
static struct flow **
	get_flow_paths(
		const tal_t *ctx,
		const struct gossmap *gossmap,
		
		// chan_extra_map cannot be const because we use it to keep
		// track of htlcs and in_flight sats.
		struct chan_extra_map *chan_extra_map,
		const struct linear_network *linear_network,
		const struct residual_network *residual_network)
{
	// printf("%s: starting\n",__PRETTY_FUNCTION__);
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	const size_t max_num_chans = gossmap_max_chan_idx(gossmap);
	struct chan_flow *chan_flow = tal_arrz(this_ctx,struct chan_flow,max_num_chans);
	
	const size_t max_num_nodes = gossmap_max_node_idx(gossmap);
	s64 *balance = tal_arrz(this_ctx,s64,max_num_nodes);
	
	struct gossmap_chan const **prev_chan 
		= tal_arr(this_ctx,struct gossmap_chan const*,max_num_nodes);
	
	int *prev_dir = tal_arr(this_ctx,int,max_num_nodes);
	u32 *prev_idx = tal_arr(this_ctx,u32,max_num_nodes);
	
	// Convert the arc based residual network flow into a flow in the
	// directed channel network.
	// Compute balance on the nodes.
	for(u32 n = 0;n<max_num_nodes;++n)
	{
		for(arc_t arc = node_adjacency_begin(linear_network,n);
		        !node_adjacency_end(arc);
			arc = node_adjacency_next(linear_network,arc))
		{
			if(arc_is_dual(arc))
				continue;
			u32 m = arc_head(linear_network,arc);
			s64 flow = get_arc_flow(residual_network,arc);
			
			balance[n] -= flow;
			balance[m] += flow;
			
			chan_flow[arc.chanidx].half[arc.chandir] +=flow;
		}
			
	}
	
	
	size_t num_paths=0;
	tal_t *list_ctx = tal(this_ctx,tal_t);
	LIST_HEAD(path_list);
	struct list_data *ld;
	
	// Select all nodes with negative balance and find a flow that reaches a
	// positive balance node.
	for(u32 node_idx=0;node_idx<max_num_nodes;++node_idx)
	{
		// for(size_t i=0;i<tal_count(prev_idx);++i)
		// {
		// 	prev_idx[i]=INVALID_INDEX;
		// }
		// this node has negative balance, flows leaves from here
		while(balance[node_idx]<0)
		{	
			prev_chan[node_idx]=NULL;
			u32 final_idx = find_positive_balance(gossmap,chan_flow,node_idx,balance,
							prev_chan,prev_dir,prev_idx);
		
			s64 delta=-balance[node_idx];
			int length = 0;
			delta = MIN(delta,balance[final_idx]);
			
			// walk backwards, get me the length and the max flow we
			// can send.
			for(u32 cur_idx = final_idx;
			    cur_idx!=node_idx;
			    cur_idx=prev_idx[cur_idx])
			{
				assert(cur_idx!=INVALID_INDEX);
				
				const int dir = prev_dir[cur_idx];
				struct gossmap_chan const * const c = prev_chan[cur_idx];
				const u32 c_idx = gossmap_chan_idx(gossmap,c);
				
				delta=MIN(delta,chan_flow[c_idx].half[dir]);
				length++;
				
				// TODO(eduardo) does htlc_max has any relevance
				// here?
				delta = MIN(delta,chan_flow[c_idx].half[dir]);
			}
			
			
			struct flow *fp = tal(list_ctx,struct flow);
			fp->path = tal_arr(fp,struct gossmap_chan const*,length);
			fp->dirs = tal_arr(fp,int,length);
			
			balance[node_idx] += delta;
			balance[final_idx]-= delta;
			
			// walk backwards, substract flow
			for(u32 cur_idx = final_idx;
			    cur_idx!=node_idx;
			    cur_idx=prev_idx[cur_idx])
			{
				assert(cur_idx!=INVALID_INDEX);
				
				const int dir = prev_dir[cur_idx];
				struct gossmap_chan const * const c = prev_chan[cur_idx];
				const u32 c_idx = gossmap_chan_idx(gossmap,c);
				
				length--;
				fp->path[length]=c;
				fp->dirs[length]=dir;
				// notice: fp->path and fp->dirs have the path
				// in the correct order.
				
				chan_flow[c_idx].half[prev_dir[cur_idx]]-=delta;
			}
			
			// complete the flow path by adding real fees and
			// probabilities.
			flow_complete(fp,gossmap,chan_extra_map,amount_msat(delta*1000));
			
			// add fp to list
			ld = tal(list_ctx,struct list_data);
			ld->flow_path = fp;
			list_add(&path_list,&ld->list);
			num_paths++;
		}
	}
	
	// copy the list into the array we are going to return	
	struct flow **flows = tal_arr(ctx,struct flow*,num_paths);
	size_t pos=0;
	list_for_each(&path_list,ld,list)
	{
		flows[pos++] = tal_steal(flows,ld->flow_path);
	}
	
	tal_free(this_ctx);
	// printf("%s: done\n",__PRETTY_FUNCTION__);
	return flows;
}

/* Given the constraints on max fee and min prob.,
 * is the flow A better than B? */
static bool is_better(
		struct amount_msat max_fee,
		double min_probability,
		
		struct amount_msat A_fee,
		double A_prob,
		
		struct amount_msat B_fee,
		double B_prob)
{
	bool A_fee_pass = amount_msat_less_eq(A_fee,max_fee);
	bool B_fee_pass = amount_msat_less_eq(B_fee,max_fee);
	bool A_prob_pass = A_prob >= min_probability;
	bool B_prob_pass = B_prob >= min_probability;
	
	// all bounds are met
	if(A_fee_pass && B_fee_pass && A_prob_pass && B_prob_pass)
	{
		// prefer lower fees
		return amount_msat_less_eq(A_fee,B_fee);
	}
	
	// prefer the solution that satisfies both bounds
	if((!A_fee_pass || !A_prob_pass) && (B_fee_pass && B_prob_pass))
	{
		return false;
	}
	// prefer the solution that satisfies both bounds
	if((A_fee_pass && A_prob_pass) && (!B_fee_pass || !B_prob_pass))
	{
		return true;
	}
	
	// no solution satisfies both bounds
	
	// bound on fee is met
	if(A_fee_pass && B_fee_pass)
	{
		// pick the highest prob.
		return A_prob > B_prob;
	}
	
	// bound on prob. is met
	if(A_prob_pass && B_prob_pass)
	{
		// pick the lowest fee
		return amount_msat_less_eq(A_fee,B_fee);
	}
	
	// prefer the solution that satisfies the bound on fees
	if(A_fee_pass)
	{
		return true;
	}
	if(B_fee_pass)
	{
		return false;
	}
	
	// none of them satisfy the fee bound
	
	// prefer the solution that satisfies the bound on prob.
	if(A_prob_pass)
	{
		return true;
	}
	if(B_prob_pass)
	{
		return true;
	}
	
	// no bound whatsoever is satisfied
	// go for fees
	return amount_msat_less_eq(A_fee,B_fee);
}


// TODO(eduardo): choose some default values for the minflow parameters
/* eduardo: I think it should be clear that this module deals with linear
 * flows, ie. base fees are not considered. Hence a flow along a path is
 * described with a sequence of directed channels and one amount. 
 * In the `pay_flow` module there are dedicated routes to compute the actual
 * amount to be forward on each hop. 
 * 
 * TODO(eduardo): notice that we don't pay fees to forward payments with local
 * channels and we can tell with absolute certainty the liquidity on them. 
 * Check that local channels have fee costs = 0 and bounds with certainty (min=max). */
struct flow** minflow(
		const tal_t *ctx,
		struct gossmap *gossmap,
		const struct gossmap_node *source,
		const struct gossmap_node *target,
		struct chan_extra_map *chan_extra_map,
		const bitmap *disabled,
		struct amount_msat amount,
		struct amount_msat max_fee,
		double min_probability,
		double delay_feefactor,
		double base_fee_penalty,
		u32 prob_cost_factor )
{
	printf("%s: starting\n",__PRETTY_FUNCTION__);
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	struct pay_parameters *params = tal(this_ctx,struct pay_parameters);	
	
	params->gossmap = gossmap;
	params->source = source;
	params->target = target;
	params->chan_extra_map = chan_extra_map;
	
	params->disabled = disabled;
	assert(!disabled
	       || tal_bytelen(disabled) == bitmap_sizeof(gossmap_max_chan_idx(gossmap)));
	       
	params->amount = amount;
	
	// template the channel partition into linear arcs
	params->cap_fraction[0]=0;
	params->cost_fraction[0]=0;
	for(size_t i =0;i<CHANNEL_PARTS;++i)
	{
		params->cap_fraction[i]=CHANNEL_PIVOTS[i]-CHANNEL_PIVOTS[i-1];
		params->cost_fraction[i]=
			log((1-CHANNEL_PIVOTS[i-1])/(1-CHANNEL_PIVOTS[i]))
			/params->cap_fraction[i];
		
		// printf("channel part: %ld, fraction: %lf, cost_fraction: %lf\n",
		//	i,params->cap_fraction[i],params->cost_fraction[i]);
	}
	
	params->max_fee = max_fee;
	params->min_probability = min_probability;
	params->delay_feefactor = delay_feefactor;
	params->base_fee_penalty = base_fee_penalty;
	params->prob_cost_factor = prob_cost_factor;
	
	// build the uncertainty network with linearization and residual arcs
	struct linear_network *linear_network= tal(this_ctx,struct linear_network);
	init_linear_network(params,linear_network);
	
	struct residual_network *residual_network = tal(this_ctx,struct residual_network);
	alloc_residual_netork(linear_network,residual_network);
	
	dijkstra_malloc(this_ctx,gossmap_max_node_idx(params->gossmap));
	
	const u32 target_idx = gossmap_node_idx(params->gossmap,target);
	const u32 source_idx = gossmap_node_idx(params->gossmap,source);
	
	init_residual_netork(linear_network,residual_network);
	
	printf("%s: done with allocation and initialization\n",__PRETTY_FUNCTION__);
	
	struct amount_msat best_fee;
	double best_prob_success;
	struct flow **best_flow_paths = NULL;
	
	printf("%s: searching for a feasible flow\n",__PRETTY_FUNCTION__);
	int err = find_feasible_flow(linear_network,residual_network,source_idx,target_idx,
	                             params->amount.millisatoshis/1000);
	
	if(err!=RENEPAY_ERR_OK)
	{
		// there is no flow that satisfy the constraints, we stop here
		printf("%s: feasible flow not found\n",__PRETTY_FUNCTION__);
		goto finish;
	}
	printf("%s: found a feasible flow\n",__PRETTY_FUNCTION__);
	
	// first flow found
	best_flow_paths = get_flow_paths(ctx,params->gossmap,params->chan_extra_map,
	                            linear_network,residual_network);
	best_prob_success = flow_set_probability(best_flow_paths,
						params->gossmap,
						params->chan_extra_map);
	best_fee = flows_fee(best_flow_paths);
	
	// binary search for a value of `mu` that fits our fee and prob.
	// constraints.
	// mu=0 corresponds to only probabilities
	// mu=MU_MAX-1 corresponds to only fee
	s64 mu_left = 0, mu_right = MU_MAX;
	while(mu_left<mu_right)
	{
		
		s64 mu = (mu_left + mu_right)/2;
		printf("%s: mu=%ld\n",__PRETTY_FUNCTION__,mu);
		
		combine_cost_function(linear_network,residual_network,mu);
		
		optimize_mcf(linear_network,residual_network,
				source_idx,target_idx,params->amount.millisatoshis/1000);
		
		struct flow **flow_paths;
		flow_paths = get_flow_paths(this_ctx,params->gossmap,params->chan_extra_map,
		                            linear_network,residual_network);
		
		double prob_success = flow_set_probability(
						flow_paths,
						params->gossmap,
						params->chan_extra_map);
		struct amount_msat fee = flows_fee(flow_paths);
		
		printf("prob %.2f, fee %s\n",prob_success,
				type_to_string(this_ctx,struct amount_msat,&fee));
		
		// is this better than the previous one?
		if(!best_flow_paths || 
			is_better(params->max_fee,params->min_probability,
				  fee,prob_success,
			          best_fee, best_prob_success))
		{
			best_flow_paths = tal_steal(ctx,flow_paths);
			best_fee = fee;
			best_prob_success=prob_success;
			flow_paths = NULL;
		}
		
		if(amount_msat_greater(fee,params->max_fee))
		{
			// too expensive
			mu_left = mu+1;
			printf("%s: too expensive\n",__PRETTY_FUNCTION__);
		}else if(prob_success < params->min_probability)
		{
			// too unlikely
			mu_right = mu;
			printf("%s: too unlikely\n",__PRETTY_FUNCTION__);
		}else
		{
			// with mu constraints are satisfied, now let's optimize
			// the fees
			mu_left = mu+1;
		}
		
		if(flow_paths)
			tal_free(flow_paths);
	}
	
	
	
	finish:
	
	printf("%s: finished\n",__PRETTY_FUNCTION__);
	
	tal_free(this_ctx);
	return best_flow_paths;
}

