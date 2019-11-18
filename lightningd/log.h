#ifndef LIGHTNING_LIGHTNINGD_LOG_H
#define LIGHTNING_LIGHTNINGD_LOG_H
#include "config.h"
#include <ccan/list/list.h>
#include <ccan/tal/tal.h>
#include <ccan/time/time.h>
#include <ccan/typesafe_cb/typesafe_cb.h>
#include <common/status.h>
#include <common/type_to_string.h>
#include <jsmn.h>
#include <stdarg.h>

struct command;
struct json_stream;
struct lightningd;
struct node_id;
struct timerel;

/* We can have a single log book, with multiple logs in it: it's freed
 * by the last struct log itself. */
struct log_book *new_log_book(struct lightningd *ld, size_t max_mem,
			      enum log_level printlevel);

/* With different entry points */
struct log *new_log(const tal_t *ctx, struct log_book *record,
		    const struct node_id *default_node_id,
		    const char *fmt, ...) PRINTF_FMT(4,5);

#define log_debug(log, ...) log_((log), LOG_DBG, NULL, false, __VA_ARGS__)
#define log_info(log, ...) log_((log), LOG_INFORM, NULL, false, __VA_ARGS__)
#define log_unusual(log, ...) log_((log), LOG_UNUSUAL, NULL, true, __VA_ARGS__)
#define log_broken(log, ...) log_((log), LOG_BROKEN, NULL, true, __VA_ARGS__)

#define log_peer_debug(log, nodeid, ...) log_((log), LOG_DBG, nodeid, false, __VA_ARGS__)
#define log_peer_info(log, nodeid, ...) log_((log), LOG_INFORM, nodeid, false, __VA_ARGS__)
#define log_peer_unusual(log, nodeid, ...) log_((log), LOG_UNUSUAL, nodeid, true, __VA_ARGS__)
#define log_peer_broken(log, nodeid, ...) log_((log), LOG_BROKEN, nodeid, true, __VA_ARGS__)

void log_io(struct log *log, enum log_level dir,
	    const struct node_id *node_id,
	    const char *comment,
	    const void *data, size_t len);

void log_(struct log *log, enum log_level level,
	  const struct node_id *node_id,
	  bool call_notifier,
	  const char *fmt, ...)
	PRINTF_FMT(5,6);
void log_add(struct log *log, const char *fmt, ...) PRINTF_FMT(2,3);
void logv(struct log *log, enum log_level level, const struct node_id *node_id,
	  bool call_notifier, const char *fmt, va_list ap);
void logv_add(struct log *log, const char *fmt, va_list ap);

const char *log_prefix(const struct log *log);

void opt_register_logging(struct lightningd *ld);

char *arg_log_to_file(const char *arg, struct lightningd *ld);

/* Once this is set, we dump fatal with a backtrace to this log */
extern struct log *crashlog;
void NORETURN PRINTF_FMT(1,2) fatal(const char *fmt, ...);

void log_backtrace_print(const char *fmt, ...);
void log_backtrace_exit(void);

/* Adds an array showing log entries */
void json_add_log(struct json_stream *result,
		  const struct log_book *lr,
		  const struct node_id *node_id,
		  enum log_level minlevel);

struct command_result *param_loglevel(struct command *cmd,
				      const char *name,
				      const char *buffer,
				      const jsmntok_t *tok,
				      enum log_level **level);

struct log_entry {
	struct list_node list;
	struct timeabs time;
	enum log_level level;
	unsigned int skipped;
	struct node_id *node_id;
	const char *prefix;
	char *log;
	/* Iff LOG_IO */
	const u8 *io;
};

#endif /* LIGHTNING_LIGHTNINGD_LOG_H */
