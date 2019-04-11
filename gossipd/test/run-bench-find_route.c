#include <assert.h>
#include <bitcoin/pubkey.h>
#include <ccan/err/err.h>
#include <ccan/opt/opt.h>
#include <ccan/tal/str/str.h>
#include <ccan/time/time.h>
#include <common/pseudorand.h>
#include <common/status.h>
#include <common/type_to_string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../routing.c"
#include "../gossip_store.c"
#include "../broadcast.c"

void status_fmt(enum log_level level UNUSED, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_channel_announcement */
bool fromwire_channel_announcement(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, secp256k1_ecdsa_signature *node_signature_1 UNNEEDED, secp256k1_ecdsa_signature *node_signature_2 UNNEEDED, secp256k1_ecdsa_signature *bitcoin_signature_1 UNNEEDED, secp256k1_ecdsa_signature *bitcoin_signature_2 UNNEEDED, u8 **features UNNEEDED, struct bitcoin_blkid *chain_hash UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, struct node_id *node_id_1 UNNEEDED, struct node_id *node_id_2 UNNEEDED, struct pubkey *bitcoin_key_1 UNNEEDED, struct pubkey *bitcoin_key_2 UNNEEDED)
{ fprintf(stderr, "fromwire_channel_announcement called!\n"); abort(); }
/* Generated stub for fromwire_channel_update */
bool fromwire_channel_update(const void *p UNNEEDED, secp256k1_ecdsa_signature *signature UNNEEDED, struct bitcoin_blkid *chain_hash UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, u32 *timestamp UNNEEDED, u8 *message_flags UNNEEDED, u8 *channel_flags UNNEEDED, u16 *cltv_expiry_delta UNNEEDED, struct amount_msat *htlc_minimum_msat UNNEEDED, u32 *fee_base_msat UNNEEDED, u32 *fee_proportional_millionths UNNEEDED)
{ fprintf(stderr, "fromwire_channel_update called!\n"); abort(); }
/* Generated stub for fromwire_channel_update_option_channel_htlc_max */
bool fromwire_channel_update_option_channel_htlc_max(const void *p UNNEEDED, secp256k1_ecdsa_signature *signature UNNEEDED, struct bitcoin_blkid *chain_hash UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, u32 *timestamp UNNEEDED, u8 *message_flags UNNEEDED, u8 *channel_flags UNNEEDED, u16 *cltv_expiry_delta UNNEEDED, struct amount_msat *htlc_minimum_msat UNNEEDED, u32 *fee_base_msat UNNEEDED, u32 *fee_proportional_millionths UNNEEDED, struct amount_msat *htlc_maximum_msat UNNEEDED)
{ fprintf(stderr, "fromwire_channel_update_option_channel_htlc_max called!\n"); abort(); }
/* Generated stub for fromwire_gossipd_local_add_channel */
bool fromwire_gossipd_local_add_channel(const void *p UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED, struct node_id *remote_node_id UNNEEDED, struct amount_sat *satoshis UNNEEDED)
{ fprintf(stderr, "fromwire_gossipd_local_add_channel called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_channel_announcement */
bool fromwire_gossip_store_channel_announcement(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, u8 **announcement UNNEEDED, struct amount_sat *satoshis UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_channel_announcement called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_channel_delete */
bool fromwire_gossip_store_channel_delete(const void *p UNNEEDED, struct short_channel_id *short_channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_channel_delete called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_channel_update */
bool fromwire_gossip_store_channel_update(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, u8 **update UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_channel_update called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_local_add_channel */
bool fromwire_gossip_store_local_add_channel(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, u8 **local_add UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_local_add_channel called!\n"); abort(); }
/* Generated stub for fromwire_gossip_store_node_announcement */
bool fromwire_gossip_store_node_announcement(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, u8 **announcement UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_store_node_announcement called!\n"); abort(); }
/* Generated stub for fromwire_node_announcement */
bool fromwire_node_announcement(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, secp256k1_ecdsa_signature *signature UNNEEDED, u8 **features UNNEEDED, u32 *timestamp UNNEEDED, struct node_id *node_id UNNEEDED, u8 rgb_color[3] UNNEEDED, u8 alias[32] UNNEEDED, u8 **addresses UNNEEDED)
{ fprintf(stderr, "fromwire_node_announcement called!\n"); abort(); }
/* Generated stub for fromwire_peektype */
int fromwire_peektype(const u8 *cursor UNNEEDED)
{ fprintf(stderr, "fromwire_peektype called!\n"); abort(); }
/* Generated stub for fromwire_wireaddr */
bool fromwire_wireaddr(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct wireaddr *addr UNNEEDED)
{ fprintf(stderr, "fromwire_wireaddr called!\n"); abort(); }
/* Generated stub for onion_type_name */
const char *onion_type_name(int e UNNEEDED)
{ fprintf(stderr, "onion_type_name called!\n"); abort(); }
/* Generated stub for sanitize_error */
char *sanitize_error(const tal_t *ctx UNNEEDED, const u8 *errmsg UNNEEDED,
		     struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "sanitize_error called!\n"); abort(); }
/* Generated stub for status_failed */
void status_failed(enum status_failreason code UNNEEDED,
		   const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "status_failed called!\n"); abort(); }
/* Generated stub for towire_errorfmt */
u8 *towire_errorfmt(const tal_t *ctx UNNEEDED,
		    const struct channel_id *channel UNNEEDED,
		    const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "towire_errorfmt called!\n"); abort(); }
/* Generated stub for towire_gossipd_local_add_channel */
u8 *towire_gossipd_local_add_channel(const tal_t *ctx UNNEEDED, const struct short_channel_id *short_channel_id UNNEEDED, const struct node_id *remote_node_id UNNEEDED, struct amount_sat satoshis UNNEEDED)
{ fprintf(stderr, "towire_gossipd_local_add_channel called!\n"); abort(); }
/* Generated stub for towire_gossip_store_channel_announcement */
u8 *towire_gossip_store_channel_announcement(const tal_t *ctx UNNEEDED, const u8 *announcement UNNEEDED, struct amount_sat satoshis UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_channel_announcement called!\n"); abort(); }
/* Generated stub for towire_gossip_store_channel_delete */
u8 *towire_gossip_store_channel_delete(const tal_t *ctx UNNEEDED, const struct short_channel_id *short_channel_id UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_channel_delete called!\n"); abort(); }
/* Generated stub for towire_gossip_store_channel_update */
u8 *towire_gossip_store_channel_update(const tal_t *ctx UNNEEDED, const u8 *update UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_channel_update called!\n"); abort(); }
/* Generated stub for towire_gossip_store_local_add_channel */
u8 *towire_gossip_store_local_add_channel(const tal_t *ctx UNNEEDED, const u8 *local_add UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_local_add_channel called!\n"); abort(); }
/* Generated stub for towire_gossip_store_node_announcement */
u8 *towire_gossip_store_node_announcement(const tal_t *ctx UNNEEDED, const u8 *announcement UNNEEDED)
{ fprintf(stderr, "towire_gossip_store_node_announcement called!\n"); abort(); }
/* Generated stub for update_peers_broadcast_index */
void update_peers_broadcast_index(struct list_head *peers UNNEEDED, u32 offset UNNEEDED)
{ fprintf(stderr, "update_peers_broadcast_index called!\n"); abort(); }
/* Generated stub for wire_type_name */
const char *wire_type_name(int e UNNEEDED)
{ fprintf(stderr, "wire_type_name called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

#if DEVELOPER
/* Generated stub for memleak_remove_htable */
void memleak_remove_htable(struct htable *memtable UNNEEDED, const struct htable *ht UNNEEDED)
{ fprintf(stderr, "memleak_remove_htable called!\n"); abort(); }
/* Generated stub for memleak_remove_intmap_ */
void memleak_remove_intmap_(struct htable *memtable UNNEEDED, const struct intmap *m UNNEEDED)
{ fprintf(stderr, "memleak_remove_intmap_ called!\n"); abort(); }
#endif

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
	if (!chan)
		chan = new_chan(rstate, &scid, &nodes[from], &nodes[to],
				AMOUNT_SAT(1000000));

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
			       pseudorand(100),
			       pseudorand(100),
			       pseudorand(144));
		add_connection(rstate, nodes, randnode, n,
			       pseudorand(100),
			       pseudorand(100),
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
	setup_locale();

	struct routing_state *rstate;
	size_t num_nodes = 100, num_runs = 1;
	struct timemono start, end;
	size_t num_success;
	struct node_id me;
	struct node_id *nodes;
	bool perfme = false;
	const double riskfactor = 0.01 / BLOCKS_PER_YEAR / 10000;
	struct siphash_seed base_seed;

	secp256k1_ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY
						 | SECP256K1_CONTEXT_SIGN);
	setup_tmpctx();

	me = nodeid(0);
	rstate = new_routing_state(tmpctx, NULL, &me, 0, NULL, NULL, NULL);
	opt_register_noarg("--perfme", opt_set_bool, &perfme,
			   "Run perfme-start and perfme-stop around benchmark");

	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (argc > 1)
		num_nodes = atoi(argv[1]);
	if (argc > 2)
		num_runs = atoi(argv[2]);
	if (argc > 3)
		opt_usage_and_exit("[num_nodes [num_runs]]");

	nodes = tal_arr(rstate, struct node_id, num_nodes);
	for (size_t i = 0; i < num_nodes; i++)
		nodes[i] = nodeid(i);

	memset(&base_seed, 0, sizeof(base_seed));
	for (size_t i = 0; i < num_nodes; i++)
		populate_random_node(rstate, nodes, i);

	if (perfme)
		run("perfme-start");

	start = time_mono();
	num_success = 0;
	for (size_t i = 0; i < num_runs; i++) {
		const struct node_id *from = &nodes[pseudorand(num_nodes)];
		const struct node_id *to = &nodes[pseudorand(num_nodes)];
		struct amount_msat fee;
		struct chan **route;

		route = find_route(tmpctx, rstate, from, to,
				   (struct amount_msat){pseudorand(100000)},
				   riskfactor,
				   0.75, &base_seed,
				   ROUTING_MAX_HOPS,
				   &fee);
		num_success += (route != NULL);
		tal_free(route);
	}
	end = time_mono();

	if (perfme)
		run("perfme-stop");

	printf("%zu (%zu succeeded) routes in %zu nodes in %"PRIu64" msec (%"PRIu64" nanoseconds per route)",
	       num_runs, num_success, num_nodes,
	       time_to_msec(timemono_between(end, start)),
	       time_to_nsec(time_divide(timemono_between(end, start), num_runs)));

	tal_free(tmpctx);
	secp256k1_context_destroy(secp256k1_ctx);
	opt_free_table();
	return 0;
}
