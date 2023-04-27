#ifndef LIGHTNING_COMMON_GOSSTEST_H
#define LIGHTNING_COMMON_GOSSTEST_H
#include "config.h"
#include <ccan/tal/tal.h>

struct node_id;
struct privkey;

/**
 * gosstest_privkey: get secret key for this node name.
 * @name: the name, typically A, B (or Alice, Bob).
 * @privkey: the returned key, created using first letter of @name.
 */
void gosstest_privkey(const char *name, struct privkey *privkey);

/**
 * gosstest_node_id: get node id for this node name.
 * @name: the name, typically A, B (or Alice, Bob).
 * @id: the returned id, created by private key using first letter of @name.
 */
void gosstest_node_id(const char *name, struct node_id *id);

/**
 * gosstest_from_comment: parse the source file and get this graph.
 * @ctx: context to allocate returned string from.
 * @argv: arguments to this program.
 * @graphname: the name of the graph in the comment.
 *
 * This magic function creates a gossip_store by parsing an ASCII
 * graph from your source code.  It's used for unit testing.
 *
 * If argv[1] is `-v` then debugging information is printed.
 * Here's an example graph, called Alice-Bob, showing both separate
 * sides of a channel with different settings.  The defaults are:
 *   scid: <first-letter-of-node1>x<first-letter-of-node2>x1
 *         (node1 and node2 are in gossip order) e.g. 65x66x1.
 *   cap: 100000sats
 *   htlcmin: 0
 *   htlcmax: cap
 *   feebase: 0
 *   feeppm: 0
 */

/* GRAPH Alice-Bob
 *
 * Alice---------------->Bob
 *  ^       cap=100sat    |
 *  |       scid=1x2x3    |
 *  |      htlcmin=4msat  |
 *  |      htlcmax=5sat   |
 *  |       feebase=6     |
 *  |        feeppm=7    /
 *   \			/
 *    -----------------/
 *        scid=1x2x3
 *	  cap=100sat
 */
const char *gosstest_from_comment(const tal_t *ctx,
				  char **argv,
				  const char *graphname);

#endif /* LIGHTNING_COMMON_GOSSTEST_H */
