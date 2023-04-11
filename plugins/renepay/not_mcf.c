/* Horrible approximation to Min Cost Flow.
 *
 * i.e. https://arxiv.org/abs/2107.05322
 *
 * But the best solution for Min Cost Flow is approximate, and almost
 * linear in the number of edges: https://arxiv.org/abs/2203.00671
 *
 * It is, however, O(🤯) in implementation time :) I welcome anyone
 * who wants to implement it, and they can compare themselves to this
 * gross implementation with pride.
 */
/* Without this, gheap is *really* slow!  Comment out for debugging. */
#define NDEBUG
#include "config.h"
#include <ccan/bitmap/bitmap.h>
#include <ccan/cast/cast.h>
#include <common/gossmap.h>
#include <common/pseudorand.h>
#include <gheap.h>
#include <plugins/renepay/flow.h>
#include <plugins/renepay/mcf.h>

#ifndef SUPERVERBOSE
#define SUPERVERBOSE(...)
#else
#define SUPERVERBOSE_ENABLED 1
#endif

#define ASSERT(x) do { if (!(x)) abort(); } while(0)

/* Each node has this side-info. */
struct dijkstra {
	/* Using our cost function */
	double cost;
	/* I want to use an index here, except that gheap moves things onto
	 * a temporary on the stack and that makes things complex. */
	/* NULL means it's been visited already. */
	const struct gossmap_node **heapptr;
	/* We could re-evaluate to determine this, but keeps it simple */
	struct gossmap_chan *best_chan;
};

struct pay_parameters {
	/* The gossmap we are using */
	struct gossmap *gossmap;
	/* Our array of dijkstra per-node info */
	struct dijkstra *dij;
	/* How much do we care about fees vs certainty? */
	double mu;
	/* How much do we care about locktimes? */
	double delay_feefactor;
	/* Penalty factor for having a basefee (adds to ppm). */
	double basefee_penalty;
	/* Optional bitarray of disabled chans. */
	const bitmap *disabled;

	/* The working heap */
	struct gheap_ctx gheap_ctx;
	const struct gossmap_node **heap;
	size_t heapsize;

	/* Extra information we intuited about the channels */
	struct chan_extra_map *chan_extra_map;
};

/* Required global for gheap less_comparer */
static struct pay_parameters *global_params;

static struct dijkstra *get_dijkstra(const struct pay_parameters *params,
				     const struct gossmap_node *n)
{
	return params->dij + gossmap_node_idx(params->gossmap, n);
}

/* We want a minheap, not a maxheap, so this is backwards! */
static int less_comparer(const void *const ctx,
			 const void *const a,
			 const void *const b)
{
	const struct pay_parameters *params = ctx;
	return get_dijkstra(params, *(struct gossmap_node **)a)->cost
		> get_dijkstra(params, *(struct gossmap_node **)b)->cost;
}

static void item_mover(void *const dst, const void *const src)
{
	const struct pay_parameters *params = global_params;
	struct gossmap_node *n = *((struct gossmap_node **)src);
	get_dijkstra(params, n)->heapptr = dst;
	*((struct gossmap_node **)dst) = n;
}

static void append_to_heap(struct pay_parameters *params,
			   const struct gossmap_node *n,
			   double cost)
{
	struct dijkstra *d = get_dijkstra(params, n);

	params->heap[params->heapsize] = n;
	d->heapptr = &params->heap[params->heapsize];
	d->cost = cost;
	params->heapsize++;
}

static void init_heap(struct pay_parameters *params,
		      const struct gossmap_node *target)
{
	/* Initialize all heapptrs to NULL, costs to infinite */
	for (size_t i = 0; i < tal_count(params->dij); i++) {
		params->dij[i].cost = FLOW_INF_COST;
		params->dij[i].heapptr = NULL;
	}

	/* First entry in heap is start, cost 0 */
	params->heapsize = 0;
	append_to_heap(params, target, 0);

	ASSERT(gheap_is_heap(&params->gheap_ctx, params->heap, params->heapsize));
}

/* Convenient wrapper for flow_edge_cost */
static double costfn(const struct pay_parameters *params,
		     const struct gossmap_chan *c, int dir,
		     struct amount_msat flow)
{
	struct amount_msat min, max, prev_flow;
	struct chan_extra_half *h = get_chan_extra_half(params->gossmap,
							params->chan_extra_map,
							c, dir);
	if (h) {
		SUPERVERBOSE("costfn found extra %p: cap %s-%s, flow %s\n",
			     h,
			     type_to_string(tmpctx, struct amount_msat, &h->known_min),
			     type_to_string(tmpctx, struct amount_msat, &h->known_max),
			     type_to_string(tmpctx, struct amount_msat, &h->htlc_total));
		min = h->known_min;
		max = h->known_max;
		prev_flow = h->htlc_total;
	} else {
		/* We know nothing, use 0 - capacity */
		struct amount_sat cap;

		min = AMOUNT_MSAT(0);
		if (!gossmap_chan_get_capacity(params->gossmap, c, &cap))
			cap = AMOUNT_SAT(0);
		if (!amount_sat_to_msat(&max, cap))
			abort();
		prev_flow = AMOUNT_MSAT(0);
	}

	return flow_edge_cost(params->gossmap,
			      c, dir,
			      min, max, prev_flow,
			      flow,
			      params->mu,
			      params->basefee_penalty,
			      params->delay_feefactor);
}

static bool run_shortest_path(struct pay_parameters *params,
			      const struct gossmap_node *source,
			      const struct gossmap_node *target,
			      struct amount_msat flow)
{
	init_heap(params, target);

	while (params->heapsize != 0) {
		struct dijkstra *cur_d;
		const struct gossmap_node *cur;

		/* Pop off top of heap */
		cur = params->heap[0];

		/* Reached the source?  Done! */
		if (cur == source)
			return true;

		cur_d = get_dijkstra(params, cur);
		ASSERT(cur_d->heapptr == params->heap);
		gheap_pop_heap(&params->gheap_ctx, params->heap, params->heapsize--);
		cur_d->heapptr = NULL;

		for (size_t i = 0; i < cur->num_chans; i++) {
			struct gossmap_node *neighbor;
			int which_half;
			struct gossmap_chan *c;
			struct dijkstra *d;
			double cost;

			c = gossmap_nth_chan(params->gossmap,
					     cur, i, &which_half);
			/* We're going to traverse backwards, so we need
			 * channel_update into this node */
			if (!gossmap_chan_set(c, !which_half))
				continue;

			/* Must allow it through. */
			if (amount_msat_greater_fp16(flow,
						     c->half[!which_half].htlc_max))
				continue;

			neighbor = gossmap_nth_node(params->gossmap,
						    c, !which_half);

			d = get_dijkstra(params, neighbor);
			/* Ignore if already visited. */
			if (d->cost != FLOW_INF_COST && !d->heapptr)
				continue;

			/* They can explicitly disable some channels for this
			 * run */
			if (params->disabled
			    && bitmap_test_bit(params->disabled,
					       gossmap_chan_idx(params->gossmap,
								c)))
				continue;

			/* Get edge cost (will be FLOW_INF_COST in
			 * insufficient cpacity) */
			cost = costfn(params, c, !which_half, flow);

			/* If that doesn't give us a cheaper path, ignore */
			if (cur_d->cost + cost >= d->cost)
				continue;

			/* Yay, we have a new winner! */
			d->cost = cur_d->cost + cost;
			d->best_chan = c;
			if (!d->heapptr)
				append_to_heap(params, neighbor, d->cost);

			gheap_restore_heap_after_item_increase(&params->gheap_ctx,
							       params->heap,
							       params->heapsize,
							       d->heapptr
							       - params->heap);
		}
	}
	return false;
}

/* Which dir for the channel is n? */
static int gossmap_dir(const struct gossmap *gossmap,
		       const struct gossmap_node *n,
		       const struct gossmap_chan *c)
{
	/* Get the *other* peer attached to this channel */
	if (gossmap_nth_node(gossmap, c, 0) == n)
		return 0;
	ASSERT(gossmap_nth_node(gossmap, c, 1) == n);
	return 1;
}

/* We find the cheapest path between source and target for flow, ignoring
 * capacity limits. */
static const struct gossmap_chan **
find_cheap_flow(const tal_t *ctx,
		struct pay_parameters *params,
		const struct gossmap_node *source,
		const struct gossmap_node *target,
		struct amount_msat flow,
		int **dirs)
{
	const struct gossmap_node *n;
	const struct gossmap_chan **arr;

	if (!run_shortest_path(params, source, target, flow))
		return NULL;

	arr = tal_arr(ctx, const struct gossmap_chan *, 0);
	*dirs = tal_arr(ctx, int, 0);
	n = source;
	while (n != target) {
		struct dijkstra *d = get_dijkstra(params, n);
		int dir;

		dir = gossmap_dir(params->gossmap, n, d->best_chan);
		tal_arr_expand(&arr, d->best_chan);
		tal_arr_expand(dirs, dir);

		/* Get other end of channel */
		n = gossmap_nth_node(params->gossmap, d->best_chan, !dir);
	}
	return arr;
}

static void print_path(const char *desc,
		       const struct pay_parameters *params,
		       const struct gossmap_chan **path,
		       const int *dirs,
		       struct amount_msat amt)
{
#ifdef SUPERVERBOSE_ENABLED
	SUPERVERBOSE("%s: ", desc);

	for (size_t i = 0; i < tal_count(path); i++) {
		struct short_channel_id scid
			= gossmap_chan_scid(params->gossmap, path[i]);
		double cost;

		print_enable = false;
		cost = costfn(params, path[i], dirs[i], amt);
		print_enable = true;
		SUPERVERBOSE("%s%s/%i(set=%i/%i,htlc_max=%"PRIu64"/rev:%"PRIu64",fee=%u+%u,cost=%f) ",
			     i ? "->" : "",
			     type_to_string(tmpctx, struct short_channel_id, &scid),
			     dirs[i],
			     gossmap_chan_set(path[i], 0),
			     gossmap_chan_set(path[i], 1),
			     fp16_to_u64(path[i]->half[dirs[i]].htlc_max),
			     fp16_to_u64(path[i]->half[!dirs[i]].htlc_max),
			     path[i]->half[dirs[i]].base_fee,
			     path[i]->half[dirs[i]].proportional_fee,
			     cost);
	}
	SUPERVERBOSE("\n");
#endif /* SUPERVERBOSE_ENABLED */
}

static void print_flow(const char *desc,
		       const struct pay_parameters *params,
		       const struct flow *flow)
{
	struct amount_msat fee, delivered;

	delivered = flow->amounts[tal_count(flow->amounts)-1];
	print_path(desc, params, flow->path, flow->dirs, delivered);
	if (!amount_msat_sub(&fee, flow->amounts[0], delivered))
		abort();
	SUPERVERBOSE(" prob %.2f, %s delivered with fee %s\n",
		     flow->success_prob,
		     type_to_string(tmpctx, struct amount_msat, &delivered),
		     type_to_string(tmpctx, struct amount_msat, &fee));
}

static double flow_cost(struct pay_parameters *params,
			const struct gossmap_chan **path,
			const int *dirs,
			struct amount_msat amount)
{
	double cost = 0;

	for (size_t i = 0; i < tal_count(path); i++)
		cost += costfn(params, path[i], dirs[i], amount);
	return cost;
}

static struct flow *find_similar_flow(struct pay_parameters *params,
				      struct flow **flows, size_t num_flows,
				      const struct gossmap_chan **path,
				      const int *dirs,
				      struct amount_msat amount)
{
	double best_cost;
	struct flow *best;

	/* First search for exact equal */
	for (size_t i = 0; i < num_flows; i++) {
		if (flow_path_eq(flows[i]->path, flows[i]->dirs, path, dirs))
			return flows[i];
	}

	/* OK, see if one of the existing paths is similar cost. */
	best_cost = flow_cost(params, path, dirs, amount) * 1.10;
	best = NULL;
	for (size_t i = 0; i < num_flows; i++) {
		double cost = flow_cost(params, flows[i]->path, flows[i]->dirs, amount);
		if (cost < best_cost) {
			best = flows[i];
			best_cost = cost;
		}
	}
	return best;
}

struct flow **
minflow(const tal_t *ctx,
	struct gossmap *gossmap,
	const struct gossmap_node *source,
	const struct gossmap_node *target,
	struct chan_extra_map *chan_extra_map,
	const bitmap *disabled,
	struct amount_msat amount,
	double frugality,
	double delay_feefactor)
{
	struct pay_parameters *params = tal(tmpctx, struct pay_parameters);
	struct flow **flows;
	const struct amount_msat min_flow = AMOUNT_MSAT(5000);
	struct amount_msat step, remaining;
	size_t num_flows;

	params->gossmap = gossmap;
	params->dij = tal_arr(params, struct dijkstra,
			      gossmap_max_node_idx(gossmap));

	params->mu = derive_mu(gossmap, amount, frugality);
	params->delay_feefactor = delay_feefactor;
	params->disabled = disabled;
	ASSERT(!disabled
	       || tal_bytelen(disabled) == bitmap_sizeof(gossmap_max_chan_idx(gossmap)));

	/* If we assume we split into 5 parts, adjust ppm on basefee to
	 * make be correct at amt / 5.  Thus 1 basefee is worth 1 ppm at
	 * amount / 5. */
	params->basefee_penalty
		= 5000000.0 / amount.millisatoshis; /* Raw: penalty */

	/* There doesn't seem to be much difference with fanout 2-4. */
	params->gheap_ctx.fanout = 2;
	/* There seems to be a slight decrease if we alter this value. */
	params->gheap_ctx.page_chunks = 1;
	params->gheap_ctx.item_size = sizeof(*params->heap);
	params->gheap_ctx.less_comparer = less_comparer;
	params->gheap_ctx.less_comparer_ctx = params;
	params->gheap_ctx.item_mover = item_mover;

	/* This is initialized (and heapsize set) in init_heap each time */
	params->heap = tal_arr(params, const struct gossmap_node *,
			       gossmap_num_nodes(gossmap));

	params->chan_extra_map = chan_extra_map;
	global_params = params;

	/* Now gather the flows: we randomize step a little, but aim for 50. */
	flows = tal_arr(ctx, struct flow *, 100);
	step = amount_msat_div(amount, 50);
	if (amount_msat_less(step, min_flow))
		step = min_flow;

	remaining = amount;
	num_flows = 0;
	while (amount_msat_greater(remaining, AMOUNT_MSAT(0))) {
		struct amount_msat this_amount;
		const struct gossmap_chan **path;
		int *dirs;
		struct flow *flow;

		/* Randomize amount a little */
		if (!amount_msat_scale(&this_amount, step,
				       0.8 + 0.4 * pseudorand_double()))
			abort();

		if (amount_msat_greater(this_amount, remaining))
			this_amount = remaining;

		path = find_cheap_flow(tmpctx, params, source, target,
				       this_amount, &dirs);
		if (!path)
			return tal_free(flows);

		/* Maybe add to existing. */
		flow = find_similar_flow(params, flows, num_flows, path, dirs, this_amount);
		if (flow) {
			struct amount_msat before, after, tot;

			before = flow->amounts[tal_count(flow->amounts)-1];
			print_flow("Duplicate flow before", params, flow);
			flow_add(flow, params->gossmap, params->chan_extra_map,
				 this_amount);
			print_flow("Duplicate flow after", params, flow);
			after = flow->amounts[tal_count(flow->amounts)-1];
			if (!amount_msat_sub(&tot, after, before))
				abort();
			ASSERT(amount_msat_eq(tot, this_amount));
		} else {
			flow = tal(flows, struct flow);
			flow->path = tal_steal(flow, path);
			flow->dirs = tal_steal(flow, dirs);
			flow_complete(flow, params->gossmap,
				      params->chan_extra_map,
				      this_amount);
			print_flow("New flow", params, flow);
			flows[num_flows++] = flow;
			ASSERT(amount_msat_eq(flow->amounts[tal_count(flow->amounts)-1],
					      this_amount));
		}
		if (!amount_msat_sub(&remaining, remaining, this_amount))
			abort();
	}

	/* Shrink the array we return */
	tal_resize(&flows, num_flows);
	global_params = NULL;
	return flows;
}

#ifndef SUPERVERBOSE_ENABLED
#undef SUPERVERBOSE
#endif
