#include "config.h"
#include <plugins/renepay/mcf.h>
#include <plugins/renepay/flow.h>
#include <math.h>

#define PARTS_BITS 2
#define CHANNEL_PARTS (1 << PARTS_BITS)
static const double CHANNEL_PIVOTS[]={0,0.5,0.8,0.95};

// how many bits for linearization parts plus 1 bit for the direction of the
// channel plus 1 bit for the dual representation.
static const size_t ARC_ADDITIONAL_BITS = PARTS_BITS + 2;

static const s64 INFINITE = 9e18;
static const u32 INVALID_INDEX=0xffffffff;
static const s64 COST_FACTOR=10;
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
 * Then for each outgoing channel `(c,!half)` there will
 * be 4 parts for the actual residual capacity, hence
 * with the dual bit set to 0:
 * 
 * 	(c,!half,0,0)
 * 	(c,!half,1,0)
 * 	(c,!half,2,0)
 * 	(c,!half,3,0)
 * 	
 * and also we need to consider the dual arcs
 * corresponding to the channel direction `(c,half)`
 * (the dual has reverse direction):
 * 
 * 	(c,half,0,1)
 * 	(c,half,1,1)
 * 	(c,half,2,1)
 * 	(c,half,3,1)
 *  
 * These are the 8 outgoing arcs relative to `node` and
 * associated with channel `c`. The incoming arcs will
 * be:
 * 
 * 	(c,half,0,0)
 * 	(c,half,1,0)
 * 	(c,half,2,0)
 * 	(c,half,3,0)
 * 
 * 	(c,!half,0,1)
 * 	(c,!half,1,1)
 * 	(c,!half,2,1)
 * 	(c,!half,3,1)
 * 	
 * but they will be stored as outgoing arcs on the peer
 * node `next`.
 * 
 * I hope this will clarify my future self when I forget.
 * 
 * */

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct pay_parameters {
	/* The gossmap we are using */
	struct gossmap *gossmap;
	struct gossmap_node *source;
	struct gossmap_node *target;
	
	/* Extra information we intuited about the channels */
	struct chan_extra_map *chan_extra_map;
	
	/* Optional bitarray of disabled chans. */
	// TODO: did you consider these?, hint: disable channels by ignoring
	// them when constructing the adjacency list of nodes in
	// `init_residual_network`.
	const bitmap *disabled;
	
	struct amount_msat total_payment;
	
	u32 *arc_head_node; 
	// notice that a tail node is not needed, 
	// because the tail of arc is the head of dual(arc)
	
	// probability cost associated to an arc
	s64 *arc_prob_cost;
	
	u32 *arc_adjacency_next_arc;
	u32 *node_adjacency_first_arc;	
	
	// channel linearization parameters
	double cap_fraction[CHANNEL_PARTS], 
	       cost_fraction[CHANNEL_PARTS];
};

struct residual_network {
	/* residual capacity on arcs */
	s64 *cap; 
	s64 *cost;
};
				

/* Helper function. 
 * Given an arc idx, return the dual's idx in the residual network. */
static inline u32 arc_dual(const struct pay_parameters * params UNUSED,
                           const u32 arc)
{
	return arc ^ 1;
}

/* Helper function. 
 * Given an arc idx, return the node from which this arc emanates in the residual network. */
static inline u32 arc_tail(const struct pay_parameters *params,
                           const u32 arc)
{
	return params->arc_head_node[ arc_dual(params,arc) ];
}
/* Helper function. 
 * Given an arc idx, return the node that this arc is pointing to in the residual network. */
static inline u32 arc_head(const struct pay_parameters *params,
                           const u32 arc)
{
	return params->arc_head_node[arc];
}
		
/* Helper function. 
 * Given node idx `node`, return the idx of the first arc whose tail is `node`.
 * */
static inline u32 node_adjacency_begin(const struct pay_parameters *params,
                                       const u32 node)
{
	return params->node_adjacency_first[node];
}

/* Helper function. 
 * Given node idx `node`, return the idx of one past the last arc whose tail is `node`. */
static inline u32 node_adjacency_end(const struct pay_parameters *params UNUSED,
                                     const u32 node UNUSED)
{
	return INVALID_INDEX;
}

/* Helper function. 
 * Given node idx `node` and `arc`, returns the idx of the next arc whose tail is `node`. */
static inline u32 node_adjacency_next(const struct pay_parameters *params,
                                     const u32 node UNUSED,
				     const u32 arc)
{
	return params->adjacency_next[arc];
}

/* Helper function.
 * Given an arc index, we should be able to deduce the channel id, and part that
 * corresponds to this arc. */
static inline u32 arc_to_channel_idx(const u32 arc,
                              int *half_ptr,
			      int *part_ptr,
			      int *dual_ptr)
{
	&half_ptr = (arc >> (1+PARTS_BITS)) & 1;
	&part_ptr = (arc >> 1) & (CHANNEL_PARTS-1);
	&dual_ptr = arc & 1;
	return arc>>ARC_ADDITIONAL_BITS;
}

/* Helper function.
 * Given a channel index, we should be able to deduce the arc id. */
static inline u32 channel_idx_to_arc(const u32 chan_idx,
                              int half,
			      int part,
			      int dual)
{
	half &= 1;
	dual &= 1;
	part &= (CHANNEL_PARTS-1);
	return (chan_idx << ARC_ADDITIONAL_BITS)|(half<<(1+PARTS_BITS))|(part<<1)|(dual);
}

/* Split a directed channel into parts with linear cost function. */
static void linearize_channel(
		const struct pay_parameters *params,
		const struct gossmap_chan *c,
		const int dir,
		s64 *capacity,
		s64 *cost)
{
	struct chan_extra_half *extra_half = get_chan_extra_half_by_chan(
							params->gossmap,
							params->chan_extra_map,
							c,
							dir);
	
	s64 a = extra_half->known_min.millisatoshis/1000,
	    b = extra_half->known_max.millisatoshis/1000;
	
	capacity[0]=a;
	cost[0]=0;
	for(size_t i=1;i<CHANNEL_PARTS;++i)
	{
		capacity[i] = params->cap_fraction[i]*(b-a);
		cost[i] = params->cost_fraction[i]*COST_FACTOR*params->total_payment.millisatoshis;
	}
}

/* Get the fee cost associated to this directed channel. 
 * Cost is expressed as PPM of the payment. */
static s64 compute_fee_cost(
		const struct pay_parameters *params,
		const struct gossmap_chan *c,
		const int dir)
{
	s64 pfee = c->half[dir].proportional_fee,
	    bfee = c->half[dir].base_fee;
	
	// Base fee to proportional fee. We want
	// 	cost_eff(x) >= cost_real(x)
	// 	x * (p + b*a) >= x * p + b*1000
	// where the effective proportional cost is `(p+b*a)`, `a` is factor we
	// need to choose. Then
	// 	x >= 1000/a
	// therefore if `a`=1, our effective cost is good only after 1000 sats,
	// otherwise the cost is underestimated. If we want to make it valid
	// from 1 sat, then a=1000, but that means that setting a base fee of
	// 1msat will be consider like having a proportional cost of 1000ppm,
	// which is a lot. Some middle ground can be obtained with numbers in
	// between, but in general `a` does not make sense above 1000.
	
	// In this case having a base fee of 1 is equivalent of having a
	// proportional fee of 10 ppm.
	return pfee + bfee * 10;
	
	// TODO(eduardo) How to convert the base fee properly into an effective
	// proportional fee?
}

static void init_residual_network(struct pay_parameters *params,
                                  struct residual_network *network)
{
	const size_t max_num_chans = gossmap_max_chan_idx(params->gossmap);
	const size_t max_num_arcs = max_num_chans << ARC_ADDITIONAL_BITS;
	const size_t max_num_nodes = gossmap_max_node_idx(params->gossmap);
	
	network->cap = tal_arr(network,s64,max_num_arcs);
	for(size_t i=0;i<tal_count(network->cap);++i)
		network->cap[i]=0;
	
	network->cost = tal_arr(network,s64,max_num_arcs);
	for(size_t i=0;i<tal_count(network->cost);++i)
		network->cost[i]=INFINITE;
	
	params->arc_head_node = tal_arr(params,u32,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_head_node);++i)
		params->arc_head_node[i]=INVALID_INDEX;
	
	params->arc_prob_cost = tal_arr(params,s64,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_prob_cost);++i)
		params->arc_prob_cost[i]=INFINITE;
		
	params->arc_adjacency_next_arc = tal_arr(params,u32,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_adjacency_next_arc);++i)
		params->arc_adjacency_next_arc[i]=INVALID_INDEX;
	
	params->node_adjacency_first_arc = tal_arr(params,u32,max_num_nodes);
	for(size_t i=0;i<tal_count(params->node_adjacency_first_arc);++i)
		params->node_adjacency_first_arc[i]=INVALID_INDEX;
	
	s64 capacity[CHANNEL_PARTS],prob_cost[CHANNEL_PARTS];
	
	struct gossmap_node *node = gossmap_first_node(params->gossmap);
	const size_t num_nodes = gossmap_num_nodes(params->gossmap);
	for(size_t i=0; i<num_nodes; ++i, node = gossmap_next_node(params->gossmap,node))
	{
		const u32 node_id = gossmap_node_idx(params->gossmap,node);
		
		u32 prev_arc = INVALID_INDEX;
		
		for(size_t j=0;j<node->num_chans;++j)
		{
			int half;
			const struct gossmap_chan *c = gossmap_nth_chan(params->gossmap,
			                                                node, j, &half);
			const u32 chan_id = gossmap_chan_idx(params->gossmap, c);
			
			// TODO: question here, is the pair `(c,!half)` identified
			// with the outgoing channel from `node` or the incoming
			// channel?
			
			const struct gossmap_node *next = gossmap_nth_node(params->gossmap,
									   c,!half);
			const u32 next_id = gossmap_node_idx(params->gossmap,next);
			
			ASSERT(node_id!=next_id);
			
			// `cost` is the word normally used to denote cost per
			// unit of flow in the context of MCF.
			s64 prob_cost[CHANNEL_PARTS], capacity[CHANNEL_PARTS];
			
			// split this channel direction to obtain the arcs
			// that are outgoing to `node`
			linearize_channel(params,c,!half,capacity,prob_cost);
			
			// let's subscribe the 4 parts of the channel direction
			// (c,!half), the dual of these guys will be subscribed
			// when the `i` hits the `next` node.
			for(size_t k=0;k<CHANNEL_PARTS;++k)
			{
				u32 arc = channel_idx_to_arc(chan_id,!half,k,0);
				arc_head_node[arc] = next_id;
				
				// Is this is the first arc?
				if(prev_arc==INVALID_INDEX)
				// yes, then set it at the head of the linked list
				{
					node_adjacency_first_arc[node_id]=arc;
				} else
				// no, then link it to the previous arc
				{
					arc_adjacency_next_arc[prev_arc]=arc;
				}
				
				prev_arc = arc;
				
				network->cap[arc] = capacity[k];
				params->arc_prob_cost[arc] = prob_cost[k];
				params->fee_cost[arc] = compute_fee_cost(params,c,!half);
			}
			
			// split the opposite direction to obtain the dual arcs
			// that are outgoing to `node`
			linearize_channel(params,c,half,capacity,prob_cost);
			
			// let's subscribe the 4 parts of the channel direction
			// (c,half) in dual representation
			for(size_t k=0;k<CHANNEL_PARTS;++k)
			{
				u32 arc = channel_idx_to_arc(chan_id,half,k,1);
				arc_head_node[arc] = next_id;
				
				// Is this is the first arc?
				if(prev_arc==INVALID_INDEX)
				// yes, then set it at the head of the linked list
				{
					node_adjacency_first_arc[node_id]=arc;
				} else
				// no, then link it to the previous arc
				{
					arc_adjacency_next_arc[prev_arc]=arc;
				}
				
				prev_arc = arc;
				
				network->cap[arc] = 0;
				params->arc_prob_cost[arc] = -prob_cost[k];
				params->fee_cost[arc] = -compute_fee_cost(params,c,!half);
			}
		}
	}
}

/* Finds an admissible path from source to target, traversing arcs in the
 * residual network with capacity greater than 0. 
 * The path is encoded into prev, which contains the idx of the arcs that are
 * traversed. 
 * Returns RENEPAY_ERR_OK if the path exists. */
static int find_admissible_path(const struct pay_parameters *params,
			        struct residual_network *network,
                                const u32 source,
				const u32 target,
				u32 *prev)
{
	int status = RENEPAY_ERR_NOFEASIBLEFLOW;
	
	// TODO: memset(prev) = INVALID_INDEX
	
	// The graph is dense, and the farthest node is just a few hops away,
	// hence let's BFS search.
	queue Q; // TODO
	
	// TODO: Q.push(source);
	while // TODO: Q not empty
	{
		// TODO: u32 cur = Q.front(); Q.pop();
		if(cur==target)
		{
			status = RENEPAY_ERR_OK;
			break;
		}
		
		for(u32 arc = node_adjacency_begin(params,cur);
		        arc!= node_adjacency_end(params,cur);
			arc = node_adjacency_next(params,cur,arc))
		{
			// check if this arc is traversable
			if(network->cap[arc] <= 0)
				continue;
			
			u32 next = arc_head(params,arc);
			
			// if that node has been seen previously
			if(prev[next]!=INVALID_INDEX)
				continue;
			
			prev[next] = arc;
			// TODO: Q.push(next);
		}
	}
	
	return status;
}

/* Get the max amount of flow one can send from source to target along the path
 * encoded in `prev`. */
static s64 get_augmenting_flow(const struct pay_parameters *params,
			       struct residual_network *network,
	                       const u32 source,
			       const u32 target,
			       const u32 *prev)
{
	s64 flow = INFINITE;
	
	u32 cur = target;
	while(cur!=source)
	{
		const u32 arc = prev[cur];
		const u32 dual = arc_dual(params,arc);
		
		flow = MIN(flow , network->cap[arc]);
		
		// we are traversing in the opposite direction to the flow,
		// hence the next node is at the head of the `dual` arc.
		cur = arc_head(params,dual);
	}
	
	ASSERT(flow<INFINITE && flow>0);
	return flow;
}

/* Augment a `flow` amount along the path defined by `prev`.*/
static void augment_flow(const struct pay_parameters *params,
			 struct residual_network *network,
	                 const u32 source,
			 const u32 target,
			 const u32 *prev,
			 s64 flow)
{
	u32 cur = target;
	
	while(cur!=source)
	{
		const u32 arc = prev[cur];
		const u32 dual = arc_dual(params,arc);
		
		network->cap[arc] -= flow;
		ASSERT(network->cap[arc] >=0 );
		network->cap[dual] += flow;
		
		// we are traversing in the opposite direction to the flow,
		// hence the next node is at the head of the `dual` arc.
		cur = arc_head(params,dual);
	}
}

/* Finds any flow that satisfy the capacity and balance constraints of the
 * uncertainty network. For the balance function condition we have:
 * 	balance(source) = - balance(target) = amount
 * 	balance(node) = 0 , for every other node
 * Returns an error code if no feasible flow is found.
 * 
 * 13/04/2023 This implementation uses a simple augmenting path approach.
 * */
static int find_feasible_flow(const struct pay_parameters *params,
			      struct residual_network *network,
			      const u32 source, 
			      const u32 target,
			      s64 amount)
{
	int status = RENEPAY_ERR_OK;
	
	/* path information 
	 * prev: is the id of the arc that lead to the node. */
	// TODO: how do we encode the arc idx?
	u32 *prev = tal_arr(tmpctx,u32
		            gossmap_max_node_idx(params->gossmap));
	
	while(amount>0)
	{
		// find a path from source to target
		int err = find_admissible_path(params,network,source,target,prev);
		if(err!=RENEPAY_ERR_OK)
		{
			status = RENEPAY_ERR_NOFEASIBLEFLOW;
			break;
		}
		
		// traverse the path and see how much flow we can send
		s64 delta = get_augmenting_flow(params,network,source,target,prev);
		
		// commit that flow to the path
		delta = MIN(amount,delta);
		augment_flow(params,network,source,target,prev,delta);
		
		ASSERT(delta>0 && delta<=amount);
		amount -= delta;
	}
	
	tal_free(prev);	
	
	return status;
}

/* TODO(eduardo): How to combine probability cost and fee cost?
 * This is tricky, because we say that a fee cost is high when the ratio of fee
 * and actual payment is high, typically 1000ppm or more. 
 * On the other hand a probability cost is high if the probability of success is
 * very low; since cost = - log prob, we have
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
 * $ cost \approx 1-P $ when $ P > 0.9 $.
 * 
 * Interesting to see:
 * How much do we pay for reliability?
 * Cost_fee(most reliable solution) - Cost_fee(cheapest solution)
 * 
 * Interesting to see:
 * Is the most reliable path much more reliable than the cheapest?
 * Prob(reliable)/Prob(cheapest) = Exp(Cost_prob(cheapest)-Cost_prob(reliable))
 * 
 * When combining probability cost and fee cost, we need to understand that this
 * is like comparing apples to oranges. 
 * Probability cost is, for small values, equal to the probability of failure;
 * and we say this cost is acceptable if c(x) = 0.01 (1% probability of failure).
 * On the other hand fee cost is the money we pay for payment delivery and our
 * acceptance criteria depends on how much we want to send; so for instance a
 * fee cost is acceptable if c(x) = 100 T (we pay 100 ppm of T), 
 * where T is the total value we want to pay.
 * These numbers can be tuned; but the lesson is we evaluate probability cost on
 * its absolute value and fee cost relative to T. 
 * 
 * Notice also that the fee cost per unit flow is an integer number,
 * representing ppm (parts per million). On the other hand the first linear
 * arc with non zero prob. cost on a directed channel has a cost per unit flow
 * of around 1.3/(b-a), depending on the linearization partition (I prefer the
 * first partition to contain 50% of the (b-a) range, i.e. up to the point where
 * the probability of forwarding the flow reaches 50%), in essence this prob.
 * cost per unit of flow is a real number and we want to use algorithms (for
 * example cost scaling) that assume that cost is integer. 
 * 
 * How to solve both problems? 
 * Well, we know that we must combine both prob. cost and fee cost linearly and
 * we like the fact that fee cost is an integer. We can multiply the prob. cost
 * times a factor such that the prob. cost becomes becomes equivalent to the fee
 * cost, and approximate it to the nearest integer number in the process.
 * For instance, define a new prob. cost $ c_prob(x) = - 10^4 T log(P(x))$,
 * then when $1-P(x) \approx 1%$, i.e. the probability of failure is around 1%
 * is perceived as if we pay 100 ppm of the total payment T. 
 * 
 * Is this enough to make integer prob. cost per unit flow?
 * Let's see: 
 * 	-> consider an arc with $(b-a) >> 10^4 T$, recall for the first
 * 	partition $ c_prob(x) = x 1.3/(b-a) 10^4 T $, hence this arc has cost
 * 	per unit flow 0. Which is reasonable because T is very small and can be
 * 	sent through this arc without any issues.
 * 	
 * 	-> if $(b-a) \approx 10^4 T$ we obtain $c_prob(x) = x$ which is
 * 	equivalent to say we just pay 1 ppm for flows on this arc, which is non
 * 	zero but very small, and it is reasonable because $(b-a)$ is $10^4$
 * 	times bigger than T.
 * 	
 * 	-> if $(b-a)/2 \approx T$ then we start to enter a danger zone because
 * 	it is very likely that T cannot go through this arc with capacity $(b-a)/2$,
 * 	non-surprisingly the monetary cost associated to this case is
 * 	$c_prob(x) \approx 5000 x $, which is like 5000 ppm.
 * 	
 * 	-> for $(b-a)/2 < T$ we are paying an ever increasing price in ppm of T
 * 	per unit flow sent through this arc. The MCF algorithm will try keep
 * 	this fake monetary cost as low as possible.
 * 
 * Summary: we combine use as prob. cost the function $c_prob(x) = -10^4 T log(P(x))$,
 * and in this scheme we can approximate to integer numbers the cost per unit
 * flow, and 1% prob. of failure is equivalent to pay 100 ppm of the total
 * payment T. Then we can combine the fee cost $c_fee(x)$ and $c_prob(x)$ with
 * a factor mu ~ [0,1]. Instead of hard-coding the 10^4 factor, we use the
 * variable COST_FACTOR.
 * */

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
		      int *errorcode)
{
	struct pay_parameters *params = tal(tmpctx,struct pay_parameters);	
	
	params->gossmap = gossmap;
	params->source = source;
	params->target = target;
	params->chan_extra_map = chan_extra_map;
	params->disabled = disabled;
	params->total_payment = amount;
	
	params->cap_fraction[0]=0;
	params->cost_fraction[0]=0;
	for(size_t i =0;i<CHANNEL_PARTS;++i)
	{
		params->cap_fraction[i]=CHANNEL_PIVOTS[i]-CHANNEL_PIVOTS[i-1];
		params->cost_fraction[i]=
			log((1-CHANNEL_PIVOTS[i-1])/(1-CHANNEL_PIVOTS[i]))
			/params->cap_fraction[i];
	}
	
	// cap_fraction[i] = pivot[i]-pivot[i-1]
	// cost_fraction[i] = log((1-pivot[i-1])/(1-pivot[i]))/(pivot[i]-pivot[i-1])
	
	// TODO: handle max_fee_ppm, delay_feefactor and basefee_penalty
	// params->max_fee_ppm = max_fee_ppm;
	// params->delay_feefactor = delay_feefactor;
	// params->basefee_penalty = 
	
	// build the uncertainty network with linearization and residual arcs
	struct residual_network *network = tal(tmpctx,struct residual_network);
	init_residual_network(params,network);
	
	const u32 target_id = get_node_id(params,target);
	const u32 source_id = get_node_id(params,source);
	
	// TODO
	// 
	// -> network_flow = compute any feasible flow
	// -> if network_flow cannot be computed return an error code: 
	// 	No feasible flow
	// 	return NULL
	// 
	// range for mu: mu_left = 0, mu_right=MU_MAX
	// -> loop a binary seach for mu
	// 	-> mu = ( mu_left + mu_right )/ 2
	// 	-> set c(x) = (MU_MAX-1-mu)*c_prob(x) + mu*c_fee(x)
	// 	-> network_flow = refine network_flow to minimize costs
	// 	-> if fee ratio > max_fee_ppm:
	// 		mu_left = mu + 1
	// 	   else if prob < min_prob:
	// 	   	mu_right = mu
	// 	   else:
	// 	   	a good flow was found, break
	// 	-> if mu_left >= mu_right:
	// 		cannot find a cheap and reliable route
	// 		flow not found
	// 		break
	// 
	// // if flow not found fallback to c_prob or c_fee?
	// 		
	// -> flow = get flow paths from network_flow
	
	tal_free(network);
	tal_free(params);
	
	// TODO
	// return flow
}

