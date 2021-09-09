/* This file was generated by generate-wire.py */
/* Do not modify this file! Modify the .csv file it was generated from. */
/* Original template can be found at tools/gen/impl_template */

#include <wire/common_wiregen.h>
#include <assert.h>
#include <ccan/array_size/array_size.h>
#include <ccan/mem/mem.h>
#include <ccan/tal/str/str.h>
#include <common/utils.h>
#include <stdio.h>

#ifndef SUPERVERBOSE
#define SUPERVERBOSE(...)
#endif


const char *common_wire_name(int e)
{
	static char invalidbuf[sizeof("INVALID ") + STR_MAX_CHARS(e)];

	switch ((enum common_wire)e) {
	case WIRE_CUSTOMMSG_IN: return "WIRE_CUSTOMMSG_IN";
	case WIRE_CUSTOMMSG_OUT: return "WIRE_CUSTOMMSG_OUT";
	}

	snprintf(invalidbuf, sizeof(invalidbuf), "INVALID %i", e);
	return invalidbuf;
}

bool common_wire_is_defined(u16 type)
{
	switch ((enum common_wire)type) {
	case WIRE_CUSTOMMSG_IN:;
	case WIRE_CUSTOMMSG_OUT:;
	      return true;
	}
	return false;
}





/* WIRE: CUSTOMMSG_IN */
/* A custom message that we got from a peer and don't know how to handle */
/* forward it to the master for further handling. */
u8 *towire_custommsg_in(const tal_t *ctx, const u8 *msg)
{
	u16 msg_len = tal_count(msg);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_CUSTOMMSG_IN);
	towire_u16(&p, msg_len);
	towire_u8_array(&p, msg, msg_len);

	return memcheck(p, tal_count(p));
}
bool fromwire_custommsg_in(const tal_t *ctx, const void *p, u8 **msg)
{
	u16 msg_len;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_CUSTOMMSG_IN)
		return false;
 	msg_len = fromwire_u16(&cursor, &plen);
 	// 2nd case msg
	*msg = msg_len ? tal_arr(ctx, u8, msg_len) : NULL;
	fromwire_u8_array(&cursor, &plen, *msg, msg_len);
	return cursor != NULL;
}

/* WIRE: CUSTOMMSG_OUT */
/* A custom message that the master tells us to send to the peer. */
u8 *towire_custommsg_out(const tal_t *ctx, const u8 *msg)
{
	u16 msg_len = tal_count(msg);
	u8 *p = tal_arr(ctx, u8, 0);

	towire_u16(&p, WIRE_CUSTOMMSG_OUT);
	towire_u16(&p, msg_len);
	towire_u8_array(&p, msg, msg_len);

	return memcheck(p, tal_count(p));
}
bool fromwire_custommsg_out(const tal_t *ctx, const void *p, u8 **msg)
{
	u16 msg_len;

	const u8 *cursor = p;
	size_t plen = tal_count(p);

	if (fromwire_u16(&cursor, &plen) != WIRE_CUSTOMMSG_OUT)
		return false;
 	msg_len = fromwire_u16(&cursor, &plen);
 	// 2nd case msg
	*msg = msg_len ? tal_arr(ctx, u8, msg_len) : NULL;
	fromwire_u8_array(&cursor, &plen, *msg, msg_len);
	return cursor != NULL;
}
// SHA256STAMP:0f582c50fa133054209e1f89e22ae78f969ada0d08ce162c63700051a6a25a9e
