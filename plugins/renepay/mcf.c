#include "config.h"
#include <ccan/lqueue/lqueue.h>
#include <plugins/renepay/mcf.h>
#include <plugins/renepay/flow.h>
#include <plugins/renepay/heap.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

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
static const s64 COST_FACTOR=10;
static const s64 MU_MAX = 128;

struct queue_data
{
	u32 idx;
	struct lqueue_link ql;
};
			
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
	// TODO(eduardo): did you consider these?, hint: disable channels by ignoring
	// them when constructing the adjacency list of nodes in
	// `init_residual_network`.
	const bitmap *disabled;
	
	// how much we pay
	struct amount_msat amount;
	
	u32 *arc_head_node; 
	// notice that a tail node is not needed, 
	// because the tail of arc is the head of dual(arc)
	
	// probability and fee cost associated to an arc
	s64 *arc_prob_cost, *arc_fee_cost;
	
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
    s64 *potential;
};
				

/* Helper function. 
 * Given an arc idx, return the dual's idx in the residual network. */
static u32 arc_dual(const struct pay_parameters * params UNUSED,
                           const u32 arc)
{
	return arc ^ 1;
}
/* Helper function. */
static bool arc_is_dual(
		const struct pay_parameters *params UNUSED,
		const u32 arc)
{
	return (arc & 1) == 1;
}

/* Helper function. 
 * Given an arc of the network (not residual) give me the flow. */
static s64 get_arc_flow(
			const struct pay_parameters * params,
			const struct residual_network *network,
			const u32 arc)
{
	assert(!arc_is_dual(arc));
	return network->cap[ arc_dual(params,arc) ];
}

/* Helper function. 
 * Given an arc idx, return the node from which this arc emanates in the residual network. */
static u32 arc_tail(const struct pay_parameters *params,
                           const u32 arc)
{
	return params->arc_head_node[ arc_dual(params,arc) ];
}
/* Helper function. 
 * Given an arc idx, return the node that this arc is pointing to in the residual network. */
static u32 arc_head(const struct pay_parameters *params,
                           const u32 arc)
{
	return params->arc_head_node[arc];
}
		
/* Helper function. 
 * Given node idx `node`, return the idx of the first arc whose tail is `node`.
 * */
static u32 node_adjacency_begin(const struct pay_parameters *params,
                                       const u32 node)
{
	return params->node_adjacency_first[node];
}

/* Helper function. 
 * Given node idx `node`, return the idx of one past the last arc whose tail is `node`. */
static u32 node_adjacency_end(const struct pay_parameters *params UNUSED,
                                     const u32 node UNUSED)
{
	return INVALID_INDEX;
}

/* Helper function. 
 * Given node idx `node` and `arc`, returns the idx of the next arc whose tail is `node`. */
static u32 node_adjacency_next(const struct pay_parameters *params,
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

/* Helper function.
 * Evaluate Pickhardt's probability cost function. */
static double pickhardt_probability_cost(
	double lim_low, double lim_high, double flow)
{
	if(flow<=lim_low)
		return 0.;
	ASSSERT(flow<lim_high);
	return -log(1.-(flow-lim_low)/(lim_high-lim_low));
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
		cost[i] = params->cost_fraction[i]*COST_FACTOR*params->amount.millisatoshis;
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
	
    network->potential = tal_arr(network,s64,max_num_nodes);
    for(size_t i=0;i<tal_count(network->potential);++i)
        network->potential[i]=0;
    
	params->arc_head_node = tal_arr(params,u32,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_head_node);++i)
		params->arc_head_node[i]=INVALID_INDEX;
	
	params->arc_prob_cost = tal_arr(params,s64,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_prob_cost);++i)
		params->arc_prob_cost[i]=INFINITE;
	
	params->arc_fee_cost = tal_arr(params,s64,max_num_arcs);
	for(size_t i=0;i<tal_count(params->arc_fee_cost);++i)
		params->arc_fee_cost[i]=INFINITE;
		
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
			
			const struct gossmap_node *next = gossmap_nth_node(params->gossmap,
									   c,!half);
			const u32 next_id = gossmap_node_idx(params->gossmap,next);
			
			assert(node_id!=next_id);
			
			// `cost` is the word normally used to denote cost per
			// unit of flow in the context of MCF.
			s64 prob_cost[CHANNEL_PARTS], capacity[CHANNEL_PARTS];
			
			// split this channel direction to obtain the arcs
			// that are outgoing to `node`
			linearize_channel(params,c,half,capacity,prob_cost);
			
			// let's subscribe the 4 parts of the channel direction
			// (c,half), the dual of these guys will be subscribed
			// when the `i` hits the `next` node.
			for(size_t k=0;k<CHANNEL_PARTS;++k)
			{
				u32 arc = channel_idx_to_arc(chan_id,half,k,0);
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
				params->arc_fee_cost[arc] = compute_fee_cost(params,c,half);
			}
			
			// split the opposite direction to obtain the dual arcs
			// that are outgoing to `node`
			linearize_channel(params,c,!half,capacity,prob_cost);
			
			// let's subscribe the 4 parts of the channel direction
			// (c,!half) in dual representation
			for(size_t k=0;k<CHANNEL_PARTS;++k)
			{
				u32 arc = channel_idx_to_arc(chan_id,!half,k,1);
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
				params->arc_fee_cost[arc] = -compute_fee_cost(params,c,!half);
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
			        const struct residual_network *network,
                                const u32 source,
				const u32 target,
				u32 *prev)
{
	int ret = RENEPAY_ERR_NOFEASIBLEFLOW;
	
	for(size_t i=0;i<tal_count(prev);++i)
		prev[i]=INVALID_INDEX;
	
	// The graph is dense, and the farthest node is just a few hops away,
	// hence let's BFS search.
	LQUEUE(struct queue_data,ql) myqueue = LQUEUE_INIT;
	struct queue_data *qdata;
	
	qdata = tal(tmpctx,struct queue_data);
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
			
			qdata = tal(tmpctx,struct queue_data);
			qdata->idx = next;
			lqueue_enqueue(&myqueue,qdata);
		}
	}
	
	return ret;
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
	
	assert(flow<INFINITE && flow>0);
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
		assert(network->cap[arc] >=0 );
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
	int ret = RENEPAY_ERR_OK;
	
	/* path information 
	 * prev: is the id of the arc that lead to the node. */
	u32 *prev = tal_arr(tmpctx,u32
		            gossmap_max_node_idx(params->gossmap));
	
	while(amount>0)
	{
		// find a path from source to target
		int err = find_admissible_path(params,network,source,target,prev);
		if(err!=RENEPAY_ERR_OK)
		{
			ret = RENEPAY_ERR_NOFEASIBLEFLOW;
			break;
		}
		
		// traverse the path and see how much flow we can send
		s64 delta = get_augmenting_flow(params,network,source,target,prev);
		
		// commit that flow to the path
		delta = MIN(amount,delta);
		augment_flow(params,network,source,target,prev,delta);
		
		assert(delta>0 && delta<=amount);
		amount -= delta;
	}
	
	tal_free(prev);	
	
	return ret;
}
	
/* Similar to `find_admissible_path` but use Dijkstra to optimize the distance
 * label. Stops when the target is hit. */
static int  find_optimal_path(
	const struct pay_parameters *params,
	const struct residual_network* network,
	const u32 source,
	const u32 target,
	s32 *prev,
	s64 *distance)
{
	tal_t *this_ctx = tal(tmpctx,tal_t);
	int ret = RENEPAY_ERR_NOFEASIBLEFLOW;
	
	char *visited = tal_array(this_ctx,char,tal_count(prev));
	
	for(size_t i=0;i<tal_count(prev);++i)
	{	
		prev[i]=INVALID_INDEX;
		distance[i]=INFINITE;
		visited[i]=0;
	}
	distance[source]=0;
	
	struct heap *myheap = heap_new(this_ctx);	
	heap_insert(myheap,source,0);
	
	while(!heap_empty(myheap))
	{
		struct heap_data *top = heap_top(myheap);
		u32 cur = top->idx;
		heap_pop(myheap);
		
		if (visited[cur])
			continue;
		
		visited[cur]=1;
		
		if(cur==target)
		{
			ret = RENEPAY_ERR_OK;
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
			
			s64 cij = network->cost[arc] - network->potential[cur]
			                             + network->potential[next];
			// Dijkstra only works with non-negative weights
			assert(cij>=0);
			
			if(distance[next]<=distance[cur]+cij)
				continue;
			
			prev[next]=arc;
			distance[next]=distance[cur]+cij;
			
			heap_insert(myheap,next,distance[next]);
		}
	}
	tal_free(this_ctx);	
	return ret;
}
    
/* Set zero flow in the residual network. */
static void zero_flow(
	const struct pay_parameters *params,
	struct residual_network *network)
{
	for(u32 node=0;node<tal_count(network->potential);++node)
	{
		network->potential[node]=0;
		for(u32 arc=node_adjacency_begin(params,node);
			arc!=node_adjacency_end(params,node);
			arc = node_adjacency_next(params,node,arc))
		{
			if(arc_is_dual(params,arc))continue;
			
			u32 dual = arc_dual(params,arc);
			
			network->cap[arc] += network->cap[dual];
			network->cap[dual] = 0;
		}
	}
}
    
/* Starting from a feasible flow (satisfies the balance and capacity
 * constraints), find a solution that minimizes the network->cost function. 
 * 
 * TODO(eduardo) The MCF must be called several times until we get a good
 * compromise between fees and probabilities. Instead of re-computing the MCF at
 * each step, we might use the previous flow result, which is not optimal in the
 * current iteration but I might be not too far from the truth.
 * It comes to mind to use cycle cancelling. */
static int optimize_mcf(const struct pay_parameters *params,
			 struct residual_network *network)
{
	int ret = RENEPAY_ERR_OK;
	
	zero_flow(params,network);
   	s64 amount = params->amount->millisatoshis/1000;
	u32 source = gossmap_node_idx(params->gossmap,params->source),
	    target = gossmap_node_idx(params->gossmap,params->target);
	
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	u32 *prev = tal_arr(this_ctx,u32,tal_count(network->potential));
	s64 *distance = tal_arr(this_ctx,s64,tal_count(network->potential));
	
	while(amount>0)
	{
		int err = find_optimal_path(params,network,source,target,prev,distance);
		if(err!=RENEPAY_ERR_OK)
		{
			// unexpected error
			ret = RENEPAY_ERR_NOFEASIBLEFLOW;
			break;
		}
		
		// traverse the path and see how much flow we can send
		s64 delta = get_augmenting_flow(params,network,source,target,prev);
		
		// commit that flow to the path
		delta = MIN(amount,delta);
		augment_flow(params,network,source,target,prev,delta);
		
		assert(delta>0 && delta<=amount);
		amount -= delta;
		
		// update potentials
		for(u32 n=0;n<tal_count(network->potential);++n)
		{
			// see page 323 of Ahuja-Magnanti-Orlin
			network->potential[n] -= MIN(distance[target],distance[n]);
			
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
	
/* Given a flow in the residual network, compute the probability and fee cost
 * ppm. 
 * Instead of using the cost function, which contains many approximations we
 * will use a direct approach to compute in O(n+m) the fees and prob. */
static void estimate_costs(const struct pay_parameters *params,
                           const struct residual_network *network,
			   double *prob_ptr,
			   double *fee_ppm_ptr)
{
	double fee_microsats = 0;
	double prob_cost = 0;
	
	struct gossmap_node *node = gossmap_first_node(params->gossmap);
	const size_t num_nodes = gossmap_num_nodes(params->gossmap);
	for(size_t i=0; i<num_nodes; ++i, node = gossmap_next_node(params->gossmap,node))
	{
		for(size_t j=0;j<node->num_chans;++j)
		{
			int dir;
			const struct gossmap_chan *c = gossmap_nth_chan(params->gossmap,
			                                                node, j, &dir);
			const u32 chan_idx = gossmap_chan_idx(params->gossmap, c);
			
			// in sats
			s64 chan_flow=0;
			
			for(size_t k=0;k<CHANNEL_PARTS;++k)
			{
				const u32 arc = channel_idx_to_arc(chan_idx,dir,k,0);
				chan_flow += get_arc_flow(params,network,arc);	
			}
			
			// TODO(eduardo): this is wrong, because the base fee is
			// invoked for each HTLC, we may use a single payment
			// channel in multiple payment flows.
			fee_microsats += chan_flow * c->half[dir].proportional_fee 
			                  + c->half[dir].base_fee*1e3; 
			
			struct chan_extra_half *extra_half 
				= get_chan_extra_half_by_chan(
						params->gossmap,
						params->chan_extra_map
						c,
						dir);  
			
			prob_cost += pickhardt_probability_cost(
				extra_half->known_min.millisatoshis*1e3,
				extra_half->known_max.millisatoshis*1e3,
				chan_flow);
		}
	}
	*prob_str = exp(-prob_cost);
	*fee_ppm_str = fee_microsats * 1e3 / params->amount.millisatoshis;
}

// flow on directed channels
struct chan_flow
{
	s64 half[2];
};

struct list_data
{
	struct flow_path *flow_path;
	struct list_node list;
};

/* Given a flow in the residual network, build a set of payment flows in the
 * gossmap that corresponds to this flow. */		
static struct flow_path **
	get_flow_paths(const struct pay_parameters *params,
				    const struct residual_network *network,
				    const tal_t *ctx)
{
	tal_t *this_ctx = tal(tmpctx,tal_t);
	
	const size_t max_num_chans = gossmap_max_chan_idx(params->gossmap);
	struct chan_flow *chan_flow = tal_arr(this_ctx,struct chan_flow,max_num_chans);
	
	for(size_t i=0;i<max_num_chans;++i)
		chan_flow[i].half[0]=chan_flow[i].half[1]=0;
	
	const size_t max_num_nodes = gossmap_max_node_idx(params->gossmap);
	s64 *balance = tal_arr(this_ctx,s64,max_num_nodes);
	struct gossmap_chan **prev_chan = tal_arr(this_ctx,struct gossmap_chan*,max_num_nodes);
	int *prev_dir = tal_arr(this_ctx,int,max_num_nodes);
	u32 *prev_idx = tal_arr(this_ctx,u32,max_num_nodes);
	
	for(size_t i=0;i<max_num_nodes;++i)
	{
		balance[i]=0;
	}
	
	// Convert the arc based residual network flow into a flow in the
	// directed channel network.
	// Compute balance on the nodes.
	for(u32 n = 0;n<max_num_nodes;++n)
	{
		for(u32 arc = node_adjacency_begin(params,cur);
		        arc!= node_adjacency_end(params,cur);
			arc = node_adjacency_next(params,cur,arc))
		{
			if(arc_is_dual(arc))
				continue;
			u32 m = arc_head(params,arc);
			s64 flow = get_arc_flow(params,network,arc);
			
			balance[n] -= flow;
			balance[m] += flow;
			
			u32 chan_idx;
			int dir,part,dual;
			chan_idx = arc_to_channel_idx(arc,dir,part,dual);
			
			chan_flow[chan_idx].half[dir] +=flow;
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
		// this node has negative balance, flows leaves from here
		while(balance[node_idx]<0)
		{	
			s64 delta=-balance[node_idx];
			u32 final_idx;
			
			int length = 0;
			prev_chan[node_idx]=NULL;
			
			u32 final_idx = node_idx;
			
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
				const struct gossmap_node *cur
					= gossmap_node_byidx(params->gossmap,final_idx);
				
				for(size_t i=0;i<cur->num_chans;++i)
				{
					int dir;
					const struct gossmap_chan *c 
						= gossmap_nth_chan(params->gossmap,
						                   cur,i,&dir);
					
					const u32 c_idx = gossmap_chan_idx(params->gossmap,c);
					
					if(chan_flow[c_idx].half[dir]>0)
					{
						// follow the flow
						const struct gossmap_node *next
							= gossmap_nth_node(params->gossmap,
									   c,!dir);
						u32 next_idx = gossmap_node_idx(params->gossmap,next);
						
						delta=MIN(delta,chan_flow[c_idx].half[dir]);
						
						prev_dir[next_idx] = dir;
						prev_chan[next_idx] = c;
						prev_idx[next_idx] = final_idx;
						length ++;
						final_idx = next_idx;
						break;
					}
				}
			}
			delta = MIN(delta,balance[final_idx]);
			
			struct flow_path *fp = tal(ctx,struct flow_path);
			fp->path = tal_arr(fp,struct gossmap_chan*,length);
			fp->dirs = tal_arr(fp,int,length);
			fp->amount.satoshis = delta;
			
			balance[node_idx] += delta;
			balance[final_idx]-= delta;
			
			// walk backwards, substract flow
			for(u32 cur_idx = final_idx;cur_idx!=node_idx;cur_idx=prev_idx[node_idx])
			{
				length--;
				fp->path[length]=prev_chan[cur_idx];
				fp->dirs[length]=prev_dir[cur_idx];
				
				chan_flow[prev_chan[cur_idx]].half[prev_dir[cur_idx]]-=delta;
			}
			
			// add fp to list
			ld = tal(list_ctx,struct list_data);
			ld->flow_path = fp;
			list_add(&path_list,&ld->list);
			num_paths++;
		}
	}
	
	// copy the list into the array we are going to return	
	struct flow_path **flows = tal_arr(ctx,struct flow_path*,num_paths);
	size_t pos=0;
	list_for_each(&path_list,ld,list)
	{
		flows[pos++] = ld->flow_path;
	}
	
	tal_free(this_ctx);
	return flows;
}

/* How to combine probability cost and fee cost?
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


/* eduardo: I think it should be clear that this module deals with linear
 * flows, ie. base fees are not considered. Hence a flow along a path is
 * described with a sequence of directed channels and one amount. 
 * In the `pay_flow` module there are dedicated routes to compute the actual
 * amount to be forward on each hop. */
int optimal_payment_flow(
                      const tal_t *ctx,
		      struct gossmap *gossmap,
		      const struct gossmap_node *source,
		      const struct gossmap_node *target,
		      struct chan_extra_map *chan_extra_map,
		      const bitmap *disabled,
		      struct amount_msat amount,
		      double max_fee_ppm,
		      double min_probability,
		      double delay_feefactor,
		      struct flow_paths **flow_paths)
{
	// TODO(eduardo) on which ctx should I allocate the MCF data?
	struct pay_parameters *params = tal(tmpctx,struct pay_parameters);	
	
	int ret;
	
	params->gossmap = gossmap;
	params->source = source;
	params->target = target;
	params->chan_extra_map = chan_extra_map;
	params->disabled = disabled;
	params->amount = amount;
	
	params->cap_fraction[0]=0;
	params->cost_fraction[0]=0;
	for(size_t i =0;i<CHANNEL_PARTS;++i)
	{
		params->cap_fraction[i]=CHANNEL_PIVOTS[i]-CHANNEL_PIVOTS[i-1];
		params->cost_fraction[i]=
			log((1-CHANNEL_PIVOTS[i-1])/(1-CHANNEL_PIVOTS[i]))
			/params->cap_fraction[i];
	}
	
	// TODO(eduardo): handle max_fee_ppm, delay_feefactor and basefee_penalty
	// params->max_fee_ppm = max_fee_ppm;
	// params->delay_feefactor = delay_feefactor;
	// params->basefee_penalty = 
	
	// build the uncertainty network with linearization and residual arcs
	struct residual_network *network = tal(tmpctx,struct residual_network);
	init_residual_network(params,network);
	
	const u32 target_idx = gossmap_node_idx(params->gossmap,target);
	const u32 source_idx = gossmap_node_idx(params->gossmap,source);
	
	int err = find_feasible_flow(params,network,source_idx,target_idx,
	                             params->amount->millisatoshis/1000);
	
	if(err!=RENEPAY_ERR_OK)
	{
		// there is no flow that satisfy the constraints, we stop here
		ret = RENEPAY_ERR_NOFEASIBLEFLOW;
		goto end;
	}
	
	const size_t max_num_arcs = tal_count(params->arc_prob_cost);
	assert(max_num_arcs==tal_count(params->arc_prob_cost));
	assert(max_num_arcs==tal_count(params->arc_fee_cost));
	assert(max_num_arcs==tal_count(network->cap));
	assert(max_num_arcs==tal_count(network->cost));
	
	// binary search for a value of `mu` that fits our fee and prob.
	// constraints.
	bool flow_found = false;
	s64 mu_left = 0, mu_right = MU_MAX;
	ret=RENEPAY_ERR_NOCHEAPFLOW;
	while(mu_left<mu_right)
	{
		s64 mu = (mu_left + mu_right)/2;
		
		for(u32 arc=0;arc<max_num_arcs;++arc)
		{
			const s64 pcost = params->arc_prob_cost[arc],
			          fcost = params->arc_fee_cost[arc];
				  
			const s64 combined = pcost==INFINITE || fcost==INFINITE ? INFINITE :
			                     mu*fcost + (MU_MAX-1-mu)*pcost;
				       
			network->cost[arc]
				= mu==0 ? pcost : 
				          (mu==(MU_MAX-1) ? fcost : combined);
		}
		
		optimize_mcf(params,network);
		
		double prob_success,fee_ppm;
		estimate_costs(params,network,&prob,&fee_ppm);
		
		if(fee_ppm>max_fee_ppm)
		{
			// too expensive
			mu_left = mu+1;
		}else if(prob<min_probability)
		{
			// too unlikely
			mu_right = mu;
		}else
		{
			// a good compromise
			ret=RENEPAY_ERR_OK;
			break;
		}
	}
	
	end:
	
	*flow_paths = NULL;

	if(ret==RENEPAY_ERR_OK)
	{
		*flow_paths = get_flow_paths(params,network,ctx);
	}else
	{
	// TODO(eduardo) fallback to c_prob or c_fee?
	}
	
	tal_free(network);
	tal_free(params);
	
	return ret;
}

