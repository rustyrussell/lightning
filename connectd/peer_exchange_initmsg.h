#ifndef LIGHTNING_CONNECTD_PEER_EXCHANGE_INITMSG_H
#define LIGHTNING_CONNECTD_PEER_EXCHANGE_INITMSG_H
#include "config.h"
#include <ccan/short_types/short_types.h>
#include <ccan/time/time.h>
#include <connectd/handshake.h>
#include <stdbool.h>

struct crypto_state;
struct daemon;
struct io_conn;
struct node_id;
struct wireaddr_internal;
struct oneshot;
struct feature_set;

/* If successful, calls peer_connected() */
struct io_plan *peer_exchange_initmsg(struct io_conn *conn,
				      struct daemon *daemon,
				      const struct feature_set *our_features,
				      const struct crypto_state *cs,
				      const struct node_id *id,
				      const struct wireaddr_internal *addr,
				      struct oneshot *timeout,
				      enum is_websocket is_websocket,
				      struct timemono starttime,
				      bool incoming);

#endif /* LIGHTNING_CONNECTD_PEER_EXCHANGE_INITMSG_H */
