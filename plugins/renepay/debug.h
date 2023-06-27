#ifndef LIGHTNING_PLUGINS_RENEPAY_DEBUG_H
#define LIGHTNING_PLUGINS_RENEPAY_DEBUG_H

#include <common/type_to_string.h>
#include <common/json_param.h>
#include <common/json_stream.h>
#include <plugins/libplugin.h>
#include <stdio.h>
#include <wire/peer_wire.h>
#include <ccan/json_out/json_out.h>
#include <pthread.h>

void _debug_outreq(const char *fname, const struct out_req *req);
void _debug_reply(const char* fname, const char* buf,const jsmntok_t *toks);
void _debug_info(const char* fname, const char *fmt, ...);
void _debug_call(const char* fname, const char* fun);
void _debug_exec_branch(const char* fname,const char* fun, int lineno);

#ifndef MYLOG
#define MYLOG "/tmp/debug.txt"
#endif

#ifdef FLOW_UNITTEST

#define debug_info(...) \
	_debug_info(MYLOG,__VA_ARGS__)

#define debug_err(...) \
	_debug_info(MYLOG,__VA_ARGS__); abort()

#define debug_paynote(p,...) \
	_debug_info(MYLOG,__VA_ARGS__);

#else

#include <plugins/renepay/pay.h>

#define debug_info(...) \
	plugin_log(pay_plugin->plugin,LOG_DBG,__VA_ARGS__)

#define debug_err(...) \
	plugin_err(pay_plugin->plugin,__VA_ARGS__)

#define debug_paynote(p,...) \
	paynote(p,__VA_ARGS__);

#endif

	
#endif
