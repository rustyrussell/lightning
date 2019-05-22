/* These functions must be supplied by any binary linking with common/param
 * so it can fail commands. */
#ifndef LIGHTNING_COMMON_JSON_COMMAND_H
#define LIGHTNING_COMMON_JSON_COMMAND_H
#include "config.h"
#include <ccan/compiler/compiler.h>
#include <stdbool.h>

struct command;
struct command_result;

/* The command category helps us to sort them. */
enum command_category {
	/* This is a Bitcoin network related command. */
	CMD_BITCOIN,
	/* This is a channels management related command. */
	CMD_CHANNELS,
	/* This is a (lightning) network related command. */
	CMD_NETWORK,
	/* This is a payment related command. */
	CMD_PAYMENT,
	/* This is a command added via an *external* plugin. */
	CMD_PLUGIN,
	/* This is an utility command. */
	CMD_UTILITY,
	/* This is a developer command. */
	CMD_DEVELOPER
};

/* Caller supplied this: param assumes it can call it. */
struct command_result *command_fail(struct command *cmd, int code,
				    const char *fmt, ...)
	PRINTF_FMT(3, 4) WARN_UNUSED_RESULT;

/* Also caller supplied: is this invoked simply to get usage? */
bool command_usage_only(const struct command *cmd);

/* If so, this is called. */
void command_set_usage(struct command *cmd, const char *usage);

/* Also caller supplied: is this invoked simply to check parameters? */
bool command_check_only(const struct command *cmd);

#endif /* LIGHTNING_COMMON_JSON_COMMAND_H */
