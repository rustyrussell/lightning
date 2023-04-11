#include "config.h"
#include "../flow.c"
#include "../not_mcf.c"
#include <ccan/array_size/array_size.h>
#include <ccan/read_write_all/read_write_all.h>
#include <common/bigsize.h>
#include <common/channel_id.h>
#include <common/node_id.h>
#include <common/setup.h>
#include <common/type_to_string.h>
#include <common/wireaddr.h>
#include <stdio.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_bigsize */
bigsize_t fromwire_bigsize(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_bigsize called!\n"); abort(); }
/* Generated stub for fromwire_channel_id */
bool fromwire_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			 struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_channel_id called!\n"); abort(); }
/* Generated stub for fromwire_wireaddr */
bool fromwire_wireaddr(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct wireaddr *addr UNNEEDED)
{ fprintf(stderr, "fromwire_wireaddr called!\n"); abort(); }
/* Generated stub for towire_bigsize */
void towire_bigsize(u8 **pptr UNNEEDED, const bigsize_t val UNNEEDED)
{ fprintf(stderr, "towire_bigsize called!\n"); abort(); }
/* Generated stub for towire_channel_id */
void towire_channel_id(u8 **pptr UNNEEDED, const struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "towire_channel_id called!\n"); abort(); }
/* Generated stub for towire_wireaddr */
void towire_wireaddr(u8 **pptr UNNEEDED, const struct wireaddr *addr UNNEEDED)
{ fprintf(stderr, "towire_wireaddr called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

/* Canned gossmap, taken from tests/test_gossip.py's
 * setup_gossip_store_test via od -v -Anone -tx1 < /tmp/ltests-kaf30pn0/test_gossip_store_compact_noappend_1/lightning-2/regtest/gossip_store
 */
static u8 canned_map[] = {
	0x09, 0x80, 0x00, 0x01, 0xbc, 0x09, 0x8b, 0x67, 0xe6, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x42, 0x40, 0x01, 0xb0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x22, 0x6e
	, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f
	, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x67
	, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x2d, 0x22, 0x36, 0x20, 0xa3, 0x59, 0xa4, 0x7f, 0xf7, 0xf7
	, 0xac, 0x44, 0x7c, 0x85, 0xc4, 0x6c, 0x92, 0x3d, 0xa5, 0x33, 0x89, 0x22, 0x1a, 0x00, 0x54, 0xc1
	, 0x1c, 0x1e, 0x3c, 0xa3, 0x1d, 0x59, 0x03, 0x5d, 0x2b, 0x11, 0x92, 0xdf, 0xba, 0x13, 0x4e, 0x10
	, 0xe5, 0x40, 0x87, 0x5d, 0x36, 0x6e, 0xbc, 0x8b, 0xc3, 0x53, 0xd5, 0xaa, 0x76, 0x6b, 0x80, 0xc0
	, 0x90, 0xb3, 0x9c, 0x3a, 0x5d, 0x88, 0x5d, 0x03, 0x1b, 0x84, 0xc5, 0x56, 0x7b, 0x12, 0x64, 0x40
	, 0x99, 0x5d, 0x3e, 0xd5, 0xaa, 0xba, 0x05, 0x65, 0xd7, 0x1e, 0x18, 0x34, 0x60, 0x48, 0x19, 0xff
	, 0x9c, 0x17, 0xf5, 0xe9, 0xd5, 0xdd, 0x07, 0x8f, 0x03, 0x1b, 0x84, 0xc5, 0x56, 0x7b, 0x12, 0x64
	, 0x40, 0x99, 0x5d, 0x3e, 0xd5, 0xaa, 0xba, 0x05, 0x65, 0xd7, 0x1e, 0x18, 0x34, 0x60, 0x48, 0x19
	, 0xff, 0x9c, 0x17, 0xf5, 0xe9, 0xd5, 0xdd, 0x07, 0x8f, 0x80, 0x00, 0x00, 0x8e, 0x33, 0x3b, 0x90
	, 0x12, 0x00, 0x00, 0x00, 0x00, 0x10, 0x06, 0x00, 0x8a, 0x01, 0x02, 0x14, 0xb8, 0x21, 0x42, 0x7d
	, 0x40, 0x89, 0x60, 0x71, 0x05, 0x8d, 0xe4, 0x50, 0x8e, 0xc3, 0x87, 0x6f, 0xa6, 0x4b, 0x19, 0xe4
	, 0x81, 0xc5, 0x5f, 0xb7, 0x04, 0xb8, 0x74, 0x08, 0x0b, 0x40, 0x5a, 0x74, 0x89, 0xbc, 0x63, 0x24
	, 0x27, 0x93, 0x4d, 0xfc, 0x1a, 0x72, 0xe4, 0xc7, 0xf8, 0x9b, 0xc1, 0x6b, 0xad, 0x9b, 0x04, 0x2e
	, 0x14, 0xa4, 0xe9, 0xf5, 0x80, 0xf1, 0x02, 0x8f, 0x50, 0xf3, 0x2c, 0x06, 0x22, 0x6e, 0x46, 0x11
	, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e
	, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x67, 0x00, 0x00
	, 0x01, 0x00, 0x00, 0x60, 0x17, 0x53, 0x70, 0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x3b
	, 0x02, 0x33, 0x80, 0x80, 0x00, 0x00, 0x8e, 0x3e, 0xa2, 0x81, 0xe6, 0x00, 0x00, 0x00, 0x00, 0x10
	, 0x06, 0x00, 0x8a, 0x01, 0x02, 0x01, 0x0a, 0xb3, 0x54, 0x3f, 0xd2, 0xa9, 0xf5, 0x30, 0x0f, 0x60
	, 0x7d, 0xf9, 0xf1, 0xdd, 0x63, 0x62, 0xd8, 0xde, 0xe2, 0x94, 0xe4, 0x68, 0xc9, 0x5c, 0xe8, 0x32
	, 0x9b, 0x14, 0xd9, 0xf8, 0x6a, 0x23, 0x3a, 0x67, 0x10, 0x09, 0x64, 0x96, 0x40, 0xcb, 0x0b, 0xf5
	, 0xec, 0xe6, 0xba, 0x8e, 0x77, 0xb4, 0x6a, 0xf1, 0x39, 0x94, 0x86, 0xb0, 0x69, 0xd5, 0x17, 0x67
	, 0x83, 0xda, 0xfa, 0x49, 0x63, 0x06, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12
	, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7
	, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x67, 0x00, 0x00, 0x01, 0x00, 0x00, 0x60, 0x17, 0x53
	, 0x70, 0x01, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x02, 0x33, 0x80, 0x00, 0x00, 0x00
	, 0x0a, 0x01, 0xf0, 0xcb, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x10, 0x07, 0x00, 0x00, 0x67, 0x00, 0x00
	, 0x01, 0x00, 0x00, 0x40, 0x00, 0x01, 0xb0, 0xd2, 0xfa, 0x8f, 0x8d, 0x60, 0x17, 0x53, 0x70, 0x01
	, 0x00, 0x24, 0xfd, 0xae, 0x1a, 0xc8, 0x40, 0xa7, 0x33, 0x22, 0xe1, 0x45, 0x7e, 0x76, 0xb8, 0x86
	, 0xdd, 0x17, 0x8c, 0xd4, 0x49, 0x4b, 0x14, 0x3f, 0x81, 0xd4, 0xd4, 0xfa, 0xa7, 0x16, 0x17, 0xd2
	, 0x51, 0x33, 0x9e, 0xcb, 0x0e, 0x22, 0x1c, 0xf6, 0x02, 0x3a, 0x2e, 0x3e, 0x94, 0xf8, 0xae, 0xdb
	, 0xee, 0x47, 0x23, 0xda, 0x5c, 0x35, 0x51, 0x57, 0xd8, 0xe4, 0x67, 0x2b, 0x46, 0x82, 0x5e, 0xc7
	, 0x98, 0x51, 0xb3, 0xb0, 0x1a, 0x2c, 0x72, 0x3f, 0x9b, 0xf5, 0xdb, 0xa8, 0xe3, 0x5f, 0x8b, 0x47
	, 0x9d, 0x9c, 0xd9, 0x73, 0xae, 0xc5, 0x0c, 0xca, 0x08, 0xfb, 0x97, 0x57, 0xb5, 0x21, 0x92, 0x05
	, 0x18, 0x42, 0x2d, 0x68, 0x19, 0x70, 0x76, 0x30, 0x61, 0x24, 0xff, 0xa5, 0xb6, 0x58, 0xa2, 0xe2
	, 0xb3, 0x68, 0x93, 0x37, 0xda, 0x6c, 0x3c, 0xcc, 0x5e, 0xf7, 0x3b, 0x51, 0x29, 0x64, 0x30, 0xbe
	, 0x2a, 0x19, 0x38, 0x88, 0x9d, 0xda, 0x2a, 0xd1, 0xcb, 0x5e, 0x33, 0xdb, 0x75, 0xcf, 0x2e, 0x0e
	, 0xfd, 0xbd, 0x38, 0xce, 0x01, 0x54, 0x62, 0x30, 0xb4, 0xdd, 0xdc, 0x7f, 0x67, 0xca, 0xf8, 0x39
	, 0x10, 0x02, 0x8a, 0x05, 0x3b, 0x76, 0x62, 0x72, 0xd2, 0x84, 0x71, 0x19, 0x19, 0x30, 0x92, 0xfa
	, 0x2a, 0x1f, 0xdf, 0x71, 0xe3, 0xd8, 0x4a, 0x56, 0xd0, 0xe4, 0x35, 0xfe, 0x5d, 0x4a, 0x5b, 0x5b
	, 0x90, 0x05, 0x28, 0xe4, 0x3b, 0x24, 0x13, 0x46, 0x99, 0x45, 0xc4, 0x92, 0x14, 0x7d, 0x43, 0x21
	, 0x06, 0x50, 0x51, 0xf8, 0x5b, 0x92, 0xb5, 0xb0, 0x90, 0xb1, 0xd7, 0x0d, 0x5a, 0xac, 0xfe, 0xf4
	, 0xe2, 0x70, 0x3e, 0x97, 0x42, 0x25, 0xfb, 0x21, 0x15, 0xf6, 0xb9, 0x32, 0xc8, 0xc3, 0x03, 0xbd
	, 0x7a, 0xbd, 0x86, 0xf7, 0xcd, 0x64, 0xe6, 0x1a, 0x7f, 0x5a, 0x04, 0x7a, 0x22, 0xad, 0x7c, 0xfc
	, 0x6a, 0x00, 0x00, 0x06, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43
	, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1
	, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x67, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x2d, 0x22, 0x36, 0x20
	, 0xa3, 0x59, 0xa4, 0x7f, 0xf7, 0xf7, 0xac, 0x44, 0x7c, 0x85, 0xc4, 0x6c, 0x92, 0x3d, 0xa5, 0x33
	, 0x89, 0x22, 0x1a, 0x00, 0x54, 0xc1, 0x1c, 0x1e, 0x3c, 0xa3, 0x1d, 0x59, 0x03, 0x5d, 0x2b, 0x11
	, 0x92, 0xdf, 0xba, 0x13, 0x4e, 0x10, 0xe5, 0x40, 0x87, 0x5d, 0x36, 0x6e, 0xbc, 0x8b, 0xc3, 0x53
	, 0xd5, 0xaa, 0x76, 0x6b, 0x80, 0xc0, 0x90, 0xb3, 0x9c, 0x3a, 0x5d, 0x88, 0x5d, 0x02, 0xd5, 0x95
	, 0xae, 0x92, 0xb3, 0x54, 0x4c, 0x32, 0x50, 0xfb, 0x77, 0x2f, 0x21, 0x4a, 0xd8, 0xd4, 0xc5, 0x14
	, 0x25, 0x03, 0x37, 0x40, 0xa5, 0xbc, 0xc3, 0x57, 0x19, 0x0a, 0xdd, 0x6d, 0x7e, 0x7a, 0x02, 0xd6
	, 0x06, 0x3d, 0x02, 0x26, 0x91, 0xb2, 0x49, 0x0a, 0xb4, 0x54, 0xde, 0xe7, 0x3a, 0x57, 0xc6, 0xff
	, 0x5d, 0x30, 0x83, 0x52, 0xb4, 0x61, 0xec, 0xe6, 0x9f, 0x3c, 0x28, 0x4f, 0x2c, 0x24, 0x12, 0x00
	, 0x00, 0x00, 0x0a, 0x91, 0x11, 0x83, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x10, 0x05, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x0f, 0x42, 0x40, 0xc0, 0x00, 0x00, 0x8a, 0xf3, 0x48, 0xd5, 0xb3, 0x60, 0x17, 0x53
	, 0x70, 0x01, 0x02, 0x14, 0xb8, 0x21, 0x42, 0x7d, 0x40, 0x89, 0x60, 0x71, 0x05, 0x8d, 0xe4, 0x50
	, 0x8e, 0xc3, 0x87, 0x6f, 0xa6, 0x4b, 0x19, 0xe4, 0x81, 0xc5, 0x5f, 0xb7, 0x04, 0xb8, 0x74, 0x08
	, 0x0b, 0x40, 0x5a, 0x74, 0x89, 0xbc, 0x63, 0x24, 0x27, 0x93, 0x4d, 0xfc, 0x1a, 0x72, 0xe4, 0xc7
	, 0xf8, 0x9b, 0xc1, 0x6b, 0xad, 0x9b, 0x04, 0x2e, 0x14, 0xa4, 0xe9, 0xf5, 0x80, 0xf1, 0x02, 0x8f
	, 0x50, 0xf3, 0x2c, 0x06, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43
	, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1
	, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x67, 0x00, 0x00, 0x01, 0x00, 0x00, 0x60, 0x17, 0x53, 0x70, 0x01
	, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00
	, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x02, 0x33, 0x80, 0xc0, 0x00, 0x00, 0x8a, 0xfe
	, 0xd1, 0xc4, 0x47, 0x60, 0x17, 0x53, 0x70, 0x01, 0x02, 0x01, 0x0a, 0xb3, 0x54, 0x3f, 0xd2, 0xa9
	, 0xf5, 0x30, 0x0f, 0x60, 0x7d, 0xf9, 0xf1, 0xdd, 0x63, 0x62, 0xd8, 0xde, 0xe2, 0x94, 0xe4, 0x68
	, 0xc9, 0x5c, 0xe8, 0x32, 0x9b, 0x14, 0xd9, 0xf8, 0x6a, 0x23, 0x3a, 0x67, 0x10, 0x09, 0x64, 0x96
	, 0x40, 0xcb, 0x0b, 0xf5, 0xec, 0xe6, 0xba, 0x8e, 0x77, 0xb4, 0x6a, 0xf1, 0x39, 0x94, 0x86, 0xb0
	, 0x69, 0xd5, 0x17, 0x67, 0x83, 0xda, 0xfa, 0x49, 0x63, 0x06, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b
	, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a
	, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x67, 0x00, 0x00, 0x01, 0x00
	, 0x00, 0x60, 0x17, 0x53, 0x70, 0x01, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x02, 0x33
	, 0x80, 0x40, 0x00, 0x00, 0x9b, 0x4f, 0x9d, 0xb7, 0xb9, 0x60, 0x17, 0x53, 0x77, 0x01, 0x01, 0x6e
	, 0x99, 0xf5, 0x9c, 0x1f, 0x21, 0x8d, 0x4a, 0x2b, 0x6e, 0x36, 0x9a, 0x95, 0x20, 0x76, 0x2c, 0x27
	, 0xfb, 0xa8, 0xb1, 0x82, 0x1f, 0x64, 0x34, 0x93, 0x91, 0x9c, 0xeb, 0xfa, 0x40, 0x50, 0x73, 0x4d
	, 0x00, 0xce, 0x10, 0xbf, 0x3f, 0x42, 0x3e, 0x56, 0x8f, 0xf8, 0xe0, 0x59, 0x58, 0xb5, 0xbd, 0xc5
	, 0x00, 0x82, 0xe3, 0x27, 0x92, 0x5b, 0xf8, 0x4f, 0x2c, 0x39, 0xec, 0x49, 0x3b, 0x07, 0x5e, 0x00
	, 0x0d, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x22, 0xaa, 0xa2, 0x60, 0x17
	, 0x53, 0x77, 0x02, 0x2d, 0x22, 0x36, 0x20, 0xa3, 0x59, 0xa4, 0x7f, 0xf7, 0xf7, 0xac, 0x44, 0x7c
	, 0x85, 0xc4, 0x6c, 0x92, 0x3d, 0xa5, 0x33, 0x89, 0x22, 0x1a, 0x00, 0x54, 0xc1, 0x1c, 0x1e, 0x3c
	, 0xa3, 0x1d, 0x59, 0x02, 0x2d, 0x22, 0x53, 0x49, 0x4c, 0x45, 0x4e, 0x54, 0x41, 0x52, 0x54, 0x49
	, 0x53, 0x54, 0x2d, 0x2d, 0x35, 0x36, 0x2d, 0x67, 0x64, 0x64, 0x31, 0x35, 0x33, 0x63, 0x38, 0x2d
	, 0x6d, 0x6f, 0x64, 0x64, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9b, 0x15, 0x33, 0x0c, 0x6b
	, 0x60, 0x17, 0x53, 0x77, 0x01, 0x01, 0x0e, 0x07, 0xaf, 0xd2, 0x33, 0x19, 0x0e, 0x06, 0x01, 0x6d
	, 0x57, 0x88, 0x4e, 0x66, 0xf8, 0x08, 0xd9, 0x65, 0x8a, 0x73, 0xfb, 0x1d, 0xe0, 0xad, 0xee, 0x47
	, 0xf8, 0x1c, 0xfc, 0xc3, 0xd2, 0xfd, 0x06, 0x3e, 0x5a, 0x05, 0x65, 0x72, 0x18, 0x61, 0xb8, 0x23
	, 0x04, 0x3d, 0x4b, 0x39, 0x79, 0xe0, 0x85, 0x38, 0xd2, 0x92, 0x14, 0x35, 0x32, 0xaa, 0x9f, 0xab
	, 0x5f, 0x98, 0x2c, 0x53, 0xfe, 0x0d, 0x00, 0x0d, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00
	, 0x00, 0x00, 0x22, 0xaa, 0xa2, 0x60, 0x17, 0x53, 0x77, 0x03, 0x5d, 0x2b, 0x11, 0x92, 0xdf, 0xba
	, 0x13, 0x4e, 0x10, 0xe5, 0x40, 0x87, 0x5d, 0x36, 0x6e, 0xbc, 0x8b, 0xc3, 0x53, 0xd5, 0xaa, 0x76
	, 0x6b, 0x80, 0xc0, 0x90, 0xb3, 0x9c, 0x3a, 0x5d, 0x88, 0x5d, 0x03, 0x5d, 0x2b, 0x48, 0x4f, 0x50
	, 0x50, 0x49, 0x4e, 0x47, 0x46, 0x49, 0x52, 0x45, 0x2d, 0x33, 0x2d, 0x35, 0x36, 0x2d, 0x67, 0x64
	, 0x64, 0x31, 0x35, 0x33, 0x63, 0x38, 0x2d, 0x6d, 0x6f, 0x64, 0x64, 0x65, 0x64, 0x00, 0x00, 0x40
	, 0x00, 0x00, 0x8a, 0x22, 0x55, 0xdf, 0xb2, 0x60, 0x17, 0x53, 0x79, 0x01, 0x02, 0x3a, 0x2b, 0xe5
	, 0x81, 0x83, 0xa3, 0x1a, 0x49, 0x93, 0x89, 0x8d, 0xac, 0xa7, 0xb2, 0x2e, 0xc3, 0x94, 0x6c, 0xd1
	, 0xd6, 0xd0, 0x82, 0x34, 0xf3, 0x9c, 0x71, 0xa0, 0xd1, 0xdb, 0x3f, 0xcc, 0xfc, 0x53, 0xce, 0x8c
	, 0x84, 0x3d, 0x14, 0x2c, 0x81, 0x4a, 0x07, 0xf0, 0x00, 0x03, 0x7a, 0x28, 0x10, 0xf4, 0xb9, 0x50
	, 0xb3, 0x22, 0x00, 0xdf, 0xc2, 0xc7, 0xfb, 0x6f, 0xf3, 0xfb, 0xf6, 0x94, 0x8e, 0x06, 0x22, 0x6e
	, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f
	, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x67
	, 0x00, 0x00, 0x01, 0x00, 0x00, 0x60, 0x17, 0x53, 0x79, 0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00
	, 0x00, 0x3b, 0x02, 0x33, 0x80, 0x00, 0x00, 0x01, 0xbc, 0x4d, 0x34, 0xb9, 0xcd, 0x00, 0x00, 0x00
	, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x42, 0x40, 0x01, 0xb0, 0x01, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x06, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b
	, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91
	, 0x0f, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x01, 0x00, 0x01, 0x02, 0x2d, 0x22, 0x36, 0x20, 0xa3, 0x59
	, 0xa4, 0x7f, 0xf7, 0xf7, 0xac, 0x44, 0x7c, 0x85, 0xc4, 0x6c, 0x92, 0x3d, 0xa5, 0x33, 0x89, 0x22
	, 0x1a, 0x00, 0x54, 0xc1, 0x1c, 0x1e, 0x3c, 0xa3, 0x1d, 0x59, 0x02, 0x66, 0xe4, 0x59, 0x8d, 0x1d
	, 0x3c, 0x41, 0x5f, 0x57, 0x2a, 0x84, 0x88, 0x83, 0x0b, 0x60, 0xf7, 0xe7, 0x44, 0xed, 0x92, 0x35
	, 0xeb, 0x0b, 0x1b, 0xa9, 0x32, 0x83, 0xb3, 0x15, 0xc0, 0x35, 0x18, 0x03, 0x1b, 0x84, 0xc5, 0x56
	, 0x7b, 0x12, 0x64, 0x40, 0x99, 0x5d, 0x3e, 0xd5, 0xaa, 0xba, 0x05, 0x65, 0xd7, 0x1e, 0x18, 0x34
	, 0x60, 0x48, 0x19, 0xff, 0x9c, 0x17, 0xf5, 0xe9, 0xd5, 0xdd, 0x07, 0x8f, 0x03, 0x1b, 0x84, 0xc5
	, 0x56, 0x7b, 0x12, 0x64, 0x40, 0x99, 0x5d, 0x3e, 0xd5, 0xaa, 0xba, 0x05, 0x65, 0xd7, 0x1e, 0x18
	, 0x34, 0x60, 0x48, 0x19, 0xff, 0x9c, 0x17, 0xf5, 0xe9, 0xd5, 0xdd, 0x07, 0x8f, 0x80, 0x00, 0x00
	, 0x8e, 0xf4, 0xbc, 0x2c, 0x78, 0x00, 0x00, 0x00, 0x00, 0x10, 0x06, 0x00, 0x8a, 0x01, 0x02, 0x7a
	, 0x2a, 0x3b, 0xad, 0x69, 0xf3, 0x8b, 0xba, 0xd2, 0xd3, 0xa2, 0x99, 0x66, 0x5f, 0x2d, 0x14, 0xc2
	, 0xca, 0xc2, 0xf4, 0x84, 0x97, 0x21, 0x93, 0x2f, 0xfd, 0x44, 0x19, 0xf6, 0xfa, 0x7f, 0x21, 0x3c
	, 0x61, 0x45, 0x1e, 0x67, 0xfd, 0x5f, 0x9e, 0xee, 0x35, 0x03, 0xda, 0x96, 0xc3, 0x37, 0x2b, 0xfd
	, 0x99, 0xb4, 0xdb, 0x0b, 0x6e, 0xa3, 0xdc, 0x8e, 0xad, 0x64, 0xf5, 0x9a, 0x4f, 0x5f, 0xae, 0x06
	, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28
	, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00
	, 0x00, 0x6e, 0x00, 0x00, 0x01, 0x00, 0x01, 0x60, 0x17, 0x53, 0x7a, 0x01, 0x00, 0x00, 0x06, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00
	, 0x00, 0x00, 0x00, 0x3b, 0x02, 0x33, 0x80, 0x80, 0x00, 0x00, 0x8e, 0xc3, 0xd8, 0xfd, 0x83, 0x00
	, 0x00, 0x00, 0x00, 0x10, 0x06, 0x00, 0x8a, 0x01, 0x02, 0x17, 0xdf, 0xc0, 0xb6, 0x5f, 0x8f, 0x42
	, 0x50, 0xe1, 0x4d, 0x35, 0xe7, 0x57, 0x2a, 0x07, 0x66, 0x8e, 0xa9, 0xe2, 0x61, 0xbf, 0xbc, 0x91
	, 0x5c, 0xa1, 0x80, 0x43, 0xcf, 0xb2, 0xba, 0x40, 0xf6, 0x2f, 0x0d, 0x37, 0x2c, 0xbc, 0x90, 0x96
	, 0x71, 0x00, 0x79, 0x35, 0xe3, 0xe8, 0x94, 0x90, 0x3c, 0x23, 0x8f, 0x5b, 0x8e, 0xcc, 0x39, 0x82
	, 0x2e, 0xdf, 0xbc, 0xcb, 0x66, 0xe9, 0xe4, 0x3e, 0xad, 0x06, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b
	, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a
	, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x01, 0x00
	, 0x01, 0x60, 0x17, 0x53, 0x7a, 0x01, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x02, 0x33
	, 0x80, 0x40, 0x00, 0x00, 0x8a, 0x62, 0xdd, 0xe7, 0xfd, 0x60, 0x17, 0x53, 0x7b, 0x01, 0x02, 0x0b
	, 0x5d, 0x1b, 0x41, 0x29, 0x50, 0xe7, 0x79, 0x39, 0x76, 0xc2, 0xd0, 0xbd, 0x54, 0x2c, 0x1c, 0x2b
	, 0x78, 0x25, 0x8b, 0xd6, 0x2d, 0x70, 0x09, 0x73, 0xb7, 0x1c, 0xe4, 0xa2, 0x88, 0x98, 0xb6, 0x44
	, 0xa5, 0x33, 0x0a, 0x98, 0xdc, 0x63, 0xd1, 0x7b, 0x99, 0x49, 0xf2, 0x29, 0xe6, 0x6f, 0x58, 0xc6
	, 0xcb, 0x5a, 0x74, 0xa0, 0xdf, 0xa7, 0x74, 0x84, 0xd5, 0xe1, 0x0f, 0x03, 0x7d, 0xb6, 0xcd, 0x06
	, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28
	, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00
	, 0x00, 0x67, 0x00, 0x00, 0x01, 0x00, 0x00, 0x60, 0x17, 0x53, 0x7b, 0x01, 0x01, 0x00, 0x06, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x03, 0xe8, 0x00
	, 0x00, 0x00, 0x00, 0x3b, 0x02, 0x33, 0x80, 0x00, 0x00, 0x00, 0x8e, 0x76, 0xce, 0x94, 0x8e, 0x00
	, 0x00, 0x00, 0x00, 0x10, 0x06, 0x00, 0x8a, 0x01, 0x02, 0x25, 0x9f, 0x23, 0x6a, 0xbd, 0x5b, 0x6a
	, 0x6b, 0x0f, 0x77, 0xaa, 0xce, 0xe9, 0xe1, 0x6d, 0xe3, 0xfb, 0xcd, 0x10, 0xa6, 0x2b, 0xb6, 0x15
	, 0x0c, 0xdf, 0xa1, 0xde, 0x79, 0x82, 0x99, 0xb1, 0x83, 0x47, 0x44, 0xf7, 0x20, 0xbc, 0x49, 0x11
	, 0x59, 0x58, 0x25, 0x63, 0x76, 0x01, 0x69, 0x27, 0xdc, 0xb3, 0x6c, 0x68, 0xc8, 0x5f, 0xae, 0x13
	, 0xaa, 0x46, 0xcc, 0xe9, 0x68, 0x03, 0x2a, 0xd3, 0x21, 0x06, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b
	, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a
	, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x01, 0x00
	, 0x01, 0x60, 0x17, 0x53, 0x7f, 0x01, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x02, 0x33
	, 0x80, 0x00, 0x00, 0x00, 0x8e, 0x4b, 0x8b, 0x0b, 0xf9, 0x00, 0x00, 0x00, 0x00, 0x10, 0x06, 0x00
	, 0x8a, 0x01, 0x02, 0x7b, 0xd9, 0xa5, 0xe6, 0xfb, 0x26, 0xe2, 0xe1, 0xcb, 0x9a, 0x68, 0xdf, 0x50
	, 0x6c, 0x14, 0xcb, 0x5a, 0x2d, 0x12, 0x40, 0x94, 0x5e, 0xa4, 0x2d, 0xe9, 0x2a, 0x29, 0x48, 0xd5
	, 0xd0, 0x2e, 0xd9, 0x0c, 0xdc, 0xba, 0xe2, 0x74, 0x6e, 0xfb, 0xca, 0x77, 0xea, 0xe9, 0xa2, 0xce
	, 0x9a, 0xa8, 0x42, 0x09, 0xa3, 0xa3, 0xae, 0x0e, 0x0f, 0xcc, 0xd3, 0x93, 0xd5, 0xcc, 0x38, 0x76
	, 0xd3, 0x58, 0xcc, 0x06, 0x22, 0x6e, 0x46, 0x11, 0x1a, 0x0b, 0x59, 0xca, 0xaf, 0x12, 0x60, 0x43
	, 0xeb, 0x5b, 0xbf, 0x28, 0xc3, 0x4f, 0x3a, 0x5e, 0x33, 0x2a, 0x1f, 0xc7, 0xb2, 0xb7, 0x3c, 0xf1
	, 0x88, 0x91, 0x0f, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x01, 0x00, 0x01, 0x60, 0x17, 0x53, 0x7f, 0x01
	, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00
	, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x02, 0x33, 0x80
};

/* not_mcf sets NDEBUG, so assert() is useless */
#define ASSERT(x) do { if (!(x)) abort(); } while(0)

static void print_flows(const char *desc,
			const struct gossmap *gossmap,
			struct flow **flows)
{
	printf("%s: %zu subflows\n", desc, tal_count(flows));
	for (size_t i = 0; i < tal_count(flows); i++) {
		struct amount_msat fee, delivered;
		printf("   ");
		for (size_t j = 0; j < tal_count(flows[i]->path); j++) {
			struct short_channel_id scid
				= gossmap_chan_scid(gossmap,
						    flows[i]->path[j]);
			printf("%s%s", j ? "->" : "",
			       type_to_string(tmpctx, struct short_channel_id, &scid));
		}
		delivered = flows[i]->amounts[tal_count(flows[i]->amounts)-1];
		if (!amount_msat_sub(&fee, flows[i]->amounts[0], delivered))
			abort();
		printf(" prob %.2f, %s delivered with fee %s\n",
		       flows[i]->success_prob,
		       type_to_string(tmpctx, struct amount_msat, &delivered),
		       type_to_string(tmpctx, struct amount_msat, &fee));
	}
}

int main(int argc, char *argv[])
{
	int fd;
	char *gossfile;
	struct gossmap *gossmap;
	struct node_id l1, l2, l3;
	struct flow **flows, **flows2;
	struct short_channel_id scid12, scid23, scid13;
	struct gossmap_localmods *mods;
	struct capacity_range *capacities;

	struct gossmap_chan *local_chan;

	common_setup(argv[0]);

	fd = tmpdir_mkstemp(tmpctx, "run-not_mcf.XXXXXX", &gossfile);
	ASSERT(write_all(fd, canned_map, sizeof(canned_map)));

	gossmap = gossmap_load(tmpctx, gossfile, NULL);
	ASSERT(gossmap);

	/* There is a public channel 2<->3 (103x1x0), and private
	 * 1<->2 (110x1x1). */
	ASSERT(node_id_from_hexstr("0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518", 66, &l1));
	ASSERT(node_id_from_hexstr("022d223620a359a47ff7f7ac447c85c46c923da53389221a0054c11c1e3ca31d59", 66, &l2));
	ASSERT(node_id_from_hexstr("035d2b1192dfba134e10e540875d366ebc8bc353d5aa766b80c090b39c3a5d885d", 66, &l3));
	ASSERT(short_channel_id_from_str("110x1x1", 7, &scid12));
	ASSERT(short_channel_id_from_str("103x1x0", 7, &scid23));

	flows = minflow(tmpctx, gossmap,
			gossmap_find_node(gossmap, &l1),
			gossmap_find_node(gossmap, &l3),
			flow_capacity_init(tmpctx, gossmap),
			/* Half the capacity */
			AMOUNT_MSAT(500000000),
			1,
			1);
	print_flows("Flow via single path l1->l2->l3", gossmap, flows);

	/* Should go 1->2->3 */
	ASSERT(tal_count(flows) == 1);
	ASSERT(tal_count(flows[0]->path) == 2);
	ASSERT(tal_count(flows[0]->dirs) == 2);
	ASSERT(tal_count(flows[0]->amounts) == 2);

	ASSERT(flows[0]->path[0] == gossmap_find_chan(gossmap, &scid12));
	ASSERT(flows[0]->path[1] == gossmap_find_chan(gossmap, &scid23));
	ASSERT(flows[0]->dirs[0] == 1);
	ASSERT(flows[0]->dirs[1] == 0);
	ASSERT(amount_msat_eq(flows[0]->amounts[1], AMOUNT_MSAT(500000000)));
	/* fee_base_msat == 20, fee_proportional_millionths == 1000 */
	ASSERT(amount_msat_eq(flows[0]->amounts[0], AMOUNT_MSAT(500000000 + 500000 + 20)));

	/* Each one has probability ~ 0.5 */
	ASSERT(flows[0]->success_prob > 0.249);
	ASSERT(flows[0]->success_prob <= 0.250);

	/* Now try adding a local channel scid */
	mods = gossmap_localmods_new(tmpctx);
	ASSERT(short_channel_id_from_str("111x1x1", 7, &scid13));

	/* 400,000sat channel from 1->3, basefee 0, ppm 1000, delay 5 */
	ASSERT(gossmap_local_addchan(mods, &l1, &l3, &scid13, NULL));
	ASSERT(gossmap_local_updatechan(mods, &scid13,
					AMOUNT_MSAT(0),
					AMOUNT_MSAT(400000000),
					0, 1000, 5,
					true,
					0));

	/* Apply changes, check they work. */
	gossmap_apply_localmods(gossmap, mods);
	local_chan = gossmap_find_chan(gossmap, &scid13);
	ASSERT(local_chan);

	/* The local chans have no "capacity", so set them manually. */
	capacities = flow_capacity_init(tmpctx, gossmap);
	capacities[gossmap_chan_idx(gossmap, local_chan)].min
		= AMOUNT_MSAT(0);
	capacities[gossmap_chan_idx(gossmap, local_chan)].max
		= AMOUNT_MSAT(400000000);

	flows = minflow(tmpctx, gossmap,
			gossmap_find_node(gossmap, &l1),
			gossmap_find_node(gossmap, &l3),
			capacities,
			/* This will go first via 1-2-3, then 1->3. */
			AMOUNT_MSAT(500000000),
			0.1,
			1);

	print_flows("Flow via two paths, low mu", gossmap, flows);

	ASSERT(tal_count(flows) == 2);
	ASSERT(tal_count(flows[0]->path) == 2);
	ASSERT(tal_count(flows[0]->dirs) == 2);
	ASSERT(tal_count(flows[0]->amounts) == 2);

	ASSERT(flows[0]->path[0] == gossmap_find_chan(gossmap, &scid12));
	ASSERT(flows[0]->path[1] == gossmap_find_chan(gossmap, &scid23));
	ASSERT(flows[0]->dirs[0] == 1);
	ASSERT(flows[0]->dirs[1] == 0);

	/* First one has probability < 50% */
	ASSERT(flows[0]->success_prob < 0.50);

	ASSERT(tal_count(flows[1]->path) == 1);
	ASSERT(tal_count(flows[1]->dirs) == 1);
	ASSERT(tal_count(flows[1]->amounts) == 1);

	/* We will try cheaper path first, but not to fill it! */
	ASSERT(flows[1]->path[0] == gossmap_find_chan(gossmap, &scid13));
	ASSERT(flows[1]->dirs[0] == 0);
	ASSERT(amount_msat_less(flows[1]->amounts[0], AMOUNT_MSAT(400000000)));

	/* Second one has probability < 50% */
	ASSERT(flows[1]->success_prob < 0.50);

	/* Delivered amount must be the total! */
	ASSERT(flows[0]->amounts[1].millisatoshis
	       + flows[1]->amounts[0].millisatoshis == 500000000);

	/* Higher mu values mean we pay more for certainty! */
	flows2 = minflow(tmpctx, gossmap,
			 gossmap_find_node(gossmap, &l1),
			 gossmap_find_node(gossmap, &l3),
			 capacities,
			 /* This will go 400000000 via 1->3, rest via 1-2-3. */
			 AMOUNT_MSAT(500000000),
			 10,
			 1);
	print_flows("Flow via two paths, high mu", gossmap, flows2);
	ASSERT(tal_count(flows2) == 2);
	ASSERT(tal_count(flows2[0]->path) == 1);
	ASSERT(tal_count(flows2[1]->path) == 2);

	/* Sends more via 1->3, since it's more expensive (but lower prob) */
	ASSERT(amount_msat_greater(flows2[0]->amounts[0], flows[1]->amounts[0]));
	ASSERT(flows2[0]->success_prob < flows[1]->success_prob);

	/* Delivered amount must be the total! */
	ASSERT(flows2[0]->amounts[0].millisatoshis
	       + flows2[1]->amounts[1].millisatoshis == 500000000);

	/* But in total it's more expensive! */
	ASSERT(flows2[0]->amounts[0].millisatoshis + flows2[1]->amounts[0].millisatoshis
	       > flows[0]->amounts[0].millisatoshis - flows[1]->amounts[0].millisatoshis);


	common_shutdown();
}
