#ifndef LIGHTNING_LIGHTNINGD_JSONRPC_H
#define LIGHTNING_LIGHTNINGD_JSONRPC_H
#include "config.h"
#include <bitcoin/chainparams.h>
#include <ccan/autodata/autodata.h>
#include <ccan/list/list.h>
#include <common/json.h>

struct bitcoin_txid;
struct wireaddr;

/* Context for a command (from JSON, but might outlive the connection!)
 * You can allocate off this for temporary objects. */
struct command {
	/* Off jcon->commands */
	struct list_node list;
	/* The global state */
	struct lightningd *ld;
	/* The 'id' which we need to include in the response. */
	const char *id;
	/* The connection, or NULL if it closed. */
	struct json_connection *jcon;
	/* Have we been marked by command_still_pending?  For debugging... */
	bool pending;
};

struct json_connection {
	/* The global state */
	struct lightningd *ld;

	/* Logging for this json connection. */
	struct log *log;

	/* The buffer (required to interpret tokens). */
	char *buffer;

	/* Internal state: */
	/* How much is already filled. */
	size_t used;
	/* How much has just been filled. */
	size_t len_read;

	/* We've been told to stop. */
	bool stop;

	/* Current commands. */
	struct list_head commands;

	struct list_head output;
	const char *outbuf;
};

struct json_command {
	const char *name;
	void (*dispatch)(struct command *,
			 const char *buffer, const jsmntok_t *params);
	const char *description;
	bool deprecated;
	const char *verbose;
};

/* Get the parameters (by position or name).  Followed by triples of
 * of const char *name, const jsmntok_t **ret_ptr, then NULL.
 *
 * If name starts with '?' it is optional (and will be set to NULL
 * if it's a literal 'null' or not present).
 * Otherwise false is returned, and command_fail already called.
 */
bool json_get_params(struct command *cmd,
		     const char *buffer, const jsmntok_t param[], ...);

/* Get the parameters (by position or name).  Followed by JSON_PARAM_XXX...
 * then NULL.
 *
 * Optional parameters are either set to NULL if they are 'null' or
 * not present, otherwise they are allocated off @ctx.  Non-optional
 * parameters must be present.
 *
 * On any failure, false is returned, and command_fail already called.
 */
bool json_params(const tal_t *ctx,
		 struct command *cmd,
		 const char *buffer, const jsmntok_t param[], ...);

/* Turns it into the four parameters json_get_params() expects, while
 * checking that types are correct without double-evaluating @val or @fn. */
#define JSON_PARAM_(name, type, ptr, fn)				\
	name,								\
	false,								\
	(ptr)								\
		+ (sizeof(fn(NULL, (struct command *)NULL,		\
			     (const char *)NULL,			\
			     (const jsmntok_t *)NULL,			\
			     (type **)NULL) == (const char *)NULL)) * 0	\
		+ (sizeof((ptr) == (type *)NULL) * 0),			\
	(fn)

/* This variant takes a pointer-to-pointer */
#define JSON_PARAM_ALLOCS_(name, pptr, fn)				\
	name,								\
	true,								\
	(pptr)								\
		+ (sizeof(fn(NULL, (struct command *)NULL,		\
			     (const char *)NULL,			\
			     (const jsmntok_t *)NULL,			\
			     (pptr)) == (const char *)NULL)) * 0,	\
	(fn)

#define JSON_PARAM_BOOL(name, ptr) \
	JSON_PARAM_(name, bool, ptr, json_param_bool)
#define JSON_PARAM_U32(name, ptr) \
	JSON_PARAM_(name, u32, ptr, json_param_u32)
#define JSON_PARAM_U64(name, ptr) \
	JSON_PARAM_(name, u64, ptr, json_param_u64)
#define JSON_PARAM_PUBKEY(name, ptr) \
	JSON_PARAM_(name, struct pubkey, ptr, json_param_pubkey)
#define JSON_PARAM_DOUBLE(name, ptr) \
	JSON_PARAM_(name, double, ptr, json_param_double)
#define JSON_PARAM_SHA256(name, ptr) \
	JSON_PARAM_(name, struct sha256, ptr, json_param_sha256)
#define JSON_PARAM_SHORT_CHANNEL_ID(name, ptr) \
	JSON_PARAM_(name, struct short_channel_id, ptr, \
		    json_param_short_channel_id)
#define JSON_PARAM_STRING(name, pptr) \
	JSON_PARAM_ALLOCS_(name, pptr, json_param_string)
#define JSON_PARAM_ANY(name, ptr) \
	JSON_PARAM_(name, jsmntok_t, ptr, json_param_any)

#define JSON_PARAM_OPT_BOOL(name, pptr) \
	JSON_PARAM_ALLOCS_("?"name, pptr, json_param_bool)
#define JSON_PARAM_OPT_U32(name, pptr) \
	JSON_PARAM_ALLOCS_("?"name, pptr, json_param_u32)
#define JSON_PARAM_OPT_U64(name, pptr) \
	JSON_PARAM_ALLOCS_("?"name, pptr, json_param_u64)
#define JSON_PARAM_OPT_PUBKEY(name, pptr) \
	JSON_PARAM_ALLOCS_("?"name, pptr, json_param_pubkey)
#define JSON_PARAM_OPT_DOUBLE(name, pptr) \
	JSON_PARAM_ALLOCS_("?"name, pptr, json_param_double)
#define JSON_PARAM_OPT_STRING(name, pptr) \
	JSON_PARAM_ALLOCS_("?"name, pptr, json_param_string)
#define JSON_PARAM_OPT_SHORT_CHANNEL_ID(name, pptr) \
	JSON_PARAM_ALLOCS_("?"name, pptr, json_param_short_channel_id)
#define JSON_PARAM_OPT_ANY(name, ptr) \
	JSON_PARAM_OPT_ANY("?"name, ptr, json_param_any)

/* These all return NULL or an error description (which can allocate
 * off cmd if necessary).  If the *p is NULL, they allocate it off
 * ctx. */
const char *json_param_bool(const tal_t *ctx, struct command *cmd,
			    const char *buffer, const jsmntok_t *tok, bool **);
const char *json_param_u32(const tal_t *ctx, struct command *cmd,
			   const char *buffer, const jsmntok_t *tok, u32 **);
const char *json_param_u64(const tal_t *ctx, struct command *cmd,
			   const char *buffer, const jsmntok_t *tok, u64 **);
const char *json_param_pubkey(const tal_t *ctx, struct command *cmd,
			      const char *buffer, const jsmntok_t *tok,
			      struct pubkey **);
const char *json_param_double(const tal_t *ctx, struct command *cmd,
			      const char *buffer, const jsmntok_t *tok,
			      double **);
const char *json_param_sha256(const tal_t *ctx, struct command *cmd,
			      const char *buffer, const jsmntok_t *tok,
			      struct sha256 **);
const char *json_param_short_channel_id(const tal_t *ctx, struct command *cmd,
					const char *buffer, const jsmntok_t *tok,
					struct short_channel_id **);
const char *json_param_string(const tal_t *ctx, struct command *cmd,
			      const char *buffer, const jsmntok_t *tok,
			      struct json_escaped **);
const char *json_param_any(const tal_t *ctx, struct command *cmd,
			   const char *buffer, const jsmntok_t *tok,
			   const jsmntok_t **);


struct json_result *null_response(const tal_t *ctx);
void command_success(struct command *cmd, struct json_result *response);
void PRINTF_FMT(2, 3) command_fail(struct command *cmd, const char *fmt, ...);
void PRINTF_FMT(4, 5) command_fail_detailed(struct command *cmd,
					     int code,
					     const struct json_result *data,
					     const char *fmt, ...);

/* Mainly for documentation, that we plan to close this later. */
void command_still_pending(struct command *cmd);


/* For initialization */
void setup_jsonrpc(struct lightningd *ld, const char *rpc_filename);

enum address_parse_result {
	/* Not recognized as an onchain address */
	ADDRESS_PARSE_UNRECOGNIZED,
	/* Recognized as an onchain address, but targets wrong network */
	ADDRESS_PARSE_WRONG_NETWORK,
	/* Recognized and succeeds */
	ADDRESS_PARSE_SUCCESS,
};
/* Return result of address parsing and fills in *scriptpubkey
 * allocated off ctx if ADDRESS_PARSE_SUCCESS
 */
enum address_parse_result
json_tok_address_scriptpubkey(const tal_t *ctx,
			      const struct chainparams *chainparams,
			      const char *buffer,
			      const jsmntok_t *tok, const u8 **scriptpubkey);

AUTODATA_TYPE(json_command, struct json_command);
#endif /* LIGHTNING_LIGHTNINGD_JSONRPC_H */
