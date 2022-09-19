#include "config.h"
#include <ccan/array_size/array_size.h>
#include <ccan/mem/mem.h>
#include <ccan/ptrint/ptrint.h>
#include <ccan/tal/str/str.h>
#include <common/json_param.h>
#include <common/json_stream.h>
#include <plugins/libplugin.h>

enum subsystem {
	EXPIREDINVOICES,
#define NUM_SUBSYSTEM (EXPIREDINVOICES + 1)
};
static const char *subsystem_str[] = {
	"expiredinvoices",
};

static const char *subsystem_to_str(enum subsystem subsystem)
{
	assert(subsystem >= 0 && subsystem < NUM_SUBSYSTEM);
	return subsystem_str[subsystem];
}

static bool json_to_subsystem(const char *buffer, const jsmntok_t *tok,
			      enum subsystem *subsystem)
{
	for (size_t i = 0; i < NUM_SUBSYSTEM; i++) {
		if (memeqstr(buffer + tok->start, tok->end - tok->start,
			     subsystem_str[i])) {
			*subsystem = i;
			return true;
		}
	}
	return false;
}

/* For deprecated API, setting this to zero disabled autoclean */
static u64 deprecated_cycle_seconds = UINT64_MAX;
static u64 cycle_seconds = 3600;
static u64 subsystem_age[NUM_SUBSYSTEM];
static size_t cleanup_reqs_remaining;
static struct plugin *plugin;
static struct plugin_timer *cleantimer;

static void do_clean(void *cb_arg);

/* Fatal failures */
static struct command_result *cmd_failed(struct command *cmd,
					 const char *buf,
					 const jsmntok_t *result,
					 char *cmdname)
{
	plugin_err(plugin, "Failed '%s': '%.*s'", cmdname,
		   json_tok_full_len(result),
		   json_tok_full(buf, result));
}

static const char *datastore_path(const tal_t *ctx,
				  enum subsystem subsystem,
				  const char *field)
{
	return tal_fmt(ctx, "autoclean/%s/%s",
		       subsystem_to_str(subsystem), field);
}

static struct command_result *set_next_timer(struct plugin *plugin)
{
	plugin_log(plugin, LOG_DBG, "setting next timer");
	cleantimer = plugin_timer(plugin, time_from_sec(cycle_seconds), do_clean, plugin);
	return timer_complete(plugin);
}

static struct command_result *del_done(struct command *cmd,
				       const char *buf,
				       const jsmntok_t *result,
				       ptrint_t *unused)
{
	assert(cleanup_reqs_remaining != 0);
	plugin_log(plugin, LOG_DBG, "delinvoice_done: %zu remaining",
		   cleanup_reqs_remaining-1);
	if (--cleanup_reqs_remaining == 0)
		return set_next_timer(plugin);
	return command_still_pending(cmd);
}

static struct command_result *del_failed(struct command *cmd,
					   const char *buf,
					   const jsmntok_t *result,
					   ptrint_t *subsystemp)
{
	plugin_log(plugin, LOG_UNUSUAL, "%s del failed: %.*s",
		   subsystem_to_str(ptr2int(subsystemp)),
		   json_tok_full_len(result),
		   json_tok_full(buf, result));
	assert(cleanup_reqs_remaining != 0);
	if (--cleanup_reqs_remaining == 0)
		return command_still_pending(cmd);
	return set_next_timer(plugin);
}

static struct command_result *listinvoices_done(struct command *cmd,
						const char *buf,
						const jsmntok_t *result,
						char *unused)
{
	const jsmntok_t *t, *inv = json_get_member(buf, result, "invoices");
	size_t i;
	u64 now = time_now().ts.tv_sec;

	json_for_each_arr(i, t, inv) {
		const jsmntok_t *status = json_get_member(buf, t, "status");
		const jsmntok_t *time;
		u64 invtime;

		if (json_tok_streq(buf, status, "expired"))
			time = json_get_member(buf, t, "expires_at");
		else
			continue;

		if (!json_to_u64(buf, time, &invtime)) {
			plugin_err(plugin, "Bad time '%.*s'",
				   json_tok_full_len(time),
				   json_tok_full(buf, time));
		}

		if (invtime <= now - subsystem_age[EXPIREDINVOICES]) {
			struct out_req *req;
			const jsmntok_t *label = json_get_member(buf, t, "label");

			req = jsonrpc_request_start(plugin, NULL, "delinvoice",
						    del_done, del_failed,
						    int2ptr(EXPIREDINVOICES));
			json_add_tok(req->js, "label", label, buf);
			json_add_tok(req->js, "status", status, buf);
			send_outreq(plugin, req);
			plugin_log(plugin, LOG_DBG, "Expiring %.*s",
				   json_tok_full_len(label), json_tok_full(buf, label));
			cleanup_reqs_remaining++;
		}
	}

	if (cleanup_reqs_remaining)
		return command_still_pending(cmd);
	return set_next_timer(plugin);
}

static void do_clean(void *unused)
{
	struct out_req *req = NULL;

	assert(cleanup_reqs_remaining == 0);
	if (subsystem_age[EXPIREDINVOICES] != 0) {
		req = jsonrpc_request_start(plugin, NULL, "listinvoices",
					    listinvoices_done, cmd_failed,
					    (char *)"listinvoices");
		send_outreq(plugin, req);
	}

	if (!req)
		set_next_timer(plugin);
}

static struct command_result *json_autocleaninvoice(struct command *cmd,
						    const char *buffer,
						    const jsmntok_t *params)
{
	u64 *cycle;
	u64 *exby;
	struct json_stream *response;

	if (!param(cmd, buffer, params,
		   p_opt_def("cycle_seconds", param_u64, &cycle, 3600),
		   p_opt_def("expired_by", param_u64, &exby, 86400),
		   NULL))
		return command_param_failed();

	cleantimer = tal_free(cleantimer);

	if (*cycle == 0) {
		subsystem_age[EXPIREDINVOICES] = 0;
		response = jsonrpc_stream_success(cmd);
		json_add_bool(response, "enabled", false);
		return command_finished(cmd, response);
	}

	cycle_seconds = *cycle;
	subsystem_age[EXPIREDINVOICES] = *exby;
	cleantimer = plugin_timer(cmd->plugin, time_from_sec(cycle_seconds),
				  do_clean, cmd->plugin);

	response = jsonrpc_stream_success(cmd);
	json_add_bool(response, "enabled", true);
	json_add_u64(response, "cycle_seconds", cycle_seconds);
	json_add_u64(response, "expired_by", subsystem_age[EXPIREDINVOICES]);
	return command_finished(cmd, response);
}

static struct command_result *param_age(struct command *cmd, const char *name,
					const char *buffer, const jsmntok_t *tok,
					uint64_t **num)
{
	*num = tal(cmd, uint64_t);
	if (json_to_u64(buffer, tok, *num) && *num != 0)
		return NULL;

	if (json_tok_streq(buffer, tok, "never")) {
		**num = 0;
		return NULL;
	}

	return command_fail_badparam(cmd, name, buffer, tok,
				     "should be an positive 64 bit integer or 'never'");
}

static struct command_result *param_subsystem(struct command *cmd,
					      const char *name,
					      const char *buffer,
					      const jsmntok_t *tok,
					      enum subsystem **subsystem)
{
	*subsystem = tal(cmd, enum subsystem);
	if (json_to_subsystem(buffer, tok, *subsystem))
		return NULL;

	return command_fail_badparam(cmd, name, buffer, tok,
				     "should be a valid subsystem name");
}

static struct command_result *json_success_subsystems(struct command *cmd,
						      const enum subsystem *subsystem)
{
	struct json_stream *response = jsonrpc_stream_success(cmd);

	json_object_start(response, "autoclean");
	for (enum subsystem i = 0; i < NUM_SUBSYSTEM; i++) {
		if (subsystem && i != *subsystem)
			continue;
		json_object_start(response, subsystem_to_str(i));
		json_add_bool(response, "enabled", subsystem_age[i] != 0);
		if (subsystem_age[i] != 0)
			json_add_u64(response, "age", subsystem_age[i]);
		/* FIXME: Add stats! */
		json_object_end(response);
	}
	json_object_end(response);
	return command_finished(cmd, response);
}

static struct command_result *json_autoclean_status(struct command *cmd,
						    const char *buffer,
						    const jsmntok_t *params)
{
	enum subsystem *subsystem;

	if (!param(cmd, buffer, params,
		   p_opt("subsystem", param_subsystem, &subsystem),
		   NULL))
		return command_param_failed();

	return json_success_subsystems(cmd, subsystem);
}

static struct command_result *json_autoclean(struct command *cmd,
					     const char *buffer,
					     const jsmntok_t *params)
{
	enum subsystem *subsystem;
	u64 *age;

	if (!param(cmd, buffer, params,
		   p_req("subsystem", param_subsystem, &subsystem),
		   p_req("age", param_age, &age),
		   NULL))
		return command_param_failed();

	subsystem_age[*subsystem] = *age;
	jsonrpc_set_datastore_string(cmd->plugin, cmd,
				     datastore_path(tmpctx, *subsystem, "age"),
				     tal_fmt(tmpctx, "%"PRIu64, *age),
				     "create-or-replace", NULL, NULL, NULL);

	return json_success_subsystems(cmd, subsystem);
}

static const char *init(struct plugin *p,
			const char *buf UNUSED, const jsmntok_t *config UNUSED)
{
	plugin = p;
	if (deprecated_cycle_seconds != UINT64_MAX) {
		if (deprecated_cycle_seconds == 0) {
			plugin_log(p, LOG_DBG, "autocleaning not active");
			return NULL;
		} else
			cycle_seconds = deprecated_cycle_seconds;
	} else {
		bool active = false;
		for (enum subsystem i = 0; i < NUM_SUBSYSTEM; i++) {
			if (!rpc_scan_datastore_str(p, datastore_path(tmpctx, i, "age"),
						    JSON_SCAN(json_to_u64, &subsystem_age[i])))
				continue;
			if (subsystem_age[i]) {
				/* Only print this once! */
				if (!active)
					plugin_log(p, LOG_INFORM, "autocleaning every %"PRIu64" seconds", cycle_seconds);
				active = true;
				plugin_log(p, LOG_INFORM, "cleaning %s when age > %"PRIu64" seconds",
					   subsystem_to_str(i), subsystem_age[i]);
			}
		}
	}

	cleantimer = plugin_timer(p, time_from_sec(cycle_seconds), do_clean, p);

	return NULL;
}

static const struct plugin_command commands[] = { {
	"autocleaninvoice",
	"payment",
	"Set up autoclean of expired invoices. ",
	"Perform cleanup every {cycle_seconds} (default 3600), or disable autoclean if 0. "
	"Clean up expired invoices that have expired for {expired_by} seconds (default 86400). ",
	json_autocleaninvoice,
	true, /* deprecated! */
	}, {
	"autoclean",
	"utility",
	"Automatic deletion of old data (invoices, pays, forwards).",
	"Takes {subsystem} and {age} in seconds ",
	json_autoclean,
	}, {
	"autoclean-status",
	"utility",
	"Show status of autocleaning",
	"Takes optional {subsystem}",
	json_autoclean_status,
	},
};

int main(int argc, char *argv[])
{
	setup_locale();
	plugin_main(argv, init, PLUGIN_STATIC, true, NULL, commands, ARRAY_SIZE(commands),
	            NULL, 0, NULL, 0, NULL, 0,
		    plugin_option_deprecated("autocleaninvoice-cycle",
				  "string",
				  "Perform cleanup of expired invoices every"
				  " given seconds, or do not autoclean if 0",
				  u64_option, &deprecated_cycle_seconds),
		    plugin_option_deprecated("autocleaninvoice-expired-by",
				  "string",
				  "If expired invoice autoclean enabled,"
				  " invoices that have expired for at least"
				  " this given seconds are cleaned",
				  u64_option, &subsystem_age[EXPIREDINVOICES]),
		    plugin_option("autoclean-cycle",
				  "string",
				  "Perform cleanup every"
				  " given seconds",
				  u64_option, &cycle_seconds),
		    NULL);
}
