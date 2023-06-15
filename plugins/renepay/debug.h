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

#ifndef MYLOCALDEBUG

#define debug_reply(buf,toks) \
	if(pay_plugin->debug_payflow) \
	plugin_log(pay_plugin->plugin,LOG_DBG,\
	"%.*s",json_tok_full_len(toks),json_tok_full(buf,toks))

#define debug_info(...) \
	if(pay_plugin->debug_payflow) \
	plugin_log(pay_plugin->plugin,LOG_DBG,__VA_ARGS__)
	
#define debug_call() \
	if(pay_plugin->debug_payflow) \
	plugin_log(pay_plugin->plugin,LOG_DBG,"calling %s",__PRETTY_FUNCTION__)

#define debug_exec_branch() \
	if(pay_plugin->debug_payflow) \
	plugin_log(pay_plugin->plugin,LOG_DBG,"executing line: %d (%s)",\
		__LINE__,__PRETTY_FUNCTION__)

#else

#define MYLOG "/tmp/debug.txt"
#define debug_outreq(req) \
	_debug_outreq(MYLOG,req)

#define debug_reply(buf,toks) \
	_debug_reply(MYLOG,buf,toks)

#define debug_info(...) \
	_debug_info(MYLOG,__VA_ARGS__)
	

#define debug_call() \
	_debug_call(MYLOG,__PRETTY_FUNCTION__)

#define debug_exec_branch() \
	_debug_exec_branch(MYLOG,__PRETTY_FUNCTION__,__LINE__)


#endif
#endif
