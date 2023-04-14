#include "config.h"
#include <plugins/renepay/mcf.h>

static const size_t PARTS_BITS = 2;
static const size_t CHANNEL_PARTS = 1 << PARTS_BITS;

// how many bits for linearization parts plus 1 bit for the direction of the
// channel plus 1 bit for the dual representation.
static const size_t ARC_ADDITIONAL_BITS = PARTS_BITS + 2;

static const s64 INFINITE = 9e18;
static const u32 INVALID_INDEX=0xffffffff;
			
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
	
	u32 *arc_tail_node, *arc_head_node;
	u32 *arc_adjacency_next_arc;
	
	u32 *node_adjacency_first_arc;	
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
	return params->arc_tail_node[arc];
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
static u32 arc_to_channel_idx(const u32 arc,
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
static u32 channel_idx_to_arc(const u32 chan_idx,
                              int half,
			      int part,
			      int dual)
{
	half &= 1;
	dual &= 1;
	part &= (CHANNEL_PARTS-1);
	return (chan_idx << ARC_ADDITIONAL_BITS)|(half<<(1+PARTS_BITS))|(part<<1)|(dual);
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
	
	params->arc_tail_node = tal_arr(params,u32,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_tail_node);++i)
		params->arc_tail_node[i]=INVALID_INDEX;
	
	params->arc_head_node = tal_arr(params,u32,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_head_node);++i)
		params->arc_head_node[i]=INVALID_INDEX;
		
	params->arc_adjacency_next_arc = tal_arr(params,u32,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_adjacency_next_arc);++i)
		params->arc_adjacency_next_arc[i]=INVALID_INDEX;
	
	params->node_adjacency_first_arc = tal_arr(params,u32,max_num_nodes);
	for(size_t i=0;i<tal_count(params->node_adjacency_first_arc);++i)
		params->node_adjacency_first_arc[i]=INVALID_INDEX;
	
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
			
			// let's subscribe the 4 parts of the channel direction
			// (c,!half), the dual of these guys will be subscribed
			// when the `i` hits the `next` node.
			for(size_t k=0;k<CHANNEL_PARTS;++k)
			{
				u32 arc = channel_idx_to_arc(chan_id,!half,k,0);
				arc_tail_node[arc] = node_id;
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
				
				// TODO: set costs and capacity
				// use (c,!half)
			}
			
			// let's subscribe the 4 parts of the channel direction
			// (c,half) in dual representation
			for(size_t k=0;k<CHANNEL_PARTS;++k)
			{
				u32 arc = channel_idx_to_arc(chan_id,half,k,1);
				arc_tail_node[arc] = node_id;
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
				
				// TODO: set costs and capacity
				// use (u,half)
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
		cur = arc_destination(params,dual);
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
		cur = arc_destination(params,dual);
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
	// -> select a default value for mu
	// -> loop
	// 	-> set the cost as a combination of prob. cost and fee cost
	// 	-> network_flow = refine network_flow to minimize costs
	// 	-> if fee ratio < max_fee_ppm break, else increase mu
	// 	-> if mu is beyond the limits then return an error code: 
	// 		cannot find a cheap route
	// 		break
	// 		
	// -> flow = get flow paths from network_flow
	
	tal_free(network);
	tal_free(params);
	
	// TODO
	// return flow
}

