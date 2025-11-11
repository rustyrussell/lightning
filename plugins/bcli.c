#include "config.h"
#include <bitcoin/block.h>
#include <bitcoin/tx.h>
#include <ccan/array_size/array_size.h>
#include <ccan/cast/cast.h>
#include <ccan/crypto/siphash24/siphash24.h>
#include <ccan/htable/htable_type.h>
#include <ccan/io/io.h>
#include <ccan/mem/mem.h>
#include <ccan/pipecmd/pipecmd.h>
#include <ccan/read_write_all/read_write_all.h>
#include <ccan/tal/grab_file/grab_file.h>
#include <ccan/tal/str/str.h>
#include <common/json_param.h>
#include <common/json_stream.h>
#include <common/memleak.h>
#include <errno.h>
#include <inttypes.h>
#include <plugins/libplugin.h>

/* Bitcoind's web server has a default of 4 threads, with queue depth 16.
 * It will *fail* rather than queue beyond that, so we must not stress it!
 *
 * This is how many request for each priority level we have.
 */
#define BITCOIND_MAX_PARALLEL 4
#define RPC_TRANSACTION_ALREADY_IN_CHAIN -27

enum bitcoind_prio {
	BITCOIND_LOW_PRIO,
	BITCOIND_HIGH_PRIO
};
#define BITCOIND_NUM_PRIO (BITCOIND_HIGH_PRIO+1)

/* State machine for block filtering */
enum bcli_state {
	STATE_INITIALIZING,  /* Starting up, before watches received */
	STATE_PROVISIONAL,   /* Buffering blocks, waiting for watch sync */
	STATE_SYNCING,       /* Receiving watches from lightningd */
	STATE_ACTIVE,        /* Normal operation - filtering enabled */
};

/* Watch for outputs to specific scriptpubkeys */
struct scriptpubkey_watch {
	u8 *scriptpubkey;  /* tal_arr - the script to watch for */
	size_t len;
};

/* Hash table functions for scriptpubkey watches */
static const struct scriptpubkey_watch *scriptpubkey_watch_keyof(const struct scriptpubkey_watch *w)
{
	return w;
}

static size_t scriptpubkey_hash(const struct scriptpubkey_watch *w)
{
	return siphash24(siphash_seed(), w->scriptpubkey, w->len);
}

static bool scriptpubkey_watch_eq(const struct scriptpubkey_watch *a,
				  const struct scriptpubkey_watch *b)
{
	return memeq(a->scriptpubkey, a->len, b->scriptpubkey, b->len);
}

HTABLE_DEFINE_DUPS_TYPE(struct scriptpubkey_watch,
			scriptpubkey_watch_keyof,
			scriptpubkey_hash,
			scriptpubkey_watch_eq,
			scriptpubkey_watch_hash);

/* Watch for specific transaction IDs */
struct txid_watch {
	struct bitcoin_txid txid;  /* The transaction ID to watch for */
};

/* Hash table functions for txid watches */
static const struct bitcoin_txid *txid_watch_keyof(const struct txid_watch *w)
{
	return &w->txid;
}

static size_t txid_hash(const struct bitcoin_txid *txid)
{
	return siphash24(siphash_seed(), txid, sizeof(*txid));
}

static bool txid_watch_eq(const struct txid_watch *w, const struct bitcoin_txid *txid)
{
	return bitcoin_txid_eq(&w->txid, txid);
}

HTABLE_DEFINE_DUPS_TYPE(struct txid_watch,
			txid_watch_keyof,
			txid_hash,
			txid_watch_eq,
			txid_watch_hash);

/* Watch for specific outpoints (outputs being spent) */
struct outpoint_watch {
	struct bitcoin_outpoint outpoint;  /* The output to watch */
};

/* Hash table functions for outpoint watches */
static const struct bitcoin_outpoint *outpoint_watch_keyof(const struct outpoint_watch *w)
{
	return &w->outpoint;
}

static size_t outpoint_hash(const struct bitcoin_outpoint *outpoint)
{
	/* Hash the txid and output index together */
	BUILD_ASSERT(offsetof(struct bitcoin_outpoint, n)
		     == sizeof(((struct bitcoin_outpoint *)NULL)->txid));
	return siphash24(siphash_seed(), outpoint,
			 sizeof(outpoint->txid) + sizeof(outpoint->n));
}

static bool outpoint_watch_eq(const struct outpoint_watch *w, const struct bitcoin_outpoint *outpoint)
{
	return bitcoin_txid_eq(&w->outpoint.txid, &outpoint->txid)
		&& w->outpoint.n == outpoint->n;
}

HTABLE_DEFINE_DUPS_TYPE(struct outpoint_watch,
			outpoint_watch_keyof,
			outpoint_hash,
			outpoint_watch_eq,
			outpoint_watch_hash);

/* Buffered block during startup */
struct buffered_block {
	struct list_node list;
	u32 height;
	struct bitcoin_blkid blkid;
	/* nul terminated */
	const char *raw_block;
};

struct bitcoind {
	/* eg. "bitcoin-cli" */
	char *cli;

	/* -datadir arg for bitcoin-cli. */
	char *datadir;

	/* bitcoind's version, used for compatibility checks. */
	u32 version;

	/* Is bitcoind synced?  If not, we retry. */
	bool synced;

	/* How many high/low prio requests are we running (it's ratelimited) */
	size_t num_requests[BITCOIND_NUM_PRIO];

	/* Pending requests (high and low prio). */
	struct list_head pending[BITCOIND_NUM_PRIO];

	/* In flight requests (in a list for memleak detection) */
	struct list_head current;

	/* If non-zero, time we first hit a bitcoind error. */
	unsigned int error_count;
	struct timemono first_error_time;

	/* How long to keep trying to contact bitcoind
	 * before fatally exiting. */
	u64 retry_timeout;

	/* Passthrough parameters for bitcoin-cli */
	char *rpcuser, *rpcpass, *rpcconnect, *rpcport;
	u64 rpcclienttimeout;

	/* Whether we fake fees (regtest) */
	bool fake_fees;

	/* Override in case we're developer mode for testing*/
	bool dev_no_fake_fees;

	/* Block polling state */
	u32 last_height;
	/* Valid if last_height != 0 */
	struct bitcoin_blkid last_blkid;
	struct plugin_timer *poll_timer;
	/* Seconds between polls (default 10) */
	u64 poll_interval;
};

/* Main plugin state structure */
struct bcli_plugin {
	/* Bitcoin backend interface */
	struct bitcoind *bitcoind;

	/* State machine for block filtering */
	enum bcli_state state;

	/* Hash tables for O(1) lookup, add, and remove */
	struct scriptpubkey_watch_hash *scriptpubkey_watches;
	struct txid_watch_hash *txid_watches;
	struct outpoint_watch_hash *outpoint_watches;

	/* Buffered blocks during startup (linked list) */
	struct list_head buffered_blocks;
};

static struct bitcoind *bitcoind;
static struct bcli_plugin *bcli_plugin;

struct bitcoin_cli {
	struct list_node list;
	int fd;
	int *exitstatus;
	pid_t pid;
	const char **args;
	struct timeabs start;
	enum bitcoind_prio prio;
	char *output;
	size_t output_bytes;
	size_t new_output;
	struct command_result *(*process)(struct bitcoin_cli *);
	struct command *cmd;
	/* Used to stash content between multiple calls */
	void *stash;
};

/* Initialize bitcoind structure */
static struct bitcoind *new_bitcoind(const tal_t *ctx)
{
	bitcoind = tal(ctx, struct bitcoind);

	bitcoind->cli = NULL;
	bitcoind->datadir = NULL;
	for (size_t i = 0; i < BITCOIND_NUM_PRIO; i++) {
		bitcoind->num_requests[i] = 0;
		list_head_init(&bitcoind->pending[i]);
	}
	list_head_init(&bitcoind->current);
	bitcoind->error_count = 0;
	bitcoind->retry_timeout = 60;
	bitcoind->rpcuser = NULL;
	bitcoind->rpcpass = NULL;
	bitcoind->rpcconnect = NULL;
	bitcoind->rpcport = NULL;
	/* Do not exceed retry_timeout value to avoid a bitcoind hang,
	   although normal rpcclienttimeout default value is 900. */
	bitcoind->rpcclienttimeout = 60;
	bitcoind->dev_no_fake_fees = false;

	/* Initialize block polling state */
	bitcoind->last_height = 0;
	bitcoind->poll_timer = NULL;
	bitcoind->poll_interval = 10;  /* Default to 10 seconds */

	return bitcoind;
}

/* Initialize the plugin state structure */
static struct bcli_plugin *new_bcli_plugin(const tal_t *ctx, struct bitcoind *bitcoind_ptr)
{
	struct bcli_plugin *plugin = tal(ctx, struct bcli_plugin);

	plugin->bitcoind = bitcoind_ptr;
	plugin->state = STATE_INITIALIZING;
	plugin->scriptpubkey_watches = new_htable(plugin, scriptpubkey_watch_hash);
	plugin->txid_watches = new_htable(plugin, txid_watch_hash);
	plugin->outpoint_watches = new_htable(plugin, outpoint_watch_hash);
	list_head_init(&plugin->buffered_blocks);

	return plugin;
}

/* Helper function to get state name for logging */
static const char *bcli_state_name(enum bcli_state state)
{
	switch (state) {
	case STATE_INITIALIZING: return "INITIALIZING";
	case STATE_PROVISIONAL: return "PROVISIONAL";
	case STATE_SYNCING: return "SYNCING";
	case STATE_ACTIVE: return "ACTIVE";
	}
	return "UNKNOWN";
}

/* Buffer a block during PROVISIONAL/SYNCING states */
static void buffer_block(u32 height,
			 const struct bitcoin_blkid *blkid,
			 const char *raw_block,
			 size_t raw_block_len)
{
	struct buffered_block *block = tal(bcli_plugin, struct buffered_block);

	block->height = height;
	block->blkid = *blkid;
	block->raw_block = tal_strndup(block, raw_block, raw_block_len);
	list_add_tail(&bcli_plugin->buffered_blocks, &block->list);
}

/* Check if transaction ID is being watched */
static bool is_txid_watched(const struct bitcoin_txid *txid)
{
	return txid_watch_hash_exists(bcli_plugin->txid_watches, txid);
}

/* Check if any outputs in the transaction have watched scriptpubkeys */
static bool has_watched_scriptpubkey(const struct bitcoin_tx *tx)
{
	for (size_t i = 0; i < tx->wtx->num_outputs; i++) {
		struct scriptpubkey_watch w;

		w.scriptpubkey = tx->wtx->outputs[i].script;
		w.len = tx->wtx->outputs[i].script_len;
		if (scriptpubkey_watch_hash_exists(bcli_plugin->scriptpubkey_watches, &w))
			return true;
	}
	return false;
}

/* Check if any inputs in the transaction spend watched outpoints */
static bool spends_watched_outpoint(const struct bitcoin_tx *tx)
{
	for (size_t i = 0; i < tx->wtx->num_inputs; i++) {
		struct bitcoin_outpoint outpoint;

		memcpy(&outpoint.txid, tx->wtx->inputs[i].txhash, sizeof(outpoint.txid));
		outpoint.n = tx->wtx->inputs[i].index;

		if (outpoint_watch_hash_exists(bcli_plugin->outpoint_watches,
					       &outpoint))
			return true;
	}
	return false;
}

/* Filter a block and send relevant transactions to lightningd
 * Returns true if any matches found */
static bool filter_and_send_block(struct plugin *plugin,
				  u32 height,
				  const struct bitcoin_blkid *block_hash,
				  const char *raw_block_hex,
				  size_t raw_block_hex_len)
{
	struct bitcoin_block *block;
	const char **relevant_txs;
	struct json_stream *notification;

	/* Parse the block */
	block = bitcoin_block_from_hex(tmpctx, chainparams, raw_block_hex, raw_block_hex_len);
	if (!block) {
		plugin_log(plugin, LOG_UNUSUAL,
			   "Failed to parse block %s at height %u",
			   fmt_bitcoin_blkid(tmpctx, block_hash), height);
		return false;
	}

	plugin_log(plugin, LOG_DBG,
		   "Filtering block %u (%s) with %zu transactions",
		   height, fmt_bitcoin_blkid(tmpctx, block_hash), tal_count(block->tx));

	/* Collect all matching transactions */
	relevant_txs = tal_arr(tmpctx, const char *, 0);

	/* Iterate through all transactions in the block */
	for (size_t i = 0; i < tal_count(block->tx); i++) {
		const struct bitcoin_tx *tx = block->tx[i];
		struct bitcoin_txid txid;
		bool tx_matches = false;

		bitcoin_txid(tx, &txid);

		/* Check all watch types */
		if (is_txid_watched(&txid)) {
			plugin_log(plugin, LOG_DBG,
				   "Found watched txid: %s",
				   fmt_bitcoin_txid(tmpctx, &txid));
			tx_matches = true;
		} else if (has_watched_scriptpubkey(tx)) {
			plugin_log(plugin, LOG_DBG,
				   "Found watched scriptpubkey in tx %s",
				   fmt_bitcoin_txid(tmpctx, &txid));
			tx_matches = true;
		} else if (spends_watched_outpoint(tx)) {
			plugin_log(plugin, LOG_DBG,
				   "Found watched outpoint spent in tx %s",
				   fmt_bitcoin_txid(tmpctx, &txid));
			tx_matches = true;
		}

		/* If transaction matches, add to relevant_txs array */
		if (tx_matches) {
			const char *tx_hex = tal_hex(relevant_txs, linearize_tx(tmpctx, tx));
			tal_arr_expand(&relevant_txs, tx_hex);
		}
	}

	/* Send notification only if we found matches */
	if (tal_count(relevant_txs) > 0) {
		plugin_log(plugin, LOG_DBG,
			   "Found %zu relevant transactions in block %u",
			   tal_count(relevant_txs), height);

		notification = plugin_notification_start(tmpctx, "bcli_block_detected");
		json_add_u32(notification, "height", height);
		json_add_bitcoin_blkid(notification, "hash", block_hash);
		/* FIXME: Do we want the block header? */

		json_array_start(notification, "relevant_txs");
		for (size_t i = 0; i < tal_count(relevant_txs); i++) {
			json_add_string(notification, NULL, relevant_txs[i]);
		}
		json_array_end(notification);

		plugin_notification_end(plugin, notification);
		return true;
	} else {
		plugin_log(plugin, LOG_DBG,
			   "No matches found in block %u", height);
		return false;
	}
}

/* Process buffered blocks when transitioning to ACTIVE */
static void process_buffered_blocks(struct plugin *plugin)
{
	struct buffered_block *block;
	size_t count = 0;
	size_t matches = 0;

	while ((block = list_pop(&bcli_plugin->buffered_blocks, struct buffered_block, list)) != NULL) {
		plugin_log(plugin, LOG_DBG,
			   "Processing buffered block %u: %s",
			   block->height, fmt_bitcoin_blkid(tmpctx, &block->blkid));

		if (filter_and_send_block(plugin, block->height, &block->blkid,
					  block->raw_block, strlen(block->raw_block)))
			matches++;

		count++;
		tal_free(block);
	}

	if (count > 0) {
		plugin_log(plugin, LOG_INFORM,
			   "Processed %zu buffered blocks (%zu with matches)", count, matches);
	}
}

/* Helpers to remove one entry from the hash table.  If there is more than one,
 * they are identical, and only one is removed.
 * Returns true if found and removed, false otherwise */
static bool remove_scriptpubkey_watch(u8 *scriptpubkey)
{
	struct scriptpubkey_watch w, *watch;
	struct scriptpubkey_watch_hash_iter it;

	w.scriptpubkey = scriptpubkey;
	w.len = tal_bytelen(scriptpubkey);
	watch = scriptpubkey_watch_hash_getfirst(bcli_plugin->scriptpubkey_watches,
						 &w, &it);
	if (!watch)
		return false;

	scriptpubkey_watch_hash_del(bcli_plugin->scriptpubkey_watches, watch);
	tal_free(watch);
	return true;
}

static bool remove_txid_watch(const struct bitcoin_txid *txid)
{
	struct txid_watch *watch;
	struct txid_watch_hash_iter it;

	watch = txid_watch_hash_getfirst(bcli_plugin->txid_watches, txid, &it);
	if (!watch)
		return false;

	txid_watch_hash_del(bcli_plugin->txid_watches, watch);
	tal_free(watch);
	return true;
}

static bool remove_outpoint_watch(const struct bitcoin_outpoint *outpoint)
{
	struct outpoint_watch *watch;
	struct outpoint_watch_hash_iter it;

	watch = outpoint_watch_hash_getfirst(bcli_plugin->outpoint_watches, outpoint, &it);
	if (!watch)
		return false;

	outpoint_watch_hash_del(bcli_plugin->outpoint_watches, watch);
	tal_free(watch);
	return true;
}

/* Add the n'th arg to *args, incrementing n and keeping args of size n+1 */
static void add_arg(const char ***args, const char *arg TAKES)
{
	if (taken(arg))
		tal_steal(*args, arg);
	tal_arr_expand(args, arg);
}

static const char **gather_argsv(const tal_t *ctx, const char *cmd, va_list ap)
{
	const char **args = tal_arr(ctx, const char *, 1);
	const char *arg;

	args[0] = bitcoind->cli ? bitcoind->cli : chainparams->cli;
	if (chainparams->cli_args)
		add_arg(&args, chainparams->cli_args);
	if (bitcoind->datadir)
		add_arg(&args, tal_fmt(args, "-datadir=%s", bitcoind->datadir));
	if (bitcoind->rpcclienttimeout) {
		/* Use the maximum value of rpcclienttimeout and retry_timeout to avoid
		   the bitcoind backend hanging for too long. */
		if (bitcoind->retry_timeout &&
		    bitcoind->retry_timeout > bitcoind->rpcclienttimeout)
			bitcoind->rpcclienttimeout = bitcoind->retry_timeout;

		add_arg(&args,
			tal_fmt(args, "-rpcclienttimeout=%"PRIu64, bitcoind->rpcclienttimeout));
	}
	if (bitcoind->rpcconnect)
		add_arg(&args,
			tal_fmt(args, "-rpcconnect=%s", bitcoind->rpcconnect));
	if (bitcoind->rpcport)
		add_arg(&args,
			tal_fmt(args, "-rpcport=%s", bitcoind->rpcport));
	if (bitcoind->rpcuser)
		add_arg(&args, tal_fmt(args, "-rpcuser=%s", bitcoind->rpcuser));
	if (bitcoind->rpcpass)
		// Always pipe the rpcpassword via stdin. Do not pass it using an
		// `-rpcpassword` argument - secrets in arguments can leak when listing
		// system processes.
		add_arg(&args, "-stdinrpcpass");

	add_arg(&args, cmd);
	while ((arg = va_arg(ap, char *)) != NULL)
		add_arg(&args, arg);
	add_arg(&args, NULL);

	return args;
}

static LAST_ARG_NULL const char **
gather_args(const tal_t *ctx, const char *cmd, ...)
{
	va_list ap;
	const char **ret;

	va_start(ap, cmd);
	ret = gather_argsv(ctx, cmd, ap);
	va_end(ap);

	return ret;
}

static struct io_plan *read_more(struct io_conn *conn, struct bitcoin_cli *bcli)
{
	bcli->output_bytes += bcli->new_output;
	if (bcli->output_bytes == tal_count(bcli->output))
		tal_resize(&bcli->output, bcli->output_bytes * 2);
	return io_read_partial(conn, bcli->output + bcli->output_bytes,
			       tal_count(bcli->output) - bcli->output_bytes,
			       &bcli->new_output, read_more, bcli);
}

static struct io_plan *output_init(struct io_conn *conn, struct bitcoin_cli *bcli)
{
	bcli->output_bytes = bcli->new_output = 0;
	bcli->output = tal_arr(bcli, char, 100);
	return read_more(conn, bcli);
}

static void next_bcli(enum bitcoind_prio prio);

/* For printing: simple string of args (no secrets!) */
static char *args_string(const tal_t *ctx, const char **args)
{
	size_t i;
	char *ret = tal_strdup(ctx, args[0]);

	for (i = 1; args[i]; i++) {
		ret = tal_strcat(ctx, take(ret), " ");
		if (strstarts(args[i], "-rpcpassword")) {
			ret = tal_strcat(ctx, take(ret), "-rpcpassword=...");
		} else if (strstarts(args[i], "-rpcuser")) {
			ret = tal_strcat(ctx, take(ret), "-rpcuser=...");
		} else {
			ret = tal_strcat(ctx, take(ret), args[i]);
		}
	}
	return ret;
}

static char *bcli_args(const tal_t *ctx, struct bitcoin_cli *bcli)
{
    return args_string(ctx, bcli->args);
}

/* Only set as destructor once bcli is in current. */
static void destroy_bcli(struct bitcoin_cli *bcli)
{
	list_del_from(&bitcoind->current, &bcli->list);
}

static struct command_result *retry_bcli(struct command *cmd,
					 struct bitcoin_cli *bcli)
{
	list_del_from(&bitcoind->current, &bcli->list);
	tal_del_destructor(bcli, destroy_bcli);

	list_add_tail(&bitcoind->pending[bcli->prio], &bcli->list);
	tal_free(bcli->output);
	next_bcli(bcli->prio);
	return timer_complete(cmd);
}

/* We allow 60 seconds of spurious errors, eg. reorg. */
static void bcli_failure(struct bitcoin_cli *bcli,
                         int exitstatus)
{
	struct timerel t;

	if (!bitcoind->error_count)
		bitcoind->first_error_time = time_mono();

	t = timemono_between(time_mono(), bitcoind->first_error_time);
	if (time_greater(t, time_from_sec(bitcoind->retry_timeout)))
		plugin_err(bcli->cmd->plugin,
		           "%s exited %u (after %u other errors) '%.*s'; "
		           "we have been retrying command for "
		           "--bitcoin-retry-timeout=%"PRIu64" seconds; "
		           "bitcoind setup or our --bitcoin-* configs broken?",
		           bcli_args(tmpctx, bcli),
		           exitstatus,
		           bitcoind->error_count,
		           (int)bcli->output_bytes,
		           bcli->output,
		           bitcoind->retry_timeout);

	plugin_log(bcli->cmd->plugin, LOG_UNUSUAL, "%s exited with status %u",
		   bcli_args(tmpctx, bcli), exitstatus);
	bitcoind->error_count++;

	/* Retry in 1 second */
	command_timer(bcli->cmd, time_from_sec(1), retry_bcli, bcli);
}

static void bcli_finished(struct io_conn *conn UNUSED, struct bitcoin_cli *bcli)
{
	int ret, status;
	struct command_result *res;
	enum bitcoind_prio prio = bcli->prio;
	u64 msec = time_to_msec(time_between(time_now(), bcli->start));

	/* If it took over 10 seconds, that's rather strange. */
	if (msec > 10000)
		plugin_log(bcli->cmd->plugin, LOG_UNUSUAL,
		           "bitcoin-cli: finished %s (%"PRIu64" ms)",
		           bcli_args(tmpctx, bcli), msec);

	assert(bitcoind->num_requests[prio] > 0);

	/* FIXME: If we waited for SIGCHILD, this could never hang! */
	while ((ret = waitpid(bcli->pid, &status, 0)) < 0 && errno == EINTR);
	if (ret != bcli->pid)
		plugin_err(bcli->cmd->plugin, "%s %s", bcli_args(tmpctx, bcli),
		           ret == 0 ? "not exited?" : strerror(errno));

	if (!WIFEXITED(status))
		plugin_err(bcli->cmd->plugin, "%s died with signal %i",
		           bcli_args(tmpctx, bcli),
		           WTERMSIG(status));

	/* Implicit nonzero_exit_ok == false */
	if (!bcli->exitstatus) {
		if (WEXITSTATUS(status) != 0) {
			bcli_failure(bcli, WEXITSTATUS(status));
			bitcoind->num_requests[prio]--;
			goto done;
		}
	} else
		*bcli->exitstatus = WEXITSTATUS(status);

	if (WEXITSTATUS(status) == 0)
		bitcoind->error_count = 0;

	bitcoind->num_requests[bcli->prio]--;

	res = bcli->process(bcli);
	if (!res)
		bcli_failure(bcli, WEXITSTATUS(status));
	else
		tal_free(bcli);

done:
	next_bcli(prio);
}

static void next_bcli(enum bitcoind_prio prio)
{
	struct bitcoin_cli *bcli;
	struct io_conn *conn;
	int in;

	if (bitcoind->num_requests[prio] >= BITCOIND_MAX_PARALLEL)
		return;

	bcli = list_pop(&bitcoind->pending[prio], struct bitcoin_cli, list);
	if (!bcli)
		return;

	bcli->pid = pipecmdarr(&in, &bcli->fd, &bcli->fd,
			       cast_const2(char **, bcli->args));
	if (bcli->pid < 0)
		plugin_err(bcli->cmd->plugin, "%s exec failed: %s",
			   bcli->args[0], strerror(errno));


	if (bitcoind->rpcpass)
		write_all(in, bitcoind->rpcpass, strlen(bitcoind->rpcpass));

	close(in);

	bcli->start = time_now();

	bitcoind->num_requests[prio]++;

	/* We don't keep a pointer to this, but it's not a leak */
	conn = notleak(io_new_conn(bcli, bcli->fd, output_init, bcli));
	io_set_finish(conn, bcli_finished, bcli);

	list_add_tail(&bitcoind->current, &bcli->list);
	tal_add_destructor(bcli, destroy_bcli);
}

static void
start_bitcoin_cliv(const tal_t *ctx,
		   struct command *cmd,
		   struct command_result *(*process)(struct bitcoin_cli *),
		   bool nonzero_exit_ok,
		   enum bitcoind_prio prio,
		   void *stash,
		   const char *method,
		   va_list ap)
{
	struct bitcoin_cli *bcli = tal(bitcoind, struct bitcoin_cli);

	bcli->process = process;
	bcli->cmd = cmd;
	bcli->prio = prio;

	if (nonzero_exit_ok)
		bcli->exitstatus = tal(bcli, int);
	else
		bcli->exitstatus = NULL;

	bcli->args = gather_argsv(bcli, method, ap);
	bcli->stash = stash;

	list_add_tail(&bitcoind->pending[bcli->prio], &bcli->list);
	next_bcli(bcli->prio);
}

/* If ctx is non-NULL, and is freed before we return, we don't call process().
 * process returns false() if it's a spurious error, and we should retry. */
static void LAST_ARG_NULL
start_bitcoin_cli(const tal_t *ctx,
		  struct command *cmd,
		  struct command_result *(*process)(struct bitcoin_cli *),
		  bool nonzero_exit_ok,
		  enum bitcoind_prio prio,
		  void *stash,
		  const char *method,
		  ...)
{
	va_list ap;

	va_start(ap, method);
	start_bitcoin_cliv(ctx, cmd, process, nonzero_exit_ok, prio, stash, method,
			   ap);
	va_end(ap);
}

static size_t length_no_trailing(const char *str, size_t len)
{
	while (len > 0 && cisspace(str[len-1]))
		len--;

	return len;
}

static struct command_result *command_err_bcli_badjson(struct bitcoin_cli *bcli,
						       const char *errmsg)
{
	char *err = tal_fmt(bcli, "%s: bad JSON: %s (%.*s)",
			    bcli_args(tmpctx, bcli), errmsg,
			    (int)bcli->output_bytes, bcli->output);
	return command_done_err(bcli->cmd, BCLI_ERROR, err, NULL);
}

static struct command_result *process_getutxout(struct bitcoin_cli *bcli)
{
	const jsmntok_t *tokens;
	struct json_stream *response;
	struct bitcoin_tx_output output;
	const char *err;

	/* As of at least v0.15.1.0, bitcoind returns "success" but an empty
	   string on a spent txout. */
	if (*bcli->exitstatus != 0 || bcli->output_bytes == 0) {
		response = jsonrpc_stream_success(bcli->cmd);
		json_add_null(response, "amount");
		json_add_null(response, "script");

		return command_finished(bcli->cmd, response);
	}

	tokens = json_parse_simple(bcli->output, bcli->output,
				   bcli->output_bytes);
	if (!tokens) {
		return command_err_bcli_badjson(bcli, "cannot parse");
	}

	err = json_scan(tmpctx, bcli->output, tokens,
		       "{value:%,scriptPubKey:{hex:%}}",
		       JSON_SCAN(json_to_bitcoin_amount,
				 &output.amount.satoshis), /* Raw: bitcoind */
		       JSON_SCAN_TAL(bcli, json_tok_bin_from_hex,
				     &output.script));
	if (err)
		return command_err_bcli_badjson(bcli, err);

	response = jsonrpc_stream_success(bcli->cmd);
	json_add_sats(response, "amount", output.amount);
	json_add_string(response, "script", tal_hex(response, output.script));

	return command_finished(bcli->cmd, response);
}

static struct command_result *process_getblockchaininfo(struct bitcoin_cli *bcli)
{
	const jsmntok_t *tokens;
	struct json_stream *response;
	bool ibd;
	u32 headers, blocks;
	const char *chain, *err;

	tokens = json_parse_simple(bcli->output,
				   bcli->output, bcli->output_bytes);
	if (!tokens) {
		return command_err_bcli_badjson(bcli, "cannot parse");
	}

	err = json_scan(tmpctx, bcli->output, tokens,
			"{chain:%,headers:%,blocks:%,initialblockdownload:%}",
			JSON_SCAN_TAL(tmpctx, json_strdup, &chain),
			JSON_SCAN(json_to_number, &headers),
			JSON_SCAN(json_to_number, &blocks),
			JSON_SCAN(json_to_bool, &ibd));
	if (err)
		return command_err_bcli_badjson(bcli, err);

	response = jsonrpc_stream_success(bcli->cmd);
	json_add_string(response, "chain", chain);
	json_add_u32(response, "headercount", headers);
	json_add_u32(response, "blockcount", blocks);
	json_add_bool(response, "ibd", ibd);

	return command_finished(bcli->cmd, response);
}

struct estimatefee_params {
	u32 blocks;
	const char *style;
};

static const struct estimatefee_params estimatefee_params[] = {
	{ 2, "CONSERVATIVE" },
	{ 6, "ECONOMICAL" },
	{ 12, "ECONOMICAL" },
	{ 100, "ECONOMICAL" },
};

struct estimatefees_stash {
	/* This is max(mempoolminfee,minrelaytxfee) */
	u64 perkb_floor;
	u32 cursor;
	/* FIXME: We use u64 but lightningd will store them as u32. */
	u64 perkb[ARRAY_SIZE(estimatefee_params)];
};

static struct command_result *
estimatefees_null_response(struct bitcoin_cli *bcli)
{
	struct json_stream *response = jsonrpc_stream_success(bcli->cmd);

	/* We give a floor, which is the standard minimum */
	json_array_start(response, "feerates");
	json_array_end(response);
	json_add_u32(response, "feerate_floor", 1000);

	return command_finished(bcli->cmd, response);
}

static struct command_result *
estimatefees_parse_feerate(struct bitcoin_cli *bcli, u64 *feerate)
{
	const jsmntok_t *tokens;

	tokens = json_parse_simple(bcli->output,
				   bcli->output, bcli->output_bytes);
	if (!tokens) {
		return command_err_bcli_badjson(bcli, "cannot parse");
	}

	if (json_scan(tmpctx, bcli->output, tokens, "{feerate:%}",
		      JSON_SCAN(json_to_bitcoin_amount, feerate)) != NULL) {
		/* Paranoia: if it had a feerate, but was malformed: */
		if (json_get_member(bcli->output, tokens, "feerate"))
			return command_err_bcli_badjson(bcli, "cannot scan");
		/* Regtest fee estimation is generally awful: Fake it at min. */
		if (bitcoind->fake_fees) {
			*feerate = 1000;
			return NULL;
		}
		/* We return null if estimation failed, and bitcoin-cli will
		 * exit with 0 but no feerate field on failure. */
		return estimatefees_null_response(bcli);
	}

	return NULL;
}

static struct command_result *process_sendrawtransaction(struct bitcoin_cli *bcli)
{
	struct json_stream *response;

	/* This is useful for functional tests. */
	if (bcli->exitstatus)
		plugin_log(bcli->cmd->plugin, LOG_DBG,
			   "sendrawtx exit %i (%s) %.*s",
			   *bcli->exitstatus, bcli_args(tmpctx, bcli),
			   *bcli->exitstatus ?
				(u32)bcli->output_bytes-1 : 0,
				bcli->output);

	response = jsonrpc_stream_success(bcli->cmd);
	json_add_bool(response, "success",
		      *bcli->exitstatus == 0 ||
			  *bcli->exitstatus ==
			      RPC_TRANSACTION_ALREADY_IN_CHAIN);
	json_add_string(response, "errmsg",
			*bcli->exitstatus ?
			tal_strndup(bcli->cmd,
				    bcli->output, bcli->output_bytes-1)
			: "");

	return command_finished(bcli->cmd, response);
}

struct getrawblock_stash {
	const char *block_hash;
	u32 block_height;
	int *peers;
};

/* Mutual recursion. */
static struct command_result *getrawblock(struct bitcoin_cli *bcli);

static struct command_result *process_rawblock(struct bitcoin_cli *bcli)
{
	struct json_stream *response;
	struct getrawblock_stash *stash = bcli->stash;

	response = jsonrpc_stream_success(bcli->cmd);
	json_add_string(response, "blockhash", stash->block_hash);
	json_add_stringn(response, "block", bcli->output,
			 length_no_trailing(bcli->output, bcli->output_bytes));

	return command_finished(bcli->cmd, response);
}

static struct command_result *process_getblockfrompeer(struct bitcoin_cli *bcli)
{
	/* Remove the peer that we tried to get the block from and move along,
	 * we may also check on errors here */
	struct getrawblock_stash *stash = bcli->stash;

	if (bcli->exitstatus && *bcli->exitstatus != 0) {
		/* We still continue with the execution if we can not fetch the
		 * block from peer */
		plugin_log(bcli->cmd->plugin, LOG_DBG,
			   "failed to fetch block %s from peer %i, skip.",
			   stash->block_hash, stash->peers[tal_count(stash->peers) - 1]);
	} else {
		plugin_log(bcli->cmd->plugin, LOG_DBG,
			   "try to fetch block %s from peer %i.",
			   stash->block_hash, stash->peers[tal_count(stash->peers) - 1]);
	}
	tal_resize(&stash->peers, tal_count(stash->peers) - 1);

	/* `getblockfrompeer` is an async call. sleep for a second to allow the
	 * block to be delivered by the peer. fixme: We could also sleep for
	 * double the last ping here (with sanity limit)*/
	sleep(1);

	return getrawblock(bcli);
}

static struct command_result *process_getpeerinfo(struct bitcoin_cli *bcli)
{
	const jsmntok_t *t, *toks;
	struct getrawblock_stash *stash = bcli->stash;
	size_t i;

	toks =
	    json_parse_simple(bcli->output, bcli->output, bcli->output_bytes);

	if (!toks) {
		return command_err_bcli_badjson(bcli, "cannot parse");
	}

	stash->peers = tal_arr(bcli->stash, int, 0);

	json_for_each_arr(i, t, toks)
	{
		int id;
		u8 *services;

		if (json_scan(tmpctx, bcli->output, t, "{id:%,services:%}",
			      JSON_SCAN(json_to_int, &id),
			      JSON_SCAN_TAL(tmpctx, json_tok_bin_from_hex, &services)) == NULL) {
			/* From bitcoin source:
			 *  // NODE_NETWORK means that the node is capable of serving the complete block chain. It is currently
			 *  // set by all Bitcoin Core non pruned nodes, and is unset by SPV clients or other light clients.
			 * NODE_NETWORK = (1 << 0)
			 */
			if (tal_count(services) > 0 && (services[tal_count(services)-1] & (1<<0))) {
				// fixme: future optimization: sort by last ping
				tal_arr_expand(&stash->peers, id);
			}
		}
	}

	if (tal_count(stash->peers) <= 0) {
		/* We don't have peers yet, retry from `getrawblock` */
		plugin_log(bcli->cmd->plugin, LOG_DBG,
			   "got an empty peer list.");
		return getrawblock(bcli);
	}

	start_bitcoin_cli(NULL, bcli->cmd, process_getblockfrompeer, true,
			  BITCOIND_HIGH_PRIO, stash, "getblockfrompeer",
			  stash->block_hash,
			  take(tal_fmt(NULL, "%i", stash->peers[tal_count(stash->peers) - 1])), NULL);

	return command_still_pending(bcli->cmd);
}

static struct command_result *process_getrawblock(struct bitcoin_cli *bcli)
{
	/* We failed to get the raw block. */
	if (bcli->exitstatus && *bcli->exitstatus != 0) {
		struct getrawblock_stash *stash = bcli->stash;

		plugin_log(bcli->cmd->plugin, LOG_DBG,
			   "failed to fetch block %s from the bitcoin backend (maybe pruned).",
			   stash->block_hash);

		if (bitcoind->version >= 230000) {
			/* `getblockformpeer` was introduced in v23.0.0 */

			if (!stash->peers) {
				/* We don't have peers to fetch blocks from, get
				 * some! */
				start_bitcoin_cli(NULL, bcli->cmd,
						  process_getpeerinfo, true,
						  BITCOIND_HIGH_PRIO, stash,
						  "getpeerinfo", NULL);

				return command_still_pending(bcli->cmd);
			}

			if (tal_count(stash->peers) > 0) {
				/* We have peers left that we can ask for the
				 * block */
				start_bitcoin_cli(
				    NULL, bcli->cmd, process_getblockfrompeer,
				    true, BITCOIND_HIGH_PRIO, stash,
				    "getblockfrompeer", stash->block_hash,
				    take(tal_fmt(NULL, "%i", stash->peers[tal_count(stash->peers) - 1])),
				    NULL);

				return command_still_pending(bcli->cmd);
			}

			/* We failed to fetch the block from from any peer we
			 * got. */
			plugin_log(
			    bcli->cmd->plugin, LOG_DBG,
			    "asked all known peers about block %s, retry",
			    stash->block_hash);
			stash->peers = tal_free(stash->peers);
		}

		return NULL;
	}

	return process_rawblock(bcli);
}

static struct command_result *
getrawblockbyheight_notfound(struct bitcoin_cli *bcli)
{
	struct json_stream *response;

	response = jsonrpc_stream_success(bcli->cmd);
	json_add_null(response, "blockhash");
	json_add_null(response, "block");

	return command_finished(bcli->cmd, response);
}

static struct command_result *getrawblock(struct bitcoin_cli *bcli)
{
	struct getrawblock_stash *stash = bcli->stash;

	start_bitcoin_cli(NULL, bcli->cmd, process_getrawblock, true,
			  BITCOIND_HIGH_PRIO, stash, "getblock",
			  stash->block_hash,
			  /* Non-verbose: raw block. */
			  "0", NULL);

	return command_still_pending(bcli->cmd);
}

static struct command_result *process_getblockhash(struct bitcoin_cli *bcli)
{
	struct getrawblock_stash *stash = bcli->stash;

	/* If it failed with error 8, give an empty response. */
	if (bcli->exitstatus && *bcli->exitstatus != 0) {
		/* Other error means we have to retry. */
		if (*bcli->exitstatus != 8)
			return NULL;
		return getrawblockbyheight_notfound(bcli);
	}

	stash->block_hash = tal_strndup(stash, bcli->output,
					length_no_trailing(bcli->output,
							   bcli->output_bytes));
	if (strlen(stash->block_hash) != 64) {
		return command_err_bcli_badjson(bcli, "bad blockhash");
	}

	return getrawblock(bcli);
}

/* Get a raw block given its height.
 * Calls `getblockhash` then `getblock` to retrieve it from bitcoin_cli.
 * Will return early with null fields if block isn't known (yet).
 */
static struct command_result *getrawblockbyheight(struct command *cmd,
                                                  const char *buf,
                                                  const jsmntok_t *toks)
{
	struct getrawblock_stash *stash;
	u32 *height;

	/* bitcoin-cli wants a string. */
	if (!param(cmd, buf, toks,
	           p_req("height", param_number, &height),
	           NULL))
		return command_param_failed();

	stash = tal(cmd, struct getrawblock_stash);
	stash->block_height = *height;
	stash->peers = NULL;
	tal_free(height);

	start_bitcoin_cli(NULL, cmd, process_getblockhash, true,
			  BITCOIND_LOW_PRIO, stash,
			  "getblockhash",
			  take(tal_fmt(NULL, "%u", stash->block_height)),
			  NULL);

	return command_still_pending(cmd);
}

/* ============ Watch Management RPC Commands ============ */

/* Register a scriptpubkey to watch for in blocks */
static struct command_result *json_watch_scriptpubkey(struct command *cmd,
						      const char *buf,
						      const jsmntok_t *toks)
{
	struct scriptpubkey_watch *watch = tal(cmd, struct scriptpubkey_watch);

	if (!param(cmd, buf, toks,
		   p_req("scriptpubkey", param_bin_from_hex, &watch->scriptpubkey),
		   NULL))
		return command_param_failed();

	watch->len = tal_bytelen(watch->scriptpubkey);
	scriptpubkey_watch_hash_add(bcli_plugin->scriptpubkey_watches,
				    tal_steal(bcli_plugin->scriptpubkey_watches, watch));

	plugin_log(cmd->plugin, LOG_DBG,
		   "Registered scriptpubkey watch (%zu total)",
		   scriptpubkey_watch_hash_count(bcli_plugin->scriptpubkey_watches));

	return command_finished(cmd, jsonrpc_stream_success(cmd));
}

/* Register a txid to watch for in blocks
 *
 * This handles TWO types of txid watches:
 * 1. Explicit watches: txids we're watching (e.g., counterparty's commitment tx)
 * 2. Broadcast watches: txids we created and broadcast (e.g., our funding tx)
 *
 * From bcli's perspective, both are identical - just "match this txid in blocks".
 * The semantic difference only matters in lightningd for callback routing.
 */
static struct command_result *json_watch_txid(struct command *cmd,
					      const char *buf,
					      const jsmntok_t *toks)
{
	struct bitcoin_txid *txid;
	struct txid_watch *watch;

	if (!param(cmd, buf, toks,
		   p_req("txid", param_txid, &txid),
		   NULL))
		return command_param_failed();

	/* Create new watch */
	watch = tal(bcli_plugin->txid_watches, struct txid_watch);
	watch->txid = *txid;
	txid_watch_hash_add(bcli_plugin->txid_watches, watch);

	plugin_log(cmd->plugin, LOG_DBG,
		   "Registered txid watch %s (%zu total)",
		   fmt_bitcoin_txid(tmpctx, txid),
		   txid_watch_hash_count(bcli_plugin->txid_watches));

	return command_finished(cmd, jsonrpc_stream_success(cmd));
}

/* Register an outpoint to watch for spends in blocks */
static struct command_result *json_watch_outpoint(struct command *cmd,
						  const char *buf,
						  const jsmntok_t *toks)
{
	struct outpoint_watch *watch;
	struct bitcoin_outpoint *outpoint;

	if (!param(cmd, buf, toks,
		   p_req("outpoint", param_outpoint, &outpoint),
		   NULL))
		return command_param_failed();

	/* Create new watch */
	watch = tal(bcli_plugin->outpoint_watches, struct outpoint_watch);
	watch->outpoint = *outpoint;
	outpoint_watch_hash_add(bcli_plugin->outpoint_watches, watch);

	plugin_log(cmd->plugin, LOG_DBG,
		   "Registered outpoint watch %s (%zu total)",
		   fmt_bitcoin_outpoint(tmpctx, outpoint),
		   outpoint_watch_hash_count(bcli_plugin->outpoint_watches));

	return command_finished(cmd, jsonrpc_stream_success(cmd));
}

/* Signal that initial watch synchronization is complete */
static struct command_result *json_watch_sync_complete(struct command *cmd,
						       const char *buf UNUSED,
						       const jsmntok_t *toks UNUSED)
{
	struct json_stream *response;
	size_t scriptpubkey_count, txid_count, outpoint_count;

	if (!param(cmd, buf, toks, NULL))
		return command_param_failed();

	if (bcli_plugin->state != STATE_PROVISIONAL &&
	    bcli_plugin->state != STATE_SYNCING) {
		plugin_err(cmd->plugin,
			   "watch_sync_complete called in wrong state: %s",
			   bcli_state_name(bcli_plugin->state));
	}

	/* Transition to ACTIVE - filtering enabled! */
	bcli_plugin->state = STATE_ACTIVE;

	scriptpubkey_count = scriptpubkey_watch_hash_count(bcli_plugin->scriptpubkey_watches);
	txid_count = txid_watch_hash_count(bcli_plugin->txid_watches);
	outpoint_count = outpoint_watch_hash_count(bcli_plugin->outpoint_watches);

	plugin_log(cmd->plugin, LOG_INFORM,
		   "Watch sync complete! Transitioning to ACTIVE. "
		   "Watching: %zu scriptpubkeys, %zu txids, %zu outpoints",
		   scriptpubkey_count, txid_count, outpoint_count);

	/* Process any buffered blocks now that we have watches */
	process_buffered_blocks(cmd->plugin);

	response = jsonrpc_stream_success(cmd);
	json_add_string(response, "state", bcli_state_name(bcli_plugin->state));
	json_add_num(response, "scriptpubkey_watches", scriptpubkey_count);
	json_add_num(response, "txid_watches", txid_count);
	json_add_num(response, "outpoint_watches", outpoint_count);
	return command_finished(cmd, response);
}

/* ============ Watch Removal RPC Commands ============ */

/* Unregister a scriptpubkey watch */
static struct command_result *json_unwatch_scriptpubkey(struct command *cmd,
							const char *buf,
							const jsmntok_t *toks)
{
	u8 *scriptpubkey;
	struct json_stream *response;

	if (!param(cmd, buf, toks,
		   p_req("scriptpubkey", param_bin_from_hex, &scriptpubkey),
		   NULL))
		return command_param_failed();

	bool removed = remove_scriptpubkey_watch(scriptpubkey);

	plugin_log(cmd->plugin, LOG_DBG,
		   "Unregistered scriptpubkey watch (removed=%d)",
		   removed);

	response = jsonrpc_stream_success(cmd);
	json_add_bool(response, "removed", removed);
	return command_finished(cmd, response);
}

/* Unregister a txid watch */
static struct command_result *json_unwatch_txid(struct command *cmd,
						const char *buf,
						const jsmntok_t *toks)
{
	struct bitcoin_txid *txid;
	struct json_stream *response;

	if (!param(cmd, buf, toks,
		   p_req("txid", param_txid, &txid),
		   NULL))
		return command_param_failed();

	bool removed = remove_txid_watch(txid);

	plugin_log(cmd->plugin, LOG_DBG,
		   "Unregistered txid watch %s (removed=%d)",
		   fmt_bitcoin_txid(tmpctx, txid), removed);

	response = jsonrpc_stream_success(cmd);
	json_add_bool(response, "removed", removed);
	return command_finished(cmd, response);
}

/* Unregister an outpoint watch */
static struct command_result *json_unwatch_outpoint(struct command *cmd,
						    const char *buf,
						    const jsmntok_t *toks)
{
	struct bitcoin_outpoint *outpoint;
	struct json_stream *response;

	if (!param(cmd, buf, toks,
		   p_req("outpoint", param_outpoint, &outpoint),
		   NULL))
		return command_param_failed();

	bool removed = remove_outpoint_watch(outpoint);

	plugin_log(cmd->plugin, LOG_DBG,
		   "Unregistered outpoint watch %s (removed=%d)",
		   fmt_bitcoin_outpoint(tmpctx, outpoint), removed);

	response = jsonrpc_stream_success(cmd);
	json_add_bool(response, "removed", removed);
	return command_finished(cmd, response);
}

/* Get infos about the block chain.
 * Calls `getblockchaininfo` and returns headers count, blocks count,
 * the chain id, and whether this is initialblockdownload.
 */
static struct command_result *getchaininfo(struct command *cmd,
                                           const char *buf UNUSED,
                                           const jsmntok_t *toks UNUSED)
{
	/* FIXME(vincenzopalazzo): Inside the JSON request,
         * we have the current height known from Core Lightning. Therefore,
         * we can attempt to prevent a crash if the 'getchaininfo' function returns
         * a lower height than the one we already know, by waiting for a short period.
         * However, I currently don't have a better idea on how to handle this situation. */
	u32 *height UNUSED;
	if (!param(cmd, buf, toks,
		   p_opt("last_height", param_number, &height),
		   NULL))
		return command_param_failed();

	start_bitcoin_cli(NULL, cmd, process_getblockchaininfo, false,
			  BITCOIND_HIGH_PRIO, NULL,
			  "getblockchaininfo", NULL);

	return command_still_pending(cmd);
}

/* ============ Block polling implementation ============ */

/* Forward declaration - needed for circular dependency */
static struct command_result *poll_for_new_blocks(struct command *cmd, void *plugin_ptr UNUSED);

struct getblock_stash {
	/* Pending block change (not yet notified) */
	u32 pending_height;
	struct bitcoin_blkid pending_blkid;
};

/* Process getblock result and send notification with full block info */
static struct command_result *process_getblock_for_notification(struct bitcoin_cli *bcli)
{
	const struct getblock_stash *gbs = bcli->stash;

	plugin_log(bcli->cmd->plugin, LOG_DBG, "process_getblock_for_notification called");

	/* Only send notification if we successfully got the block */
	if (bcli->exitstatus && *bcli->exitstatus != 0) {
		plugin_log(bcli->cmd->plugin, LOG_BROKEN,
			   "Failed to get raw block %s, skipping notification",
			   fmt_bitcoin_blkid(tmpctx, &gbs->pending_blkid));
	} else {
		size_t useful_len = length_no_trailing(bcli->output, bcli->output_bytes);

		plugin_log(bcli->cmd->plugin, LOG_DBG,
			   "Processing new block: height=%u hash=%s state=%s",
			   gbs->pending_height,
			   fmt_bitcoin_blkid(tmpctx, &gbs->pending_blkid),
			   bcli_state_name(bcli_plugin->state));

		/* Handle based on current state */
		switch (bcli_plugin->state) {
		case STATE_INITIALIZING:
			/* Shouldn't happen, but treat like PROVISIONAL */
			plugin_log(bcli->cmd->plugin, LOG_UNUSUAL,
				   "Received block in INITIALIZING state, buffering");
			/* Fall through */
		case STATE_PROVISIONAL:
		case STATE_SYNCING:
			/* Buffer the block for later processing */
			plugin_log(bcli->cmd->plugin, LOG_DBG,
				   "Buffering block %u (state=%s)",
				   gbs->pending_height,
				   bcli_state_name(bcli_plugin->state));
			buffer_block(gbs->pending_height,
				     &gbs->pending_blkid,
				     bcli->output,
				     useful_len);
			break;

		case STATE_ACTIVE:
			/* Filter and send only matching transactions */
			plugin_log(bcli->cmd->plugin, LOG_DBG,
				   "Filtering block %u", gbs->pending_height);
			filter_and_send_block(bcli->cmd->plugin,
					      gbs->pending_height,
					      &gbs->pending_blkid,
					      bcli->output,
					      useful_len);
			break;
		}

		/* Update our tracking state only after successful processing */
		bitcoind->last_height = gbs->pending_height;
		bitcoind->last_blkid = gbs->pending_blkid;

		plugin_log(bcli->cmd->plugin, LOG_DBG,
			   "Block processed and state updated");
	}

	/* Always reschedule the next poll */
	bitcoind->poll_timer = global_timer(bcli->cmd->plugin,
					    time_from_sec(bitcoind->poll_interval),
					    poll_for_new_blocks, bcli->cmd->plugin);

	return timer_complete(bcli->cmd);
}

/* Process getblockchaininfo result during polling */
static struct command_result *process_poll_chaintip(struct bitcoin_cli *bcli)
{
	struct command *cmd = bcli->cmd;
	const jsmntok_t *tokens;
	const char *err;
	u32 height;
	struct bitcoin_blkid blockhash;

	/* Handle error cases first */
	tokens = json_parse_simple(bcli, bcli->output, bcli->output_bytes);
	if (!tokens)
		plugin_err(cmd->plugin, "getblockchaininfo gave bad response '%.*s'",
			   (int)bcli->output_bytes,
			   bcli->output);
	err = json_scan(tmpctx, bcli->output, tokens,
			"{blocks:%,bestblockhash:%}",
			JSON_SCAN(json_to_number, &height),
			JSON_SCAN(json_to_bitcoin_blkid, &blockhash));
	if (err)
		plugin_err(cmd->plugin, "getblockchaininfo gave bad response '%.*s': %s",
			   (int)bcli->output_bytes,
			   bcli->output, err);

	if (bitcoind->last_height == 0) {
		/* First poll - initialize state */
		plugin_log(cmd->plugin, LOG_DBG,
			   "Initial blockchain state: height=%u", height);
		bitcoind->last_height = height;
		bitcoind->last_blkid = blockhash;
	} else if (height != bitcoind->last_height ||
		   !bitcoin_blkid_eq(&blockhash, &bitcoind->last_blkid)) {
		struct getblock_stash *gbs;
		/* Tip changed - store pending values and fetch raw block */
		plugin_log(cmd->plugin, LOG_DBG,
			   "Block change: %u -> %u",
			   bitcoind->last_height, height);
		gbs = tal(cmd, struct getblock_stash);
		/* Store pending values - will be committed after successful getblock */
		gbs->pending_height = height;
		gbs->pending_blkid = blockhash;

		/* Fetch the raw block and send notification */
		start_bitcoin_cli(NULL, cmd, process_getblock_for_notification, true,
				  BITCOIND_LOW_PRIO, gbs, "getblock",
				  blockhash, "0", NULL);

		return command_still_pending(cmd);
	}

	/* Always reschedule the next poll */
	bitcoind->poll_timer = global_timer(cmd->plugin,
					    time_from_sec(bitcoind->poll_interval),
					    poll_for_new_blocks, cmd->plugin);

	/* Tell framework we're done - this will free cmd */
	return timer_complete(cmd);
}

/* Poll timer callback - starts the block check */
static struct command_result *poll_for_new_blocks(struct command *cmd, void *plugin_ptr UNUSED)
{
	/* Get current blockchain state (height + hash) in one RPC call.
	 * The framework created cmd for us when the timer fired.
	 * We use LOW priority so this background polling doesn't block
	 * urgent operations. */
	start_bitcoin_cli(NULL, cmd, process_poll_chaintip, false,
			  BITCOIND_LOW_PRIO, NULL, "getblockchaininfo", NULL);

	/* Tell framework to keep cmd alive until bitcoin-cli responds */
	return command_still_pending(cmd);
}

/* Mutual recursion. */
static struct command_result *estimatefees_done(struct bitcoin_cli *bcli);

/* Add a feerate, but don't publish one that bitcoind won't accept. */
static void json_add_feerate(struct json_stream *result, const char *fieldname,
			     struct command *cmd,
			     const struct estimatefees_stash *stash,
			     uint64_t value)
{
	/* Anthony Towns reported signet had a 900kbtc fee block, and then
	 * CLN got upset scanning feerate.  It expects a u32. */
	if (value > 0xFFFFFFFF) {
		plugin_log(cmd->plugin, LOG_UNUSUAL,
			   "Feerate %"PRIu64" is ridiculous: trimming to 32 bites",
			   value);
		value = 0xFFFFFFFF;
	}
	/* 0 is special, it means "unknown" */
	if (value && value < stash->perkb_floor) {
		plugin_log(cmd->plugin, LOG_DBG,
			   "Feerate %s raised from %"PRIu64
			   " perkb to floor of %"PRIu64,
			   fieldname, value, stash->perkb_floor);
		json_add_u64(result, fieldname, stash->perkb_floor);
	} else {
		json_add_u64(result, fieldname, value);
	}
}

static struct command_result *estimatefees_next(struct command *cmd,
						struct estimatefees_stash *stash)
{
	struct json_stream *response;

	if (stash->cursor < ARRAY_SIZE(stash->perkb)) {
		start_bitcoin_cli(NULL, cmd, estimatefees_done, true,
				  BITCOIND_LOW_PRIO, stash,
				  "estimatesmartfee",
				  take(tal_fmt(NULL, "%u",
					       estimatefee_params[stash->cursor].blocks)),
				  estimatefee_params[stash->cursor].style,
				  NULL);

		return command_still_pending(cmd);
	}

	response = jsonrpc_stream_success(cmd);
	/* Present an ordered array of block deadlines, and a floor. */
	json_array_start(response, "feerates");
	for (size_t i = 0; i < ARRAY_SIZE(stash->perkb); i++) {
		if (!stash->perkb[i])
			continue;
		json_object_start(response, NULL);
		json_add_u32(response, "blocks", estimatefee_params[i].blocks);
		json_add_feerate(response, "feerate", cmd, stash, stash->perkb[i]);
		json_object_end(response);
	}
	json_array_end(response);
	json_add_u64(response, "feerate_floor", stash->perkb_floor);
	return command_finished(cmd, response);
}

static struct command_result *getminfees_done(struct bitcoin_cli *bcli)
{
	const jsmntok_t *tokens;
	const char *err;
	u64 mempoolfee, relayfee;
	struct estimatefees_stash *stash = bcli->stash;

	if (*bcli->exitstatus != 0)
		return estimatefees_null_response(bcli);

	tokens = json_parse_simple(bcli->output,
				   bcli->output, bcli->output_bytes);
	if (!tokens)
		return command_err_bcli_badjson(bcli,
						"cannot parse getmempoolinfo");

	/* Look at minrelaytxfee they configured, and current min fee to get
	 * into mempool. */
	err = json_scan(tmpctx, bcli->output, tokens,
			"{mempoolminfee:%,minrelaytxfee:%}",
			JSON_SCAN(json_to_bitcoin_amount, &mempoolfee),
			JSON_SCAN(json_to_bitcoin_amount, &relayfee));
	if (err)
		return command_err_bcli_badjson(bcli, err);

	stash->perkb_floor = max_u64(mempoolfee, relayfee);
	stash->cursor = 0;
	return estimatefees_next(bcli->cmd, stash);
}

/* Get the current feerates. We use an urgent feerate for unilateral_close and max,
 * a slightly less urgent feerate for htlc_resolution and penalty transactions,
 * a slow feerate for min, and a normal one for all others.
 */
static struct command_result *estimatefees(struct command *cmd,
					   const char *buf UNUSED,
					   const jsmntok_t *toks UNUSED)
{
	struct estimatefees_stash *stash = tal(cmd, struct estimatefees_stash);

	if (!param(cmd, buf, toks, NULL))
		return command_param_failed();

	start_bitcoin_cli(NULL, cmd, getminfees_done, true,
			  BITCOIND_LOW_PRIO, stash,
			  "getmempoolinfo",
			  NULL);
	return command_still_pending(cmd);
}

static struct command_result *estimatefees_done(struct bitcoin_cli *bcli)
{
	struct command_result *err;
	struct estimatefees_stash *stash = bcli->stash;

	/* If we cannot estimate fees, no need to continue bothering bitcoind. */
	if (*bcli->exitstatus != 0)
		return estimatefees_null_response(bcli);

	err = estimatefees_parse_feerate(bcli, &stash->perkb[stash->cursor]);
	if (err)
		return err;

	stash->cursor++;
	return estimatefees_next(bcli->cmd, stash);
}

/* Send a transaction to the Bitcoin network.
 * Calls `sendrawtransaction` using the first parameter as the raw tx.
 */
static struct command_result *sendrawtransaction(struct command *cmd,
                                                 const char *buf,
                                                 const jsmntok_t *toks)
{
	const char *tx, *highfeesarg;
	bool *allowhighfees;

	/* bitcoin-cli wants strings. */
	if (!param(cmd, buf, toks,
	           p_req("tx", param_string, &tx),
		   p_req("allowhighfees", param_bool, &allowhighfees),
	           NULL))
		return command_param_failed();

	if (*allowhighfees) {
			highfeesarg = "0";
	} else
		highfeesarg = NULL;

	start_bitcoin_cli(NULL, cmd, process_sendrawtransaction, true,
			  BITCOIND_HIGH_PRIO, NULL,
			  "sendrawtransaction",
			  tx, highfeesarg, NULL);

	return command_still_pending(cmd);
}

static struct command_result *getutxout(struct command *cmd,
                                       const char *buf,
                                       const jsmntok_t *toks)
{
	const char *txid, *vout;

	/* bitcoin-cli wants strings. */
	if (!param(cmd, buf, toks,
	           p_req("txid", param_string, &txid),
	           p_req("vout", param_string, &vout),
	           NULL))
		return command_param_failed();

	start_bitcoin_cli(NULL, cmd, process_getutxout, true,
			  BITCOIND_HIGH_PRIO, NULL,
			  "gettxout", txid, vout, NULL);

	return command_still_pending(cmd);
}

static void bitcoind_failure(struct plugin *p, const char *error_message)
{
	const char **cmd = gather_args(bitcoind, "echo", NULL);
	plugin_err(p, "\n%s\n\n"
		      "Make sure you have bitcoind running and that bitcoin-cli"
		      " is able to connect to bitcoind.\n\n"
		      "You can verify that your Bitcoin Core installation is"
		      " ready for use by running:\n\n"
		      "    $ %s 'hello world'\n", error_message,
		      args_string(cmd, cmd));
}

/* Do some sanity checks on bitcoind based on the output of `getnetworkinfo`. */
static void parse_getnetworkinfo_result(struct plugin *p, const char *buf)
{
	const jsmntok_t *result;
	bool tx_relay;
	u32 min_version = 220000;
	const char *err;

	result = json_parse_simple(NULL, buf, strlen(buf));
	if (!result)
		plugin_err(p, "Invalid response to '%s': '%s'. Can not "
			      "continue without proceeding to sanity checks.",
			   args_string(tmpctx, gather_args(bitcoind, "getnetworkinfo", NULL)),
			   buf);

	/* Check that we have a fully-featured `estimatesmartfee`. */
	err = json_scan(tmpctx, buf, result, "{version:%,localrelay:%}",
			JSON_SCAN(json_to_u32, &bitcoind->version),
			JSON_SCAN(json_to_bool, &tx_relay));
	if (err)
		plugin_err(p, "%s.  Got '%.*s'. Can not"
			   " continue without proceeding to sanity checks.",
			   err,
			   json_tok_full_len(result), json_tok_full(buf, result));

	if (bitcoind->version < min_version)
		plugin_err(p, "Unsupported bitcoind version %"PRIu32", at least"
			      " %"PRIu32" required.", bitcoind->version, min_version);

	/* We don't support 'blocksonly', as we rely on transaction relay for fee
	 * estimates. */
	if (!tx_relay)
		plugin_err(p, "The 'blocksonly' mode of bitcoind, or any option "
			      "deactivating transaction relay is not supported.");

	tal_free(result);
}

static void wait_and_check_bitcoind(struct plugin *p)
{
	int in, from, status;
	pid_t child;
	const char **cmd = gather_args(
	    bitcoind, "-rpcwait", "-rpcwaittimeout=30", "getnetworkinfo", NULL);
	char *output = NULL;

	child = pipecmdarr(&in, &from, &from, cast_const2(char **, cmd));

	if (bitcoind->rpcpass)
		write_all(in, bitcoind->rpcpass, strlen(bitcoind->rpcpass));

	close(in);

	if (child < 0) {
		if (errno == ENOENT)
			bitcoind_failure(
			    p,
			    "bitcoin-cli not found. Is bitcoin-cli "
			    "(part of Bitcoin Core) available in your PATH?");
		plugin_err(p, "%s exec failed: %s", cmd[0], strerror(errno));
	}

	output = grab_fd_str(cmd, from);

	waitpid(child, &status, 0);

	if (!WIFEXITED(status))
		bitcoind_failure(p, tal_fmt(bitcoind, "Death of %s: signal %i",
					    cmd[0], WTERMSIG(status)));

	if (WEXITSTATUS(status) != 0) {
		if (WEXITSTATUS(status) == 1)
			bitcoind_failure(p,
					 "RPC connection timed out. Could "
					 "not connect to bitcoind using "
					 "bitcoin-cli. Is bitcoind running?");
		bitcoind_failure(p,
				 tal_fmt(bitcoind, "%s exited with code %i: %s",
					 cmd[0], WEXITSTATUS(status), output));
	}

	parse_getnetworkinfo_result(p, output);

	tal_free(cmd);
}

static void memleak_mark_bitcoind(struct plugin *p, struct htable *memtable)
{
	memleak_scan_obj(memtable, bcli_plugin);
}

static const char *init(struct command *init_cmd, const char *buffer UNUSED,
			const jsmntok_t *config UNUSED)
{
	plugin_log(init_cmd->plugin, LOG_DBG, "BCLI: init() called");
	wait_and_check_bitcoind(init_cmd->plugin);
	plugin_log(init_cmd->plugin, LOG_DBG, "BCLI: wait_and_check_bitcoind completed");

	/* Usually we fake up fees in regtest */
	if (streq(chainparams->network_name, "regtest"))
		bitcoind->fake_fees = !bitcoind->dev_no_fake_fees;
	else
		bitcoind->fake_fees = false;

	plugin_set_memleak_handler(init_cmd->plugin, memleak_mark_bitcoind);
	plugin_log(init_cmd->plugin, LOG_INFORM,
		   "bitcoin-cli initialized and connected to bitcoind.");

	/* Create the plugin state wrapper with proper tal parent */
	bcli_plugin = new_bcli_plugin(init_cmd->plugin, bitcoind);
	plugin_log(init_cmd->plugin, LOG_DBG, "BCLI: Created plugin state in %s",
		   bcli_state_name(bcli_plugin->state));

	/* Transition to PROVISIONAL state - ready to buffer blocks */
	bcli_plugin->state = STATE_PROVISIONAL;
	plugin_log(init_cmd->plugin, LOG_INFORM,
		   "BCLI: Transitioned to %s - will buffer blocks until watches arrive",
		   bcli_state_name(bcli_plugin->state));

	/* Start the poll timer to check for new blocks */
	plugin_log(init_cmd->plugin, LOG_DBG, "BCLI: Starting poll timer");
	bitcoind->poll_timer = global_timer(init_cmd->plugin,
					    time_from_sec(bitcoind->poll_interval),
					    poll_for_new_blocks,
					    init_cmd->plugin);
	plugin_log(init_cmd->plugin, LOG_DBG, "BCLI: Poll timer started, returning from init");

	return NULL;
}

static const struct plugin_command commands[] = {
	{
		"getrawblockbyheight",
		getrawblockbyheight
	},
	{
		"getchaininfo",
		getchaininfo
	},
	{
		"estimatefees",
		estimatefees
	},
	{
		"sendrawtransaction",
		sendrawtransaction
	},
	{
		"getutxout",
		getutxout
	},
	/* Watch management commands */
	{
		"watch_scriptpubkey",
		json_watch_scriptpubkey
	},
	{
		"watch_txid",
		json_watch_txid
	},
	{
		"watch_outpoint",
		json_watch_outpoint
	},
	{
		"watch_sync_complete",
		json_watch_sync_complete
	},
	/* Watch removal commands */
	{
		"unwatch_scriptpubkey",
		json_unwatch_scriptpubkey
	},
	{
		"unwatch_txid",
		json_unwatch_txid
	},
	{
		"unwatch_outpoint",
		json_unwatch_outpoint
	},
};

/* Notification topics we publish */
static const char *notification_topics[] = {
	"bcli_block_detected",
};

int main(int argc, char *argv[])
{
	setup_locale();

	/* Initialize our global context object here to handle startup options. */
	bitcoind = new_bitcoind(NULL);

	plugin_main(argv, init, NULL, PLUGIN_STATIC, false /* Do not init RPC on startup*/,
		    NULL, commands, ARRAY_SIZE(commands),
		    NULL, 0, NULL, 0,
		    notification_topics, ARRAY_SIZE(notification_topics),
		    plugin_option("bitcoin-datadir",
				  "string",
				  "-datadir arg for bitcoin-cli",
				  charp_option, NULL, &bitcoind->datadir),
		    plugin_option("bitcoin-cli",
				  "string",
				  "bitcoin-cli pathname",
				  charp_option, NULL, &bitcoind->cli),
		    plugin_option("bitcoin-rpcuser",
				  "string",
				  "bitcoind RPC username",
				  charp_option, NULL, &bitcoind->rpcuser),
		    plugin_option("bitcoin-rpcpassword",
				  "string",
				  "bitcoind RPC password",
				  charp_option, NULL, &bitcoind->rpcpass),
		    plugin_option("bitcoin-rpcconnect",
				  "string",
				  "bitcoind RPC host to connect to",
				  charp_option, NULL, &bitcoind->rpcconnect),
		    plugin_option("bitcoin-rpcport",
				  "int",
				  "bitcoind RPC host's port",
				  charp_option, NULL, &bitcoind->rpcport),
		    plugin_option("bitcoin-rpcclienttimeout",
				  "int",
				  "bitcoind RPC timeout in seconds during HTTP requests",
				  u64_option, u64_jsonfmt, &bitcoind->rpcclienttimeout),
		    plugin_option("bitcoin-retry-timeout",
				  "int",
				  "how long to keep retrying to contact bitcoind"
				  " before fatally exiting",
				  u64_option, u64_jsonfmt, &bitcoind->retry_timeout),
		    plugin_option("bitcoin-poll-interval",
				  "int",
				  "Seconds between blockchain polls (default 10)",
				  u64_option, u64_jsonfmt, &bitcoind->poll_interval),
		    plugin_option_dev("dev-no-fake-fees",
				      "bool",
				      "Suppress fee faking for regtest",
				      bool_option, NULL, &bitcoind->dev_no_fake_fees),
		    NULL);
}
