#ifndef LIGHTNING_COMMON_WIREADDR_H
#define LIGHTNING_COMMON_WIREADDR_H
#include "config.h"
#include <ccan/short_types/short_types.h>
#include <ccan/tal/tal.h>
#include <stdbool.h>
#include <stdlib.h>

/* BOLT #7:
 *
 * The following `address descriptor` types are defined:
 *
 *   * `0`: padding.  data = none (length 0).
 *   * `1`: ipv4. data = `[4:ipv4_addr][2:port]` (length 6)
 *   * `2`: ipv6. data = `[16:ipv6_addr][2:port]` (length 18)
 *   * `3`: tor v2 onion service. data = `[10:onion_addr][2:port]` (length 12)
 *       * Version 2 onion service addresses. Encodes an 80-bit truncated `SHA-1` hash
 *         of a 1024-bit `RSA` public key for the onion service.
 *   * `4`: tor v3 onion service. data `[35:onion_addr][2:port]`  (length 37)
 *       * Version 3 ([prop224](https://gitweb.torproject.org/torspec.git/tree/proposals/224-rend-spec-ng.txt))
 *         onion service addresses. Encodes: `[32:32_byte_ed25519_pubkey] || [2:checksum] || [1:version]`.
 *             where `checksum = sha3(".onion checksum" | pubkey || version)[:2]`
 */

enum wire_addr_type {
	ADDR_TYPE_PADDING = 0,
	ADDR_TYPE_IPV4 = 1,
	ADDR_TYPE_IPV6 = 2,
};

/* FIXME(cdecker) Extend this once we have defined how TOR addresses
 * should look like */
struct wireaddr {
	enum wire_addr_type type;
	u8 addrlen;
	u8 addr[16];
	u16 port;
};

/* We use wireaddr to tell gossipd both what to listen on, and what to
 * announce */
enum addr_listen_announce {
	ADDR_LISTEN = (1 << 0),
	ADDR_ANNOUNCE = (1 << 1),
	ADDR_LISTEN_AND_ANNOUNCE = ADDR_LISTEN|ADDR_ANNOUNCE
};

/* Inserts a single ADDR_TYPE_PADDING if addr is NULL */
void towire_wireaddr(u8 **pptr, const struct wireaddr *addr);
bool fromwire_wireaddr(const u8 **cursor, size_t *max, struct wireaddr *addr);

enum addr_listen_announce fromwire_addr_listen_announce(const u8 **cursor,
							size_t *max);
void towire_addr_listen_announce(u8 **pptr, enum addr_listen_announce ala);

bool parse_wireaddr(const char *arg, struct wireaddr *addr, u16 port, const char **err_msg);

char *fmt_wireaddr(const tal_t *ctx, const struct wireaddr *a);

bool wireaddr_from_hostname(struct wireaddr *addr, const char *hostname,
			    const u16 port, const char **err_msg);

#endif /* LIGHTNING_COMMON_WIREADDR_H */
