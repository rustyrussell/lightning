/* This file was generated by generate-wire.py */
/* Do not modify this file! Modify the _csv file it was generated from. */
/* Original template can be found at tools/gen/header_template */

#ifndef LIGHTNING_CONNECTD_CONNECTD_WIREGEN_H
#define LIGHTNING_CONNECTD_CONNECTD_WIREGEN_H
#include <ccan/tal/tal.h>
#include <wire/tlvstream.h>
#include <wire/wire.h>
#include <common/cryptomsg.h>
#include <common/features.h>
#include <common/per_peer_state.h>
#include <common/wireaddr.h>
#include <lightningd/gossip_msg.h>

enum connectd_wire {
        WIRE_CONNECTD_INIT = 2000,
        /*  Connectd->master */
        WIRE_CONNECTD_INIT_REPLY = 2100,
        /*  Activate the connect daemon */
        WIRE_CONNECTD_ACTIVATE = 2025,
        /*  Connectd->master */
        WIRE_CONNECTD_ACTIVATE_REPLY = 2125,
        /*  connectd->master: disconnect this peer please (due to reconnect). */
        WIRE_CONNECTD_RECONNECTED = 2112,
        /*  Master -> connectd: connect to a peer. */
        WIRE_CONNECTD_CONNECT_TO_PEER = 2001,
        /*  Connectd->master: connect failed. */
        WIRE_CONNECTD_CONNECT_FAILED = 2020,
        /*  Connectd -> master: we got a peer. Three fds: peer */
        WIRE_CONNECTD_PEER_CONNECTED = 2002,
        /*  master -> connectd: peer has disconnected. */
        WIRE_CONNECTD_PEER_DISCONNECTED = 2015,
        /*  master -> connectd: do you have a memleak? */
        WIRE_CONNECTD_DEV_MEMLEAK = 2033,
        WIRE_CONNECTD_DEV_MEMLEAK_REPLY = 2133,
};

const char *connectd_wire_name(int e);

/**
 * Determine whether a given message type is defined as a message.
 *
 * Returns true if the message type is part of the message definitions we have
 * generated parsers for, false if it is a custom message that cannot be
 * handled internally.
 */
bool connectd_wire_is_defined(u16 type);


/* WIRE: CONNECTD_INIT */
u8 *towire_connectd_init(const tal_t *ctx, const struct chainparams *chainparams, const struct feature_set *our_features, const struct node_id *id, const struct wireaddr_internal *wireaddrs, const enum addr_listen_announce *listen_announce, const struct wireaddr *tor_proxyaddr, bool use_tor_proxy_always, bool dev_allow_localhost, bool use_dns, const wirestring *tor_password, bool use_v3_autotor);
bool fromwire_connectd_init(const tal_t *ctx, const void *p, const struct chainparams **chainparams, struct feature_set **our_features, struct node_id *id, struct wireaddr_internal **wireaddrs, enum addr_listen_announce **listen_announce, struct wireaddr **tor_proxyaddr, bool *use_tor_proxy_always, bool *dev_allow_localhost, bool *use_dns, wirestring **tor_password, bool *use_v3_autotor);

/* WIRE: CONNECTD_INIT_REPLY */
/*  Connectd->master */
u8 *towire_connectd_init_reply(const tal_t *ctx, const struct wireaddr_internal *bindings, const struct wireaddr *announcable);
bool fromwire_connectd_init_reply(const tal_t *ctx, const void *p, struct wireaddr_internal **bindings, struct wireaddr **announcable);

/* WIRE: CONNECTD_ACTIVATE */
/*  Activate the connect daemon */
u8 *towire_connectd_activate(const tal_t *ctx, bool listen);
bool fromwire_connectd_activate(const void *p, bool *listen);

/* WIRE: CONNECTD_ACTIVATE_REPLY */
/*  Connectd->master */
u8 *towire_connectd_activate_reply(const tal_t *ctx);
bool fromwire_connectd_activate_reply(const void *p);

/* WIRE: CONNECTD_RECONNECTED */
/*  connectd->master: disconnect this peer please (due to reconnect). */
u8 *towire_connectd_reconnected(const tal_t *ctx, const struct node_id *id);
bool fromwire_connectd_reconnected(const void *p, struct node_id *id);

/* WIRE: CONNECTD_CONNECT_TO_PEER */
/*  Master -> connectd: connect to a peer. */
u8 *towire_connectd_connect_to_peer(const tal_t *ctx, const struct node_id *id, u32 seconds_waited, const struct wireaddr_internal *addrhint);
bool fromwire_connectd_connect_to_peer(const tal_t *ctx, const void *p, struct node_id *id, u32 *seconds_waited, struct wireaddr_internal **addrhint);

/* WIRE: CONNECTD_CONNECT_FAILED */
/*  Connectd->master: connect failed. */
u8 *towire_connectd_connect_failed(const tal_t *ctx, const struct node_id *id, errcode_t failcode, const wirestring *failreason, u32 seconds_to_delay, const struct wireaddr_internal *addrhint);
bool fromwire_connectd_connect_failed(const tal_t *ctx, const void *p, struct node_id *id, errcode_t *failcode, wirestring **failreason, u32 *seconds_to_delay, struct wireaddr_internal **addrhint);

/* WIRE: CONNECTD_PEER_CONNECTED */
/*  Connectd -> master: we got a peer. Three fds: peer */
u8 *towire_connectd_peer_connected(const tal_t *ctx, const struct node_id *id, const struct wireaddr_internal *addr, const struct per_peer_state *pps, const u8 *features);
bool fromwire_connectd_peer_connected(const tal_t *ctx, const void *p, struct node_id *id, struct wireaddr_internal *addr, struct per_peer_state **pps, u8 **features);

/* WIRE: CONNECTD_PEER_DISCONNECTED */
/*  master -> connectd: peer has disconnected. */
u8 *towire_connectd_peer_disconnected(const tal_t *ctx, const struct node_id *id);
bool fromwire_connectd_peer_disconnected(const void *p, struct node_id *id);

/* WIRE: CONNECTD_DEV_MEMLEAK */
/*  master -> connectd: do you have a memleak? */
u8 *towire_connectd_dev_memleak(const tal_t *ctx);
bool fromwire_connectd_dev_memleak(const void *p);

/* WIRE: CONNECTD_DEV_MEMLEAK_REPLY */
u8 *towire_connectd_dev_memleak_reply(const tal_t *ctx, bool leak);
bool fromwire_connectd_dev_memleak_reply(const void *p, bool *leak);


#endif /* LIGHTNING_CONNECTD_CONNECTD_WIREGEN_H */
// SHA256STAMP:ae6ae82d79526abc039896e90f677549a6acb73c0a2cd171f6658849025318b5
