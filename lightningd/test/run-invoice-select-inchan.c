#include "../channel.c"
#include "../invoice.c"
#include "../peer_control.c"
#include <ccan/alignof/alignof.h>

bool deprecated_apis = false;

/* AUTOGENERATED MOCKS START */
/* Generated stub for bip32_pubkey */
bool bip32_pubkey(const struct ext_key *bip32_base UNNEEDED,
		  struct pubkey *pubkey UNNEEDED, u32 index UNNEEDED)
{ fprintf(stderr, "bip32_pubkey called!\n"); abort(); }
/* Generated stub for bitcoind_gettxout */
void bitcoind_gettxout(struct bitcoind *bitcoind UNNEEDED,
		       const struct bitcoin_txid *txid UNNEEDED, const u32 outnum UNNEEDED,
		       void (*cb)(struct bitcoind *bitcoind UNNEEDED,
				  const struct bitcoin_tx_output *txout UNNEEDED,
				  void *arg) UNNEEDED,
		       void *arg UNNEEDED)
{ fprintf(stderr, "bitcoind_gettxout called!\n"); abort(); }
/* Generated stub for bolt11_decode */
struct bolt11 *bolt11_decode(const tal_t *ctx UNNEEDED, const char *str UNNEEDED,
			     const char *description UNNEEDED, char **fail UNNEEDED)
{ fprintf(stderr, "bolt11_decode called!\n"); abort(); }
/* Generated stub for bolt11_encode_ */
char *bolt11_encode_(const tal_t *ctx UNNEEDED,
		     const struct bolt11 *b11 UNNEEDED, bool n_field UNNEEDED,
		     bool (*sign)(const u5 *u5bytes UNNEEDED,
				  const u8 *hrpu8 UNNEEDED,
				  secp256k1_ecdsa_recoverable_signature *rsig UNNEEDED,
				  void *arg) UNNEEDED,
		     void *arg UNNEEDED)
{ fprintf(stderr, "bolt11_encode_ called!\n"); abort(); }
/* Generated stub for bolt11_new */
struct bolt11 *bolt11_new(const tal_t *ctx UNNEEDED, u64 *msatoshi UNNEEDED)
{ fprintf(stderr, "bolt11_new called!\n"); abort(); }
/* Generated stub for broadcast_tx */
void broadcast_tx(struct chain_topology *topo UNNEEDED,
		  struct channel *channel UNNEEDED, const struct bitcoin_tx *tx UNNEEDED,
		  void (*failed)(struct channel *channel UNNEEDED,
				 int exitstatus UNNEEDED,
				 const char *err))
{ fprintf(stderr, "broadcast_tx called!\n"); abort(); }
/* Generated stub for channel_tell_funding_locked */
bool channel_tell_funding_locked(struct lightningd *ld UNNEEDED,
				 struct channel *channel UNNEEDED,
				 const struct bitcoin_txid *txid UNNEEDED,
				 u32 depth UNNEEDED)
{ fprintf(stderr, "channel_tell_funding_locked called!\n"); abort(); }
/* Generated stub for command_fail */
void  command_fail(struct command *cmd UNNEEDED, int code UNNEEDED,
				   const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "command_fail called!\n"); abort(); }
/* Generated stub for command_failed */
void command_failed(struct command *cmd UNNEEDED, struct json_stream *result UNNEEDED)
{ fprintf(stderr, "command_failed called!\n"); abort(); }
/* Generated stub for command_still_pending */
void command_still_pending(struct command *cmd UNNEEDED)
{ fprintf(stderr, "command_still_pending called!\n"); abort(); }
/* Generated stub for command_success */
void command_success(struct command *cmd UNNEEDED, struct json_stream *response UNNEEDED)
{ fprintf(stderr, "command_success called!\n"); abort(); }
/* Generated stub for connect_succeeded */
void connect_succeeded(struct lightningd *ld UNNEEDED, const struct pubkey *id UNNEEDED)
{ fprintf(stderr, "connect_succeeded called!\n"); abort(); }
/* Generated stub for delay_then_reconnect */
void delay_then_reconnect(struct channel *channel UNNEEDED, u32 seconds_delay UNNEEDED,
			  const struct wireaddr_internal *addrhint TAKES UNNEEDED)
{ fprintf(stderr, "delay_then_reconnect called!\n"); abort(); }
/* Generated stub for fatal */
void   fatal(const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "fatal called!\n"); abort(); }
/* Generated stub for fromwire_connect_peer_connected */
bool fromwire_connect_peer_connected(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, struct pubkey *id UNNEEDED, struct wireaddr_internal *addr UNNEEDED, struct crypto_state *crypto_state UNNEEDED, u8 **globalfeatures UNNEEDED, u8 **localfeatures UNNEEDED)
{ fprintf(stderr, "fromwire_connect_peer_connected called!\n"); abort(); }
/* Generated stub for fromwire_gossip_get_incoming_channels_reply */
bool fromwire_gossip_get_incoming_channels_reply(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, struct route_info **route_info UNNEEDED)
{ fprintf(stderr, "fromwire_gossip_get_incoming_channels_reply called!\n"); abort(); }
/* Generated stub for fromwire_hsm_get_channel_basepoints_reply */
bool fromwire_hsm_get_channel_basepoints_reply(const void *p UNNEEDED, struct basepoints *basepoints UNNEEDED, struct pubkey *funding_pubkey UNNEEDED)
{ fprintf(stderr, "fromwire_hsm_get_channel_basepoints_reply called!\n"); abort(); }
/* Generated stub for fromwire_hsm_sign_commitment_tx_reply */
bool fromwire_hsm_sign_commitment_tx_reply(const void *p UNNEEDED, secp256k1_ecdsa_signature *sig UNNEEDED)
{ fprintf(stderr, "fromwire_hsm_sign_commitment_tx_reply called!\n"); abort(); }
/* Generated stub for fromwire_hsm_sign_invoice_reply */
bool fromwire_hsm_sign_invoice_reply(const void *p UNNEEDED, secp256k1_ecdsa_recoverable_signature *sig UNNEEDED)
{ fprintf(stderr, "fromwire_hsm_sign_invoice_reply called!\n"); abort(); }
/* Generated stub for get_chainparams */
const struct chainparams *get_chainparams(const struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "get_chainparams called!\n"); abort(); }
/* Generated stub for get_log_level */
enum log_level get_log_level(struct log_book *lr UNNEEDED)
{ fprintf(stderr, "get_log_level called!\n"); abort(); }
/* Generated stub for htlcs_reconnect */
void htlcs_reconnect(struct lightningd *ld UNNEEDED,
		     struct htlc_in_map *htlcs_in UNNEEDED,
		     struct htlc_out_map *htlcs_out UNNEEDED)
{ fprintf(stderr, "htlcs_reconnect called!\n"); abort(); }
/* Generated stub for json_add_bool */
void json_add_bool(struct json_stream *result UNNEEDED, const char *fieldname UNNEEDED,
		   bool value UNNEEDED)
{ fprintf(stderr, "json_add_bool called!\n"); abort(); }
/* Generated stub for json_add_escaped_string */
void json_add_escaped_string(struct json_stream *result UNNEEDED,
			     const char *fieldname UNNEEDED,
			     const struct json_escaped *esc TAKES UNNEEDED)
{ fprintf(stderr, "json_add_escaped_string called!\n"); abort(); }
/* Generated stub for json_add_hex */
void json_add_hex(struct json_stream *result UNNEEDED, const char *fieldname UNNEEDED,
		  const void *data UNNEEDED, size_t len UNNEEDED)
{ fprintf(stderr, "json_add_hex called!\n"); abort(); }
/* Generated stub for json_add_hex_talarr */
void json_add_hex_talarr(struct json_stream *result UNNEEDED,
			 const char *fieldname UNNEEDED,
			 const tal_t *data UNNEEDED)
{ fprintf(stderr, "json_add_hex_talarr called!\n"); abort(); }
/* Generated stub for json_add_log */
void json_add_log(struct json_stream *result UNNEEDED,
		  const struct log_book *lr UNNEEDED, enum log_level minlevel UNNEEDED)
{ fprintf(stderr, "json_add_log called!\n"); abort(); }
/* Generated stub for json_add_num */
void json_add_num(struct json_stream *result UNNEEDED, const char *fieldname UNNEEDED,
		  unsigned int value UNNEEDED)
{ fprintf(stderr, "json_add_num called!\n"); abort(); }
/* Generated stub for json_add_pubkey */
void json_add_pubkey(struct json_stream *response UNNEEDED,
		     const char *fieldname UNNEEDED,
		     const struct pubkey *key UNNEEDED)
{ fprintf(stderr, "json_add_pubkey called!\n"); abort(); }
/* Generated stub for json_add_short_channel_id */
void json_add_short_channel_id(struct json_stream *response UNNEEDED,
			       const char *fieldname UNNEEDED,
			       const struct short_channel_id *id UNNEEDED)
{ fprintf(stderr, "json_add_short_channel_id called!\n"); abort(); }
/* Generated stub for json_add_string */
void json_add_string(struct json_stream *result UNNEEDED, const char *fieldname UNNEEDED, const char *value UNNEEDED)
{ fprintf(stderr, "json_add_string called!\n"); abort(); }
/* Generated stub for json_add_txid */
void json_add_txid(struct json_stream *result UNNEEDED, const char *fieldname UNNEEDED,
		   const struct bitcoin_txid *txid UNNEEDED)
{ fprintf(stderr, "json_add_txid called!\n"); abort(); }
/* Generated stub for json_add_u64 */
void json_add_u64(struct json_stream *result UNNEEDED, const char *fieldname UNNEEDED,
		  uint64_t value UNNEEDED)
{ fprintf(stderr, "json_add_u64 called!\n"); abort(); }
/* Generated stub for json_add_uncommitted_channel */
void json_add_uncommitted_channel(struct json_stream *response UNNEEDED,
				  const struct uncommitted_channel *uc UNNEEDED)
{ fprintf(stderr, "json_add_uncommitted_channel called!\n"); abort(); }
/* Generated stub for json_array_end */
void json_array_end(struct json_stream *ptr UNNEEDED)
{ fprintf(stderr, "json_array_end called!\n"); abort(); }
/* Generated stub for json_array_start */
void json_array_start(struct json_stream *ptr UNNEEDED, const char *fieldname UNNEEDED)
{ fprintf(stderr, "json_array_start called!\n"); abort(); }
/* Generated stub for json_escape */
struct json_escaped *json_escape(const tal_t *ctx UNNEEDED, const char *str TAKES UNNEEDED)
{ fprintf(stderr, "json_escape called!\n"); abort(); }
/* Generated stub for json_object_end */
void json_object_end(struct json_stream *ptr UNNEEDED)
{ fprintf(stderr, "json_object_end called!\n"); abort(); }
/* Generated stub for json_object_start */
void json_object_start(struct json_stream *ptr UNNEEDED, const char *fieldname UNNEEDED)
{ fprintf(stderr, "json_object_start called!\n"); abort(); }
/* Generated stub for json_stream_fail */
struct json_stream *json_stream_fail(struct command *cmd UNNEEDED,
				     int code UNNEEDED,
				     const char *errmsg UNNEEDED)
{ fprintf(stderr, "json_stream_fail called!\n"); abort(); }
/* Generated stub for json_stream_success */
struct json_stream *json_stream_success(struct command *cmd UNNEEDED)
{ fprintf(stderr, "json_stream_success called!\n"); abort(); }
/* Generated stub for json_tok_address_scriptpubkey */
enum address_parse_result json_tok_address_scriptpubkey(const tal_t *ctx UNNEEDED,
			      const struct chainparams *chainparams UNNEEDED,
			      const char *buffer UNNEEDED,
			      const jsmntok_t *tok UNNEEDED, const u8 **scriptpubkey UNNEEDED)
{ fprintf(stderr, "json_tok_address_scriptpubkey called!\n"); abort(); }
/* Generated stub for json_tok_array */
bool json_tok_array(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		    const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		    const jsmntok_t **arr UNNEEDED)
{ fprintf(stderr, "json_tok_array called!\n"); abort(); }
/* Generated stub for json_tok_bool */
bool json_tok_bool(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		   const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		   bool **b UNNEEDED)
{ fprintf(stderr, "json_tok_bool called!\n"); abort(); }
/* Generated stub for json_tok_channel_id */
bool json_tok_channel_id(const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
			 struct channel_id *cid UNNEEDED)
{ fprintf(stderr, "json_tok_channel_id called!\n"); abort(); }
/* Generated stub for json_tok_escaped_string */
bool json_tok_escaped_string(struct command *cmd UNNEEDED, const char *name UNNEEDED,
			     const char * buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
			     const char **str UNNEEDED)
{ fprintf(stderr, "json_tok_escaped_string called!\n"); abort(); }
/* Generated stub for json_tok_label */
bool json_tok_label(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		    const char * buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		    struct json_escaped **label UNNEEDED)
{ fprintf(stderr, "json_tok_label called!\n"); abort(); }
/* Generated stub for json_tok_loglevel */
bool json_tok_loglevel(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		       const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		       enum log_level **level UNNEEDED)
{ fprintf(stderr, "json_tok_loglevel called!\n"); abort(); }
/* Generated stub for json_tok_msat */
bool json_tok_msat(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		   const char *buffer UNNEEDED, const jsmntok_t * tok UNNEEDED,
		   u64 **msatoshi_val UNNEEDED)
{ fprintf(stderr, "json_tok_msat called!\n"); abort(); }
/* Generated stub for json_tok_number */
bool json_tok_number(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		     const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		     unsigned int **num UNNEEDED)
{ fprintf(stderr, "json_tok_number called!\n"); abort(); }
/* Generated stub for json_tok_pubkey */
bool json_tok_pubkey(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		     const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		     struct pubkey **pubkey UNNEEDED)
{ fprintf(stderr, "json_tok_pubkey called!\n"); abort(); }
/* Generated stub for json_tok_short_channel_id */
bool json_tok_short_channel_id(struct command *cmd UNNEEDED, const char *name UNNEEDED,
			       const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
			       struct short_channel_id **scid UNNEEDED)
{ fprintf(stderr, "json_tok_short_channel_id called!\n"); abort(); }
/* Generated stub for json_tok_string */
bool json_tok_string(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		     const char * buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		     const char **str UNNEEDED)
{ fprintf(stderr, "json_tok_string called!\n"); abort(); }
/* Generated stub for json_tok_tok */
bool json_tok_tok(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		  const char *buffer UNNEEDED, const jsmntok_t * tok UNNEEDED,
		  const jsmntok_t **out UNNEEDED)
{ fprintf(stderr, "json_tok_tok called!\n"); abort(); }
/* Generated stub for json_tok_u64 */
bool json_tok_u64(struct command *cmd UNNEEDED, const char *name UNNEEDED,
		  const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		  uint64_t **num UNNEEDED)
{ fprintf(stderr, "json_tok_u64 called!\n"); abort(); }
/* Generated stub for json_to_pubkey */
bool json_to_pubkey(const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
		    struct pubkey *pubkey UNNEEDED)
{ fprintf(stderr, "json_to_pubkey called!\n"); abort(); }
/* Generated stub for json_to_short_channel_id */
bool json_to_short_channel_id(const char *buffer UNNEEDED, const jsmntok_t *tok UNNEEDED,
			      struct short_channel_id *scid UNNEEDED)
{ fprintf(stderr, "json_to_short_channel_id called!\n"); abort(); }
/* Generated stub for kill_uncommitted_channel */
void kill_uncommitted_channel(struct uncommitted_channel *uc UNNEEDED,
			      const char *why UNNEEDED)
{ fprintf(stderr, "kill_uncommitted_channel called!\n"); abort(); }
/* Generated stub for log_ */
void log_(struct log *log UNNEEDED, enum log_level level UNNEEDED, const char *fmt UNNEEDED, ...)

{ fprintf(stderr, "log_ called!\n"); abort(); }
/* Generated stub for log_add */
void log_add(struct log *log UNNEEDED, const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "log_add called!\n"); abort(); }
/* Generated stub for log_book_new */
struct log_book *log_book_new(size_t max_mem UNNEEDED,
			      enum log_level printlevel UNNEEDED)
{ fprintf(stderr, "log_book_new called!\n"); abort(); }
/* Generated stub for log_io */
void log_io(struct log *log UNNEEDED, enum log_level dir UNNEEDED, const char *comment UNNEEDED,
	    const void *data UNNEEDED, size_t len UNNEEDED)
{ fprintf(stderr, "log_io called!\n"); abort(); }
/* Generated stub for log_new */
struct log *log_new(const tal_t *ctx UNNEEDED, struct log_book *record UNNEEDED, const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "log_new called!\n"); abort(); }
/* Generated stub for null_response */
struct json_stream *null_response(struct command *cmd UNNEEDED)
{ fprintf(stderr, "null_response called!\n"); abort(); }
/* Generated stub for onchaind_funding_spent */
enum watch_result onchaind_funding_spent(struct channel *channel UNNEEDED,
					 const struct bitcoin_tx *tx UNNEEDED,
					 u32 blockheight UNNEEDED)
{ fprintf(stderr, "onchaind_funding_spent called!\n"); abort(); }
/* Generated stub for opening_peer_no_active_channels */
void opening_peer_no_active_channels(struct peer *peer UNNEEDED)
{ fprintf(stderr, "opening_peer_no_active_channels called!\n"); abort(); }
/* Generated stub for param */
bool param(struct command *cmd UNNEEDED, const char *buffer UNNEEDED,
	   const jsmntok_t params[] UNNEEDED, ...)
{ fprintf(stderr, "param called!\n"); abort(); }
/* Generated stub for peer_start_channeld */
void peer_start_channeld(struct channel *channel UNNEEDED,
			 const struct crypto_state *cs UNNEEDED,
			 int peer_fd UNNEEDED, int gossip_fd UNNEEDED,
			 const u8 *funding_signed UNNEEDED,
			 bool reconnected UNNEEDED)
{ fprintf(stderr, "peer_start_channeld called!\n"); abort(); }
/* Generated stub for peer_start_closingd */
void peer_start_closingd(struct channel *channel UNNEEDED,
			 const struct crypto_state *cs UNNEEDED,
			 int peer_fd UNNEEDED, int gossip_fd UNNEEDED,
			 bool reconnected UNNEEDED,
			 const u8 *channel_reestablish UNNEEDED)
{ fprintf(stderr, "peer_start_closingd called!\n"); abort(); }
/* Generated stub for peer_start_openingd */
void peer_start_openingd(struct peer *peer UNNEEDED,
			 const struct crypto_state *cs UNNEEDED,
			 int peer_fd UNNEEDED, int gossip_fd UNNEEDED,
			 const u8 *msg UNNEEDED)
{ fprintf(stderr, "peer_start_openingd called!\n"); abort(); }
/* Generated stub for reltimer_new_ */
struct oneshot *reltimer_new_(struct timers *timers UNNEEDED,
			      const tal_t *ctx UNNEEDED,
			      struct timerel expire UNNEEDED,
			      void (*cb)(void *) UNNEEDED, void *arg UNNEEDED)
{ fprintf(stderr, "reltimer_new_ called!\n"); abort(); }
/* Generated stub for set_log_outfn_ */
void set_log_outfn_(struct log_book *lr UNNEEDED,
		    void (*print)(const char *prefix UNNEEDED,
				  enum log_level level UNNEEDED,
				  bool continued UNNEEDED,
				  const struct timeabs *time UNNEEDED,
				  const char *str UNNEEDED,
				  const u8 *io UNNEEDED,
				  void *arg) UNNEEDED,
		    void *arg UNNEEDED)
{ fprintf(stderr, "set_log_outfn_ called!\n"); abort(); }
/* Generated stub for subd_release_channel */
void subd_release_channel(struct subd *owner UNNEEDED, void *channel UNNEEDED)
{ fprintf(stderr, "subd_release_channel called!\n"); abort(); }
/* Generated stub for subd_req_ */
void subd_req_(const tal_t *ctx UNNEEDED,
	       struct subd *sd UNNEEDED,
	       const u8 *msg_out UNNEEDED,
	       int fd_out UNNEEDED, size_t num_fds_in UNNEEDED,
	       void (*replycb)(struct subd * UNNEEDED, const u8 * UNNEEDED, const int * UNNEEDED, void *) UNNEEDED,
	       void *replycb_data UNNEEDED)
{ fprintf(stderr, "subd_req_ called!\n"); abort(); }
/* Generated stub for subd_send_msg */
void subd_send_msg(struct subd *sd UNNEEDED, const u8 *msg_out UNNEEDED)
{ fprintf(stderr, "subd_send_msg called!\n"); abort(); }
/* Generated stub for towire_channel_dev_reenable_commit */
u8 *towire_channel_dev_reenable_commit(const tal_t *ctx UNNEEDED)
{ fprintf(stderr, "towire_channel_dev_reenable_commit called!\n"); abort(); }
/* Generated stub for towire_channel_send_shutdown */
u8 *towire_channel_send_shutdown(const tal_t *ctx UNNEEDED)
{ fprintf(stderr, "towire_channel_send_shutdown called!\n"); abort(); }
/* Generated stub for towire_connectctl_connect_to_peer */
u8 *towire_connectctl_connect_to_peer(const tal_t *ctx UNNEEDED, const struct pubkey *id UNNEEDED, u32 seconds_waited UNNEEDED, const struct wireaddr_internal *addrhint UNNEEDED)
{ fprintf(stderr, "towire_connectctl_connect_to_peer called!\n"); abort(); }
/* Generated stub for towire_connectctl_peer_disconnected */
u8 *towire_connectctl_peer_disconnected(const tal_t *ctx UNNEEDED, const struct pubkey *id UNNEEDED)
{ fprintf(stderr, "towire_connectctl_peer_disconnected called!\n"); abort(); }
/* Generated stub for towire_errorfmt */
u8 *towire_errorfmt(const tal_t *ctx UNNEEDED,
		    const struct channel_id *channel UNNEEDED,
		    const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "towire_errorfmt called!\n"); abort(); }
/* Generated stub for towire_gossip_get_incoming_channels */
u8 *towire_gossip_get_incoming_channels(const tal_t *ctx UNNEEDED)
{ fprintf(stderr, "towire_gossip_get_incoming_channels called!\n"); abort(); }
/* Generated stub for towire_hsm_get_channel_basepoints */
u8 *towire_hsm_get_channel_basepoints(const tal_t *ctx UNNEEDED, const struct pubkey *peerid UNNEEDED, u64 dbid UNNEEDED)
{ fprintf(stderr, "towire_hsm_get_channel_basepoints called!\n"); abort(); }
/* Generated stub for towire_hsm_sign_commitment_tx */
u8 *towire_hsm_sign_commitment_tx(const tal_t *ctx UNNEEDED, const struct pubkey *peer_id UNNEEDED, u64 channel_dbid UNNEEDED, const struct bitcoin_tx *tx UNNEEDED, const struct pubkey *remote_funding_key UNNEEDED, u64 funding_amount UNNEEDED)
{ fprintf(stderr, "towire_hsm_sign_commitment_tx called!\n"); abort(); }
/* Generated stub for towire_hsm_sign_invoice */
u8 *towire_hsm_sign_invoice(const tal_t *ctx UNNEEDED, const u8 *u5bytes UNNEEDED, const u8 *hrp UNNEEDED)
{ fprintf(stderr, "towire_hsm_sign_invoice called!\n"); abort(); }
/* Generated stub for txfilter_add_scriptpubkey */
void txfilter_add_scriptpubkey(struct txfilter *filter UNNEEDED, const u8 *script TAKES UNNEEDED)
{ fprintf(stderr, "txfilter_add_scriptpubkey called!\n"); abort(); }
/* Generated stub for wallet_channel_delete */
void wallet_channel_delete(struct wallet *w UNNEEDED, u64 wallet_id UNNEEDED)
{ fprintf(stderr, "wallet_channel_delete called!\n"); abort(); }
/* Generated stub for wallet_channel_save */
void wallet_channel_save(struct wallet *w UNNEEDED, struct channel *chan UNNEEDED)
{ fprintf(stderr, "wallet_channel_save called!\n"); abort(); }
/* Generated stub for wallet_channels_load_active */
bool wallet_channels_load_active(const tal_t *ctx UNNEEDED, struct wallet *w UNNEEDED)
{ fprintf(stderr, "wallet_channels_load_active called!\n"); abort(); }
/* Generated stub for wallet_channel_stats_load */
void wallet_channel_stats_load(struct wallet *w UNNEEDED, u64 cdbid UNNEEDED, struct channel_stats *stats UNNEEDED)
{ fprintf(stderr, "wallet_channel_stats_load called!\n"); abort(); }
/* Generated stub for wallet_channeltxs_add */
void wallet_channeltxs_add(struct wallet *w UNNEEDED, struct channel *chan UNNEEDED,
			    const int type UNNEEDED, const struct bitcoin_txid *txid UNNEEDED,
			   const u32 input_num UNNEEDED, const u32 blockheight UNNEEDED)
{ fprintf(stderr, "wallet_channeltxs_add called!\n"); abort(); }
/* Generated stub for wallet_htlcs_load_for_channel */
bool wallet_htlcs_load_for_channel(struct wallet *wallet UNNEEDED,
				   struct channel *chan UNNEEDED,
				   struct htlc_in_map *htlcs_in UNNEEDED,
				   struct htlc_out_map *htlcs_out UNNEEDED)
{ fprintf(stderr, "wallet_htlcs_load_for_channel called!\n"); abort(); }
/* Generated stub for wallet_invoice_autoclean */
void wallet_invoice_autoclean(struct wallet * wallet UNNEEDED,
			      u64 cycle_seconds UNNEEDED,
			      u64 expired_by UNNEEDED)
{ fprintf(stderr, "wallet_invoice_autoclean called!\n"); abort(); }
/* Generated stub for wallet_invoice_create */
bool wallet_invoice_create(struct wallet *wallet UNNEEDED,
			   struct invoice *pinvoice UNNEEDED,
			   u64 *msatoshi TAKES UNNEEDED,
			   const struct json_escaped *label TAKES UNNEEDED,
			   u64 expiry UNNEEDED,
			   const char *b11enc UNNEEDED,
			   const char *description UNNEEDED,
			   const struct preimage *r UNNEEDED,
			   const struct sha256 *rhash UNNEEDED)
{ fprintf(stderr, "wallet_invoice_create called!\n"); abort(); }
/* Generated stub for wallet_invoice_delete */
bool wallet_invoice_delete(struct wallet *wallet UNNEEDED,
			   struct invoice invoice UNNEEDED)
{ fprintf(stderr, "wallet_invoice_delete called!\n"); abort(); }
/* Generated stub for wallet_invoice_delete_expired */
void wallet_invoice_delete_expired(struct wallet *wallet UNNEEDED,
				   u64 max_expiry_time UNNEEDED)
{ fprintf(stderr, "wallet_invoice_delete_expired called!\n"); abort(); }
/* Generated stub for wallet_invoice_details */
const struct invoice_details *wallet_invoice_details(const tal_t *ctx UNNEEDED,
						     struct wallet *wallet UNNEEDED,
						     struct invoice invoice UNNEEDED)
{ fprintf(stderr, "wallet_invoice_details called!\n"); abort(); }
/* Generated stub for wallet_invoice_find_by_label */
bool wallet_invoice_find_by_label(struct wallet *wallet UNNEEDED,
				  struct invoice *pinvoice UNNEEDED,
				  const struct json_escaped *label UNNEEDED)
{ fprintf(stderr, "wallet_invoice_find_by_label called!\n"); abort(); }
/* Generated stub for wallet_invoice_find_by_rhash */
bool wallet_invoice_find_by_rhash(struct wallet *wallet UNNEEDED,
				  struct invoice *pinvoice UNNEEDED,
				  const struct sha256 *rhash UNNEEDED)
{ fprintf(stderr, "wallet_invoice_find_by_rhash called!\n"); abort(); }
/* Generated stub for wallet_invoice_iterate */
bool wallet_invoice_iterate(struct wallet *wallet UNNEEDED,
			    struct invoice_iterator *it UNNEEDED)
{ fprintf(stderr, "wallet_invoice_iterate called!\n"); abort(); }
/* Generated stub for wallet_invoice_iterator_deref */
const struct invoice_details *wallet_invoice_iterator_deref(const tal_t *ctx UNNEEDED,
			      struct wallet *wallet UNNEEDED,
			      const struct invoice_iterator *it UNNEEDED)
{ fprintf(stderr, "wallet_invoice_iterator_deref called!\n"); abort(); }
/* Generated stub for wallet_invoice_waitany */
void wallet_invoice_waitany(const tal_t *ctx UNNEEDED,
			    struct wallet *wallet UNNEEDED,
			    u64 lastpay_index UNNEEDED,
			    void (*cb)(const struct invoice * UNNEEDED, void*) UNNEEDED,
			    void *cbarg UNNEEDED)
{ fprintf(stderr, "wallet_invoice_waitany called!\n"); abort(); }
/* Generated stub for wallet_invoice_waitone */
void wallet_invoice_waitone(const tal_t *ctx UNNEEDED,
			    struct wallet *wallet UNNEEDED,
			    struct invoice invoice UNNEEDED,
			    void (*cb)(const struct invoice * UNNEEDED, void*) UNNEEDED,
			    void *cbarg UNNEEDED)
{ fprintf(stderr, "wallet_invoice_waitone called!\n"); abort(); }
/* Generated stub for wallet_peer_delete */
void wallet_peer_delete(struct wallet *w UNNEEDED, u64 peer_dbid UNNEEDED)
{ fprintf(stderr, "wallet_peer_delete called!\n"); abort(); }
/* Generated stub for wallet_transaction_locate */
struct txlocator *wallet_transaction_locate(const tal_t *ctx UNNEEDED, struct wallet *w UNNEEDED,
					    const struct bitcoin_txid *txid UNNEEDED)
{ fprintf(stderr, "wallet_transaction_locate called!\n"); abort(); }
/* Generated stub for watch_txid */
struct txwatch *watch_txid(const tal_t *ctx UNNEEDED,
			   struct chain_topology *topo UNNEEDED,
			   struct channel *channel UNNEEDED,
			   const struct bitcoin_txid *txid UNNEEDED,
			   enum watch_result (*cb)(struct lightningd *ld UNNEEDED,
						   struct channel *channel UNNEEDED,
						   const struct bitcoin_txid * UNNEEDED,
						   unsigned int depth))
{ fprintf(stderr, "watch_txid called!\n"); abort(); }
/* Generated stub for watch_txo */
struct txowatch *watch_txo(const tal_t *ctx UNNEEDED,
			   struct chain_topology *topo UNNEEDED,
			   struct channel *channel UNNEEDED,
			   const struct bitcoin_txid *txid UNNEEDED,
			   unsigned int output UNNEEDED,
			   enum watch_result (*cb)(struct channel *channel UNNEEDED,
						   const struct bitcoin_tx *tx UNNEEDED,
						   size_t input_num UNNEEDED,
						   const struct block *block))
{ fprintf(stderr, "watch_txo called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

#if DEVELOPER
/* Generated stub for dev_disconnect_permanent */
bool dev_disconnect_permanent(struct lightningd *ld UNNEEDED)
{ fprintf(stderr, "dev_disconnect_permanent called!\n"); abort(); }
#endif

static void add_inchan(struct route_info **inchans, int n)
{
	struct route_info *r = tal_arr_expand(inchans);
	memset(&r->pubkey, n, sizeof(r->pubkey));
	memset(&r->short_channel_id, n, sizeof(r->short_channel_id));
	r->fee_base_msat = r->fee_proportional_millionths = r->cltv_expiry_delta
		= n;
}

static void add_peer(struct lightningd *ld, int n, enum channel_state state,
		     bool connected)
{
	struct peer *peer = tal(ld, struct peer);
	struct channel *c = tal(peer, struct channel);

	memset(&peer->id, n, sizeof(peer->id));
	list_head_init(&peer->channels);
	list_add_tail(&ld->peers, &peer->list);

	c->state = state;
	c->owner = connected ? (void *)peer : NULL;
	/* Channel has incoming capacity n*1000 - 1 millisatoshi */
	c->funding_satoshi = n+1;
	c->our_msatoshi = 1;
	c->our_config.channel_reserve_satoshis = 1;
	list_add_tail(&peer->channels, &c->list);
}

/* There *is* padding in this structure, at the end. */
STRUCTEQ_DEF(route_info, ALIGNOF(struct route_info) - sizeof(u16),
	     pubkey,
	     short_channel_id,
	     fee_base_msat,
	     fee_proportional_millionths,
	     cltv_expiry_delta);

int main(void)
{
	struct lightningd *ld;
	bool any_offline;
	struct route_info *inchans;
	struct route_info **ret;
	size_t n;

	setup_locale();
	secp256k1_ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY
						 | SECP256K1_CONTEXT_SIGN);
	setup_tmpctx();
	ld = tal(tmpctx, struct lightningd);

	list_head_init(&ld->peers);

	inchans = tal_arr(tmpctx, struct route_info, 0);
	/* Nothing to choose from -> NULL result. */
	assert(select_inchan(tmpctx, ld, 0, inchans, &any_offline) == NULL);
	assert(any_offline == false);

	/* inchan but no peer -> NULL result. */
	add_inchan(&inchans, 0);
	assert(select_inchan(tmpctx, ld, 0, inchans, &any_offline) == NULL);
	assert(any_offline == false);

	/* connected peer but no inchan -> NULL result. */
	add_peer(ld, 1, CHANNELD_NORMAL, false);
	assert(select_inchan(tmpctx, ld, 0, inchans, &any_offline) == NULL);
	assert(any_offline == false);

	/* inchan but peer awaiting lockin -> NULL result. */
	add_peer(ld, 0, CHANNELD_AWAITING_LOCKIN, true);
	assert(select_inchan(tmpctx, ld, 0, inchans, &any_offline) == NULL);
	assert(any_offline == false);

	/* inchan but peer not connected -> NULL result. */
	add_inchan(&inchans, 1);
	assert(select_inchan(tmpctx, ld, 0, inchans, &any_offline) == NULL);
	assert(any_offline == true);

	/* Finally, a correct peer! */
	add_inchan(&inchans, 2);
	add_peer(ld, 2, CHANNELD_NORMAL, true);

	ret = select_inchan(tmpctx, ld, 0, inchans, &any_offline);
	assert(tal_count(ret) == 1);
	assert(tal_count(ret[0]) == 1);
	assert(any_offline == true);
	assert(route_info_eq(ret[0], &inchans[2]));

	/* Not if we ask for too much! Reserve is 1 satoshi */
	ret = select_inchan(tmpctx, ld, 1999, inchans, &any_offline);
	assert(tal_count(ret) == 1);
	assert(tal_count(ret[0]) == 1);
	assert(any_offline == false); /* Other candidate insufficient funds. */
	assert(route_info_eq(ret[0], &inchans[2]));

	ret = select_inchan(tmpctx, ld, 2000, inchans, &any_offline);
	assert(ret == NULL);
	assert(any_offline == false); /* Other candidate insufficient funds. */

	/* Add another candidate, with twice as much excess. */
	add_inchan(&inchans, 3);
	add_peer(ld, 3, CHANNELD_NORMAL, true);

	for (size_t i = n = 0; i < 1000; i++) {
		ret = select_inchan(tmpctx, ld, 1000, inchans, &any_offline);
		assert(tal_count(ret) == 1);
		assert(tal_count(ret[0]) == 1);
		assert(any_offline == false); /* Other candidate insufficient funds. */
		assert(route_info_eq(ret[0], &inchans[2])
		       || route_info_eq(ret[0], &inchans[3]));
		n += route_info_eq(ret[0], &inchans[2]);
	}

	/* Handwave over probability of this happening!  Within 20% */
	assert(n > 333 - 66 && n < 333 + 66);
	printf("Number of selections with 999 excess: %zu\n"
	       "Number of selections with 1999 excess: %zu\n",
	       n, 1000 - n);

	for (size_t i = n = 0; i < 1000; i++) {
		ret = select_inchan(tmpctx, ld, 1499, inchans, &any_offline);
		assert(tal_count(ret) == 1);
		assert(tal_count(ret[0]) == 1);
		assert(any_offline == false); /* Other candidate insufficient funds. */
		assert(route_info_eq(ret[0], &inchans[2])
		       || route_info_eq(ret[0], &inchans[3]));
		n += route_info_eq(ret[0], &inchans[2]);
	}

	assert(n > 250 - 50 && n < 250 + 50);
	printf("Number of selections with 500 excess: %zu\n"
	       "Number of selections with 1500 excess: %zu\n",
	       n, 1000 - n);

	/* No memory leaks please */
	secp256k1_context_destroy(secp256k1_ctx);
	tal_free(tmpctx);

	/* FIXME: Do BOLT comparison! */
	return 0;
}
