#define main unused_main
int unused_main(int argc, char *argv[]);
#include "../../common/base32.c"
#include "../../common/wireaddr.c"
#include "../io_loop_with_timers.c"
#include "../lightningd.c"
#include "../subd.c"
#include <common/amount.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for activate_peers */
void activate_peers(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "activate_peers called!\n"); abort(); }
/* Generated stub for add_plugin_dir */
char *add_plugin_dir(struct plugins *plugins UNNEEDED, const char *dir UNNEEDED,
		     bool error_ok UNNEEDED)
{ fprintf(stderr, "add_plugin_dir called!\n"); abort(); }
/* Generated stub for begin_topology */
void begin_topology(struct chain_topology *topo UNNEEDED)
{ fprintf(stderr, "begin_topology called!\n"); abort(); }
/* Generated stub for channel_notify_new_block */
void channel_notify_new_block(struct lightningd *ld UNNEEDED,
			      u32 block_height UNNEEDED)
{ fprintf(stderr, "channel_notify_new_block called!\n"); abort(); }
/* Generated stub for coin_mvts_init_count */
void coin_mvts_init_count(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "coin_mvts_init_count called!\n"); abort(); }
/* Generated stub for connectd_activate */
void connectd_activate(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "connectd_activate called!\n"); abort(); }
/* Generated stub for connectd_init */
int connectd_init(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "connectd_init called!\n"); abort(); }
/* Generated stub for daemon_poll */
int daemon_poll(struct pollfd *fds UNNEEDED, nfds_t nfds UNNEEDED, int timeout UNNEEDED)
{ fprintf(stderr, "daemon_poll called!\n"); abort(); }
/* Generated stub for daemon_setup */
void daemon_setup(const char *argv0 UNNEEDED,
		  void (*backtrace_print)(const char *fmt UNNEEDED, ...) UNNEEDED,
		  void (*backtrace_exit)(void))
{ fprintf(stderr, "daemon_setup called!\n"); abort(); }
/* Generated stub for daemon_shutdown */
void daemon_shutdown(void)
{ fprintf(stderr, "daemon_shutdown called!\n"); abort(); }
/* Generated stub for db_begin_transaction_ */
void db_begin_transaction_(struct db *db UNNEEDED, const char *location UNNEEDED)
{ fprintf(stderr, "db_begin_transaction_ called!\n"); abort(); }
/* Generated stub for db_commit_transaction */
void db_commit_transaction(struct db *db UNNEEDED)
{ fprintf(stderr, "db_commit_transaction called!\n"); abort(); }
/* Generated stub for db_get_intvar */
s64 db_get_intvar(struct db *db UNNEEDED, char *varname UNNEEDED, s64 defval UNNEEDED)
{ fprintf(stderr, "db_get_intvar called!\n"); abort(); }
/* Generated stub for db_in_transaction */
bool db_in_transaction(struct db *db UNNEEDED)
{ fprintf(stderr, "db_in_transaction called!\n"); abort(); }
/* Generated stub for ecdh_hsmd_setup */
void ecdh_hsmd_setup(int hsm_fd UNNEEDED,
		     void (*failed)(enum status_failreason UNNEEDED,
				    const char *fmt UNNEEDED, ...))
{ fprintf(stderr, "ecdh_hsmd_setup called!\n"); abort(); }
/* Generated stub for fatal */
void   fatal(const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "fatal called!\n"); abort(); }
/* Generated stub for feature_set_for_feature */
struct feature_set *feature_set_for_feature(const tal_t *ctx UNNEEDED, int feature UNNEEDED)
{ fprintf(stderr, "feature_set_for_feature called!\n"); abort(); }
/* Generated stub for feature_set_or */
bool feature_set_or(struct feature_set *a UNNEEDED,
		    const struct feature_set *b TAKES UNNEEDED)
{ fprintf(stderr, "feature_set_or called!\n"); abort(); }
/* Generated stub for free_htlcs */
void free_htlcs(struct lightningd *ld UNNEEDED, const struct channel *channel UNNEEDED)
{ fprintf(stderr, "free_htlcs called!\n"); abort(); }
/* Generated stub for free_unreleased_txs */
void free_unreleased_txs(struct wallet *w UNNEEDED)
{ fprintf(stderr, "free_unreleased_txs called!\n"); abort(); }
/* Generated stub for fromwire_bigsize */
bigsize_t fromwire_bigsize(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_bigsize called!\n"); abort(); }
/* Generated stub for fromwire_channel_id */
void fromwire_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			 struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_channel_id called!\n"); abort(); }
/* Generated stub for fromwire_node_id */
void fromwire_node_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct node_id *id UNNEEDED)
{ fprintf(stderr, "fromwire_node_id called!\n"); abort(); }
/* Generated stub for fromwire_status_fail */
bool fromwire_status_fail(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, enum status_failreason *failreason UNNEEDED, wirestring **desc UNNEEDED)
{ fprintf(stderr, "fromwire_status_fail called!\n"); abort(); }
/* Generated stub for fromwire_status_peer_billboard */
bool fromwire_status_peer_billboard(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, bool *perm UNNEEDED, wirestring **happenings UNNEEDED)
{ fprintf(stderr, "fromwire_status_peer_billboard called!\n"); abort(); }
/* Generated stub for fromwire_status_peer_error */
bool fromwire_status_peer_error(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, struct channel_id *channel UNNEEDED, wirestring **desc UNNEEDED, bool *soft_error UNNEEDED, struct per_peer_state **pps UNNEEDED, u8 **error_for_them UNNEEDED)
{ fprintf(stderr, "fromwire_status_peer_error called!\n"); abort(); }
/* Generated stub for gossip_init */
void gossip_init(struct lightningd *ld UNNEEDED, int connectd_fd UNNEEDED)
{ fprintf(stderr, "gossip_init called!\n"); abort(); }
/* Generated stub for gossip_notify_new_block */
void gossip_notify_new_block(struct lightningd *ld UNNEEDED, u32 blockheight UNNEEDED)
{ fprintf(stderr, "gossip_notify_new_block called!\n"); abort(); }
/* Generated stub for handle_early_opts */
void handle_early_opts(struct lightningd *ld UNNEEDED, int argc UNNEEDED, char *argv[])
{ fprintf(stderr, "handle_early_opts called!\n"); abort(); }
/* Generated stub for handle_opts */
void handle_opts(struct lightningd *ld UNNEEDED, int argc UNNEEDED, char *argv[])
{ fprintf(stderr, "handle_opts called!\n"); abort(); }
/* Generated stub for hash_htlc_key */
size_t hash_htlc_key(const struct htlc_key *htlc_key UNNEEDED)
{ fprintf(stderr, "hash_htlc_key called!\n"); abort(); }
/* Generated stub for hsm_init */
void hsm_init(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "hsm_init called!\n"); abort(); }
/* Generated stub for htlcs_notify_new_block */
void htlcs_notify_new_block(struct lightningd *ld UNNEEDED, u32 height UNNEEDED)
{ fprintf(stderr, "htlcs_notify_new_block called!\n"); abort(); }
/* Generated stub for htlcs_resubmit */
void htlcs_resubmit(struct lightningd *ld UNNEEDED,
		    struct htlc_in_map *unconnected_htlcs_in UNNEEDED)
{ fprintf(stderr, "htlcs_resubmit called!\n"); abort(); }
/* Generated stub for json_add_member */
void json_add_member(struct json_stream *js UNNEEDED,
		     const char *fieldname UNNEEDED,
		     bool quote UNNEEDED,
		     const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "json_add_member called!\n"); abort(); }
/* Generated stub for json_array_end */
void json_array_end(struct json_stream *js UNNEEDED)
{ fprintf(stderr, "json_array_end called!\n"); abort(); }
/* Generated stub for json_array_start */
void json_array_start(struct json_stream *js UNNEEDED, const char *fieldname UNNEEDED)
{ fprintf(stderr, "json_array_start called!\n"); abort(); }
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
/* Generated stub for jsonrpc_listen */
void jsonrpc_listen(struct jsonrpc *rpc UNNEEDED, struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "jsonrpc_listen called!\n"); abort(); }
/* Generated stub for jsonrpc_setup */
void jsonrpc_setup(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "jsonrpc_setup called!\n"); abort(); }
/* Generated stub for load_channels_from_wallet */
struct htlc_in_map *load_channels_from_wallet(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "load_channels_from_wallet called!\n"); abort(); }
/* Generated stub for log_ */
void log_(struct log *log UNNEEDED, enum log_level level UNNEEDED,
	  const struct node_id *node_id UNNEEDED,
	  bool call_notifier UNNEEDED,
	  const char *fmt UNNEEDED, ...)

{ fprintf(stderr, "log_ called!\n"); abort(); }
/* Generated stub for log_backtrace_exit */
void log_backtrace_exit(void)
{ fprintf(stderr, "log_backtrace_exit called!\n"); abort(); }
/* Generated stub for log_backtrace_print */
void log_backtrace_print(const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "log_backtrace_print called!\n"); abort(); }
/* Generated stub for log_prefix */
const char *log_prefix(const struct log *log UNNEEDED)
{ fprintf(stderr, "log_prefix called!\n"); abort(); }
/* Generated stub for log_print_level */
enum log_level log_print_level(struct log *log UNNEEDED)
{ fprintf(stderr, "log_print_level called!\n"); abort(); }
/* Generated stub for log_status_msg */
bool log_status_msg(struct log *log UNNEEDED,
 		    const struct node_id *node_id UNNEEDED,
		    const u8 *msg UNNEEDED)
{ fprintf(stderr, "log_status_msg called!\n"); abort(); }
/* Generated stub for memleak_remove_strmap_ */
void memleak_remove_strmap_(struct htable *memtable UNNEEDED, const struct strmap *m UNNEEDED)
{ fprintf(stderr, "memleak_remove_strmap_ called!\n"); abort(); }
/* Generated stub for new_log */
struct log *new_log(const tal_t *ctx UNNEEDED, struct log_book *record UNNEEDED,
		    const struct node_id *default_node_id UNNEEDED,
		    const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "new_log called!\n"); abort(); }
/* Generated stub for new_log_book */
struct log_book *new_log_book(struct lightningd *ld UNNEEDED, size_t max_mem UNNEEDED)
{ fprintf(stderr, "new_log_book called!\n"); abort(); }
/* Generated stub for new_topology */
struct chain_topology *new_topology(struct lightningd *ld UNNEEDED, struct log *log UNNEEDED)
{ fprintf(stderr, "new_topology called!\n"); abort(); }
/* Generated stub for onchaind_replay_channels */
void onchaind_replay_channels(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "onchaind_replay_channels called!\n"); abort(); }
/* Generated stub for per_peer_state_set_fds_arr */
void per_peer_state_set_fds_arr(struct per_peer_state *pps UNNEEDED, const int *fds UNNEEDED)
{ fprintf(stderr, "per_peer_state_set_fds_arr called!\n"); abort(); }
/* Generated stub for plugins_config */
void plugins_config(struct plugins *plugins UNNEEDED)
{ fprintf(stderr, "plugins_config called!\n"); abort(); }
/* Generated stub for plugins_free */
void plugins_free(struct plugins *plugins UNNEEDED)
{ fprintf(stderr, "plugins_free called!\n"); abort(); }
/* Generated stub for plugins_init */
void plugins_init(struct plugins *plugins UNNEEDED)
{ fprintf(stderr, "plugins_init called!\n"); abort(); }
/* Generated stub for plugins_new */
struct plugins *plugins_new(const tal_t *ctx UNNEEDED, struct log_book *log_book UNNEEDED,
			    struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "plugins_new called!\n"); abort(); }
/* Generated stub for setup_color_and_alias */
void setup_color_and_alias(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "setup_color_and_alias called!\n"); abort(); }
/* Generated stub for setup_topology */
void setup_topology(struct chain_topology *topology UNNEEDED, struct timers *timers UNNEEDED,
		    u32 min_blockheight UNNEEDED, u32 max_blockheight UNNEEDED)
{ fprintf(stderr, "setup_topology called!\n"); abort(); }
/* Generated stub for timer_expired */
void timer_expired(tal_t *ctx UNNEEDED, struct timer *timer UNNEEDED)
{ fprintf(stderr, "timer_expired called!\n"); abort(); }
/* Generated stub for towire_bigsize */
void towire_bigsize(u8 **pptr UNNEEDED, const bigsize_t val UNNEEDED)
{ fprintf(stderr, "towire_bigsize called!\n"); abort(); }
/* Generated stub for towire_channel_id */
void towire_channel_id(u8 **pptr UNNEEDED, const struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "towire_channel_id called!\n"); abort(); }
/* Generated stub for towire_node_id */
void towire_node_id(u8 **pptr UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "towire_node_id called!\n"); abort(); }
/* Generated stub for txfilter_add_derkey */
void txfilter_add_derkey(struct txfilter *filter UNNEEDED,
			 const u8 derkey[PUBKEY_CMPR_LEN])
{ fprintf(stderr, "txfilter_add_derkey called!\n"); abort(); }
/* Generated stub for txfilter_new */
struct txfilter *txfilter_new(const tal_t *ctx UNNEEDED)
{ fprintf(stderr, "txfilter_new called!\n"); abort(); }
/* Generated stub for version */
const char *version(void)
{ fprintf(stderr, "version called!\n"); abort(); }
/* Generated stub for waitblockheight_notify_new_block */
void waitblockheight_notify_new_block(struct lightningd *ld UNNEEDED,
				      u32 block_height UNNEEDED)
{ fprintf(stderr, "waitblockheight_notify_new_block called!\n"); abort(); }
/* Generated stub for wallet_blocks_heights */
void wallet_blocks_heights(struct wallet *w UNNEEDED, u32 def UNNEEDED, u32 *min UNNEEDED, u32 *max UNNEEDED)
{ fprintf(stderr, "wallet_blocks_heights called!\n"); abort(); }
/* Generated stub for wallet_clean_utxos */
void wallet_clean_utxos(struct wallet *w UNNEEDED, struct bitcoind *bitcoind UNNEEDED)
{ fprintf(stderr, "wallet_clean_utxos called!\n"); abort(); }
/* Generated stub for wallet_network_check */
bool wallet_network_check(struct wallet *w UNNEEDED)
{ fprintf(stderr, "wallet_network_check called!\n"); abort(); }
/* Generated stub for wallet_new */
struct wallet *wallet_new(struct lightningd *ld UNNEEDED, struct timers *timers UNNEEDED)
{ fprintf(stderr, "wallet_new called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

struct log *crashlog;

#undef main
int main(int argc UNUSED, char *argv[] UNUSED)
{
	setup_locale();

	char *argv0;
	/* We're assuming we're run from top build dir. */
	const char *answer;

	setup_tmpctx();
	answer = path_canon(tmpctx, "lightningd/test/run-find_my_abspath");

	/* Various different ways we could find ourselves. */
	argv0 = path_join(tmpctx,
			  path_cwd(tmpctx), "lightningd/test/run-find_my_abspath");
	unsetenv("PATH");

	/* Absolute path. */
	assert(streq(find_my_abspath(tmpctx, argv0), answer));

	/* Relative to cwd. */
	argv0 = "lightningd/test/run-find_my_abspath";
	assert(streq(find_my_abspath(tmpctx, argv0), answer));

	/* Using $PATH */
	setenv("PATH", path_join(tmpctx,
				 path_cwd(tmpctx), "lightningd/test"), 1);
	argv0 = "run-find_my_abspath";

	assert(streq(find_my_abspath(tmpctx, argv0), answer));

	/* Even with dummy things in path. */
	char **pathelems = tal_arr(tmpctx, char *, 4);
	pathelems[0] = "/tmp/foo";
	pathelems[1] = "/sbin";
	pathelems[2] = path_join(tmpctx, path_cwd(tmpctx), "lightningd/test");
	pathelems[3] = NULL;

	setenv("PATH", tal_strjoin(tmpctx, pathelems, ":", STR_NO_TRAIL), 1);
	assert(streq(find_my_abspath(tmpctx, argv0), answer));

	assert(!taken_any());
	take_cleanup();
	tal_free(tmpctx);
	return 0;
}
