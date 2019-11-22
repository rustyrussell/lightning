#ifndef LIGHTNING_GOSSIPD_MINISKETCH_H
#define LIGHTNING_GOSSIPD_MINISKETCH_H
#include "config.h"
#include <ccan/short_types/short_types.h>

struct chan;
struct node;
struct routing_state;

#if EXPERIMENTAL_FEATURES
void init_minisketch(struct routing_state *rstate);
void destroy_minisketch(struct routing_state *rstate);
void add_to_sketch(struct routing_state *rstate, struct chan *chan);
void update_sketch(struct routing_state *rstate, struct chan *chan);
void remove_from_sketch(struct routing_state *rstate, struct chan *chan);
void update_node_sketches(struct routing_state *rstate,
			  const struct node *node);
void minisketch_blockheight_changed(struct routing_state *rstate,
				    u32 prev_blockheight);
const u8 *handle_gossip_set(struct peer *peer, const u8 *gossip_set);
#else
/* Noop versions */
static inline void init_minisketch(struct routing_state *rstate)
{
}

static inline void destroy_minisketch(struct routing_state *rstate)
{
}

static inline void add_to_sketch(struct routing_state *rstate,
				 struct chan *chan)
{
}

static inline void update_sketch(struct routing_state *rstate, struct chan *chan)
{
}

static inline void remove_from_sketch(struct routing_state *rstate,
				      struct chan *chan)
{
}

static inline void update_node_sketches(struct routing_state *rstate,
					const struct node *node)
{
}

static inline void minisketch_blockheight_changed(struct routing_state *rstate,
						  u32 prev_blockheight)
{
}
#endif /* !EXPERIMENTAL_FEATURES */

#endif /* LIGHTNING_GOSSIPD_MINISKETCH_H */
