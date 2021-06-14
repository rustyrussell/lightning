#include "config.h"
#include <assert.h>
#include <bitcoin/pubkey.h>
#include <ccan/err/err.h>
#include <ccan/opt/opt.h>
#include <ccan/tal/str/str.h>
#include <ccan/time/time.h>
#include <common/json_stream.h>
#include <common/pseudorand.h>
#include <common/setup.h>
#include <common/status.h>
#include <common/type_to_string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../routing.c"
#include "../gossip_store.c"

void status_fmt(enum log_level level UNUSED,
		const struct node_id *node_id,
		const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

/* AUTOGENERATED MOCKS START */
/* Generated stub for cupdate_different */
bool cupdate_different(struct gossip_store *gs UNNEEDED,
		       const struct half_chan *hc UNNEEDED,
		       const u8 *cupdate UNNEEDED)
{ fprintf(stderr, "cupdate_different called!\n"); abort(); }
/* Generated stub for fmt_wireaddr_without_port */
char *fmt_wireaddr_without_port(const tal_t *ctx UNNEEDED, const struct wireaddr *a UNNEEDED)
{ fprintf(stderr, "fmt_wireaddr_without_port called!\n"); abort(); }
/* Generated stub for fromwire_wireaddr_array */
struct wireaddr *fromwire_wireaddr_array(const tal_t *ctx UNNEEDED, const u8 *ser UNNEEDED)
{ fprintf(stderr, "fromwire_wireaddr_array called!\n"); abort(); }
/* Generated stub for json_add_member */
void json_add_member(struct json_stream *js UNNEEDED,
		     const char *fieldname UNNEEDED,
		     bool quote UNNEEDED,
		     const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "json_add_member called!\n"); abort(); }
/* Generated stub for json_member_direct */
char *json_member_direct(struct json_stream *js UNNEEDED,
			 const char *fieldname UNNEEDED, size_t extra UNNEEDED)
{ fprintf(stderr, "json_member_direct called!\n"); abort(); }
/* Generated stub for json_object_end */
void json_object_end(struct json_stream *js UNNEEDED)
{ fprintf(stderr, "json_object_end called!\n"); abort(); }
/* Generated stub for json_object_start */
void json_object_start(struct json_stream *ks UNNEEDED, const char *fieldname UNNEEDED)
{ fprintf(stderr, "json_object_start called!\n"); abort(); }
/* Generated stub for memleak_add_helper_ */
void memleak_add_helper_(const tal_t *p UNNEEDED, void (*cb)(struct htable *memtable UNNEEDED,
						    const tal_t *)){ }
/* Generated stub for nannounce_different */
bool nannounce_different(struct gossip_store *gs UNNEEDED,
			 const struct node *node UNNEEDED,
			 const u8 *nannounce UNNEEDED)
{ fprintf(stderr, "nannounce_different called!\n"); abort(); }
/* Generated stub for notleak_ */
void *notleak_(const void *ptr UNNEEDED, bool plus_children UNNEEDED)
{ fprintf(stderr, "notleak_ called!\n"); abort(); }
/* Generated stub for peer_supplied_good_gossip */
void peer_supplied_good_gossip(struct peer *peer UNNEEDED, size_t amount UNNEEDED)
{ fprintf(stderr, "peer_supplied_good_gossip called!\n"); abort(); }
/* Generated stub for private_channel_announcement */
const u8 *private_channel_announcement(const tal_t *ctx UNNEEDED,
				       const struct short_channel_id *scid UNNEEDED,
				       const struct node_id *local_node_id UNNEEDED,
				       const struct node_id *remote_node_id UNNEEDED,
				       const u8 *features UNNEEDED)
{ fprintf(stderr, "private_channel_announcement called!\n"); abort(); }
/* Generated stub for sanitize_error */
char *sanitize_error(const tal_t *ctx UNNEEDED, const u8 *errmsg UNNEEDED,
		     struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "sanitize_error called!\n"); abort(); }
/* Generated stub for status_failed */
void status_failed(enum status_failreason code UNNEEDED,
		   const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "status_failed called!\n"); abort(); }
/* Generated stub for towire_warningfmt */
u8 *towire_warningfmt(const tal_t *ctx UNNEEDED,
		      const struct channel_id *channel UNNEEDED,
		      const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "towire_warningfmt called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

#if DEVELOPER
/* Generated stub for memleak_remove_htable */
void memleak_remove_htable(struct htable *memtable UNNEEDED, const struct htable *ht UNNEEDED)
{ fprintf(stderr, "memleak_remove_htable called!\n"); abort(); }
/* Generated stub for memleak_remove_intmap_ */
void memleak_remove_intmap_(struct htable *memtable UNNEEDED, const struct intmap *m UNNEEDED)
{ fprintf(stderr, "memleak_remove_intmap_ called!\n"); abort(); }
#endif

/* NOOP for new_reltimer_ */
struct oneshot *new_reltimer_(struct timers *timers UNNEEDED,
			      const tal_t *ctx UNNEEDED,
			      struct timerel expire UNNEEDED,
			      void (*cb)(void *) UNNEEDED, void *arg UNNEEDED)
{
	return NULL;
}

/* Updates existing route if required. */
static void add_connection(struct routing_state *rstate,
			   const struct node_id *nodes,
			   u32 from, u32 to,
			   u32 base_fee, s32 proportional_fee,
			   u32 delay)
{
	struct short_channel_id scid;
	struct half_chan *c;
	struct chan *chan;
	int idx = node_id_idx(&nodes[from], &nodes[to]);

	/* Encode src and dst in scid. */
	memcpy((char *)&scid + idx * sizeof(from), &from, sizeof(from));
	memcpy((char *)&scid + (!idx) * sizeof(to), &to, sizeof(to));

	chan = get_channel(rstate, &scid);
	if (!chan) {
		chan = new_chan(rstate, &scid, &nodes[from], &nodes[to],
				AMOUNT_SAT(1000000), NULL);
	}

	c = &chan->half[idx];
	c->base_fee = base_fee;
	c->proportional_fee = proportional_fee;
	c->delay = delay;
	c->channel_flags = node_id_idx(&nodes[from], &nodes[to]);
	/* This must be non-zero, otherwise we consider it disabled! */
	c->bcast.index = 1;
	c->htlc_maximum = AMOUNT_MSAT(-1ULL);
	c->htlc_minimum = AMOUNT_MSAT(0);
}

static struct node_id nodeid(size_t n)
{
	struct node_id id;
	struct pubkey k;
	struct secret s;

	memset(&s, 0xFF, sizeof(s));
	memcpy(&s, &n, sizeof(n));
	pubkey_from_secret(&s, &k);
	node_id_from_pubkey(&id, &k);
	return id;
}

static void populate_random_node(struct routing_state *rstate,
				 const struct node_id *nodes,
				 u32 n)
{
	/* Create 2 random channels. */
	if (n < 1)
		return;

	for (size_t i = 0; i < 2; i++) {
		u32 randnode = pseudorand(n);

		add_connection(rstate, nodes, n, randnode,
			       pseudorand(1000),
			       pseudorand(1000),
			       pseudorand(144));
		add_connection(rstate, nodes, randnode, n,
			       pseudorand(1000),
			       pseudorand(1000),
			       pseudorand(144));
	}
}

static void run(const char *name)
{
	int status;

	switch (fork()) {
	case 0:
		execlp(name, name, NULL);
		exit(127);
	case -1:
		err(1, "forking %s", name);
	default:
		wait(&status);
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			errx(1, "%s failed", name);
	}
}

int main(int argc, char *argv[])
{
	struct routing_state *rstate;
	size_t num_nodes = 100, num_runs = 1;
	struct timemono start, end;
	size_t route_lengths[ROUTING_MAX_HOPS+1];
	struct node_id me;
	struct node_id *nodes;
	bool perfme = false;
	const double riskfactor = 0.01 / BLOCKS_PER_YEAR / 10000;
	struct siphash_seed base_seed;

	common_setup(argv[0]);

	me = nodeid(0);
	rstate = new_routing_state(tmpctx, &me, NULL, NULL, NULL,
				   false, false);
	opt_register_noarg("--perfme", opt_set_bool, &perfme,
			   "Run perfme-start and perfme-stop around benchmark");

	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (argc > 1)
		num_nodes = atoi(argv[1]);
	if (argc > 2)
		num_runs = atoi(argv[2]);
	if (argc > 3)
		opt_usage_and_exit("[num_nodes [num_runs]]");

	printf("Creating nodes...\n");
	nodes = tal_arr(rstate, struct node_id, num_nodes);
	for (size_t i = 0; i < num_nodes; i++)
		nodes[i] = nodeid(i);

	printf("Populating nodes...\n");
	memset(&base_seed, 0, sizeof(base_seed));
	for (size_t i = 0; i < num_nodes; i++)
		populate_random_node(rstate, nodes, i);

	if (perfme)
		run("perfme-start");

	printf("Starting...\n");
	memset(route_lengths, 0, sizeof(route_lengths));
	start = time_mono();
	for (size_t i = 0; i < num_runs; i++) {
		const struct node_id *from = &nodes[pseudorand(num_nodes)];
		const struct node_id *to = &nodes[pseudorand(num_nodes)];
		struct amount_msat fee;
		struct chan **route;
		size_t num_hops;

		route = find_route(tmpctx, rstate, from, to,
				   (struct amount_msat){pseudorand(100000)},
				   riskfactor,
				   0.75, &base_seed,
				   ROUTING_MAX_HOPS,
				   &fee);
		num_hops = tal_count(route);
		assert(num_hops < ARRAY_SIZE(route_lengths));
		route_lengths[num_hops]++;
		tal_free(route);
	}
	end = time_mono();

	if (perfme)
		run("perfme-stop");

	printf("%zu (%zu succeeded) routes in %zu nodes in %"PRIu64" msec (%"PRIu64" nanoseconds per route)\n",
	       num_runs, num_runs - route_lengths[0], num_nodes,
	       time_to_msec(timemono_between(end, start)),
	       time_to_nsec(time_divide(timemono_between(end, start), num_runs)));
	for (size_t i = 0; i < ARRAY_SIZE(route_lengths); i++)
		if (route_lengths[i])
			printf(" Length %zu: %zu\n", i, route_lengths[i]);

	common_shutdown();
	opt_free_table();
	return 0;
}
