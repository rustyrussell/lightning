#ifndef LIGHTNING_PLUGINS_RENEPAY_DEBUG_H
#define LIGHTNING_PLUGINS_RENEPAY_DEBUG_H

#include <common/type_to_string.h>
#include <common/json_param.h>
#include <common/json_stream.h>
#include <plugins/libplugin.h>
#include <plugins/renepay/flow.h>
#include <plugins/renepay/pay_flow.h>
#include <stdio.h>
#include <wire/peer_wire.h>
#include <ccan/json_out/json_out.h>
#include <pthread.h>

#define MYLOG "/tmp/debug.txt"

void debug_knowledge(
		struct chan_extra_map* chan_extra_map,
		const char*fname);
		
void debug_payflows(struct pay_flow **flows, const char* fname);

void debug_exec_branch(int lineno, const char* fun, const char* fname);

void debug_outreq(const struct out_req *req, const char*fname);

void debug_call(const char* fun, const char* fname);

void debug_reply(const char* buf, const char*fname);

void debug(const char* fname, const char *fmt, ...);


#endif
