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

#define MYLOG "/tmp/debug.txt"


void _debug_outreq(const char *fname, const struct out_req *req);
#define debug_outreq(req) \
	_debug_outreq(MYLOG,req)

void _debug_reply(const char* fname, const char* buf,const jsmntok_t *toks);
#define debug_reply(buf,toks) \
	_debug_reply(MYLOG,buf,toks)

void _debug_info(const char* fname, const char *fmt, ...);
#define debug_info(...) \
	_debug_info(MYLOG,__VA_ARGS__)
	

void _debug_call(const char* fname, const char* fun);
#define debug_call() \
	_debug_call(MYLOG,__PRETTY_FUNCTION__)

void _debug_exec_branch(const char* fname,const char* fun, int lineno);
#define debug_exec_branch() \
	_debug_exec_branch(MYLOG,__PRETTY_FUNCTION__,__LINE__)

#endif
