/* This file was generated by generate-wire.py */
/* Do not modify this file! Modify the _csv file it was generated from. */
/* Original template can be found at tools/gen/header_template */

#ifndef LIGHTNING_OPENINGD_OPENINGD_WIREGEN_H
#define LIGHTNING_OPENINGD_OPENINGD_WIREGEN_H
#include <ccan/tal/tal.h>
#include <wire/tlvstream.h>
#include <wire/wire.h>
#include <bitcoin/chainparams.h>
#include <common/cryptomsg.h>
#include <common/channel_config.h>
#include <common/channel_id.h>
#include <common/derive_basepoints.h>
#include <common/features.h>
#include <common/per_peer_state.h>
#include <common/penalty_base.h>

enum openingd_wire {
        WIRE_OPENINGD_INIT = 6000,
        /*  Openingd->master: they offered channel */
        WIRE_OPENINGD_GOT_OFFER = 6005,
        /*  master->openingd: optional rejection message */
        WIRE_OPENINGD_GOT_OFFER_REPLY = 6105,
        /*  Openingd->master: we've successfully offered channel. */
        /*  This gives their sig */
        WIRE_OPENINGD_FUNDER_REPLY = 6101,
        /*  master->openingd: start channel establishment for a funding tx */
        WIRE_OPENINGD_FUNDER_START = 6002,
        /*  openingd->master: send back output script for 2-of-2 funding output */
        WIRE_OPENINGD_FUNDER_START_REPLY = 6102,
        /*  master->openingd: complete channel establishment for a funding */
        /*  tx that will be paid for by an external wallet */
        /*  response to this is a normal `openingd_funder_reply` ?? */
        WIRE_OPENINGD_FUNDER_COMPLETE = 6012,
        /* master->openingd: cancel channel establishment for a funding */
        WIRE_OPENINGD_FUNDER_CANCEL = 6013,
        /*  Openingd->master: we failed to negotiation channel */
        WIRE_OPENINGD_FUNDER_FAILED = 6004,
        /*  Openingd->master: they offered channel. */
        /*  This gives their txid and info */
        WIRE_OPENINGD_FUNDEE = 6003,
        /*  master -> openingd: do you have a memleak? */
        WIRE_OPENINGD_DEV_MEMLEAK = 6033,
        WIRE_OPENINGD_DEV_MEMLEAK_REPLY = 6133,
};

const char *openingd_wire_name(int e);

/**
 * Determine whether a given message type is defined as a message.
 *
 * Returns true if the message type is part of the message definitions we have
 * generated parsers for, false if it is a custom message that cannot be
 * handled internally.
 */
bool openingd_wire_is_defined(u16 type);


/* WIRE: OPENINGD_INIT */
u8 *towire_openingd_init(const tal_t *ctx, const struct chainparams *chainparams, const struct feature_set *our_features, const struct channel_config *our_config, u32 max_to_self_delay, struct amount_msat min_effective_htlc_capacity_msat, const struct per_peer_state *pps, const struct basepoints *our_basepoints, const struct pubkey *our_funding_pubkey, u32 minimum_depth, u32 min_feerate, u32 max_feerate, const u8 *lfeatures, bool option_static_remotekey, bool option_anchor_outputs, const u8 *msg, const struct channel_id *dev_temporary_channel_id, bool dev_fast_gossip);
bool fromwire_openingd_init(const tal_t *ctx, const void *p, const struct chainparams **chainparams, struct feature_set **our_features, struct channel_config *our_config, u32 *max_to_self_delay, struct amount_msat *min_effective_htlc_capacity_msat, struct per_peer_state **pps, struct basepoints *our_basepoints, struct pubkey *our_funding_pubkey, u32 *minimum_depth, u32 *min_feerate, u32 *max_feerate, u8 **lfeatures, bool *option_static_remotekey, bool *option_anchor_outputs, u8 **msg, struct channel_id **dev_temporary_channel_id, bool *dev_fast_gossip);

/* WIRE: OPENINGD_GOT_OFFER */
/*  Openingd->master: they offered channel */
u8 *towire_openingd_got_offer(const tal_t *ctx, struct amount_sat funding_satoshis, struct amount_msat push_msat, struct amount_sat dust_limit_satoshis, struct amount_msat max_htlc_value_in_flight_msat, struct amount_sat channel_reserve_satoshis, struct amount_msat htlc_minimum_msat, u32 feerate_per_kw, u16 to_self_delay, u16 max_accepted_htlcs, u8 channel_flags, const u8 *shutdown_scriptpubkey);
bool fromwire_openingd_got_offer(const tal_t *ctx, const void *p, struct amount_sat *funding_satoshis, struct amount_msat *push_msat, struct amount_sat *dust_limit_satoshis, struct amount_msat *max_htlc_value_in_flight_msat, struct amount_sat *channel_reserve_satoshis, struct amount_msat *htlc_minimum_msat, u32 *feerate_per_kw, u16 *to_self_delay, u16 *max_accepted_htlcs, u8 *channel_flags, u8 **shutdown_scriptpubkey);

/* WIRE: OPENINGD_GOT_OFFER_REPLY */
/*  master->openingd: optional rejection message */
u8 *towire_openingd_got_offer_reply(const tal_t *ctx, const wirestring *rejection, const u8 *our_shutdown_scriptpubkey);
bool fromwire_openingd_got_offer_reply(const tal_t *ctx, const void *p, wirestring **rejection, u8 **our_shutdown_scriptpubkey);

/* WIRE: OPENINGD_FUNDER_REPLY */
/*  Openingd->master: we've successfully offered channel. */
/*  This gives their sig */
u8 *towire_openingd_funder_reply(const tal_t *ctx, const struct channel_config *their_config, const struct bitcoin_tx *first_commit, const struct penalty_base *pbase, const struct bitcoin_signature *first_commit_sig, const struct per_peer_state *pps, const struct pubkey *revocation_basepoint, const struct pubkey *payment_basepoint, const struct pubkey *htlc_basepoint, const struct pubkey *delayed_payment_basepoint, const struct pubkey *their_per_commit_point, u32 minimum_depth, const struct pubkey *remote_fundingkey, const struct bitcoin_txid *funding_txid, u16 funding_txout, u32 feerate_per_kw, struct amount_sat our_channel_reserve_satoshis, const u8 *shutdown_scriptpubkey);
bool fromwire_openingd_funder_reply(const tal_t *ctx, const void *p, struct channel_config *their_config, struct bitcoin_tx **first_commit, struct penalty_base **pbase, struct bitcoin_signature *first_commit_sig, struct per_peer_state **pps, struct pubkey *revocation_basepoint, struct pubkey *payment_basepoint, struct pubkey *htlc_basepoint, struct pubkey *delayed_payment_basepoint, struct pubkey *their_per_commit_point, u32 *minimum_depth, struct pubkey *remote_fundingkey, struct bitcoin_txid *funding_txid, u16 *funding_txout, u32 *feerate_per_kw, struct amount_sat *our_channel_reserve_satoshis, u8 **shutdown_scriptpubkey);

/* WIRE: OPENINGD_FUNDER_START */
/*  master->openingd: start channel establishment for a funding tx */
u8 *towire_openingd_funder_start(const tal_t *ctx, struct amount_sat funding_satoshis, struct amount_msat push_msat, const u8 *upfront_shutdown_script, u32 feerate_per_kw, u8 channel_flags);
bool fromwire_openingd_funder_start(const tal_t *ctx, const void *p, struct amount_sat *funding_satoshis, struct amount_msat *push_msat, u8 **upfront_shutdown_script, u32 *feerate_per_kw, u8 *channel_flags);

/* WIRE: OPENINGD_FUNDER_START_REPLY */
/*  openingd->master: send back output script for 2-of-2 funding output */
u8 *towire_openingd_funder_start_reply(const tal_t *ctx, const u8 *scriptpubkey, bool upfront_shutdown_negotiated);
bool fromwire_openingd_funder_start_reply(const tal_t *ctx, const void *p, u8 **scriptpubkey, bool *upfront_shutdown_negotiated);

/* WIRE: OPENINGD_FUNDER_COMPLETE */
/*  master->openingd: complete channel establishment for a funding */
/*  tx that will be paid for by an external wallet */
/*  response to this is a normal `openingd_funder_reply` ?? */
u8 *towire_openingd_funder_complete(const tal_t *ctx, const struct bitcoin_txid *funding_txid, u16 funding_txout);
bool fromwire_openingd_funder_complete(const void *p, struct bitcoin_txid *funding_txid, u16 *funding_txout);

/* WIRE: OPENINGD_FUNDER_CANCEL */
/* master->openingd: cancel channel establishment for a funding */
u8 *towire_openingd_funder_cancel(const tal_t *ctx);
bool fromwire_openingd_funder_cancel(const void *p);

/* WIRE: OPENINGD_FUNDER_FAILED */
/*  Openingd->master: we failed to negotiation channel */
u8 *towire_openingd_funder_failed(const tal_t *ctx, const wirestring *reason);
bool fromwire_openingd_funder_failed(const tal_t *ctx, const void *p, wirestring **reason);

/* WIRE: OPENINGD_FUNDEE */
/*  Openingd->master: they offered channel. */
/*  This gives their txid and info */
u8 *towire_openingd_fundee(const tal_t *ctx, const struct channel_config *their_config, const struct bitcoin_tx *first_commit, const struct penalty_base *pbase, const struct bitcoin_signature *first_commit_sig, const struct per_peer_state *pps, const struct pubkey *revocation_basepoint, const struct pubkey *payment_basepoint, const struct pubkey *htlc_basepoint, const struct pubkey *delayed_payment_basepoint, const struct pubkey *their_per_commit_point, const struct pubkey *remote_fundingkey, const struct bitcoin_txid *funding_txid, u16 funding_txout, struct amount_sat funding_satoshis, struct amount_msat push_msat, u8 channel_flags, u32 feerate_per_kw, const u8 *funding_signed_msg, struct amount_sat our_channel_reserve_satoshis, const u8 *local_shutdown_scriptpubkey, const u8 *remote_shutdown_scriptpubkey);
bool fromwire_openingd_fundee(const tal_t *ctx, const void *p, struct channel_config *their_config, struct bitcoin_tx **first_commit, struct penalty_base **pbase, struct bitcoin_signature *first_commit_sig, struct per_peer_state **pps, struct pubkey *revocation_basepoint, struct pubkey *payment_basepoint, struct pubkey *htlc_basepoint, struct pubkey *delayed_payment_basepoint, struct pubkey *their_per_commit_point, struct pubkey *remote_fundingkey, struct bitcoin_txid *funding_txid, u16 *funding_txout, struct amount_sat *funding_satoshis, struct amount_msat *push_msat, u8 *channel_flags, u32 *feerate_per_kw, u8 **funding_signed_msg, struct amount_sat *our_channel_reserve_satoshis, u8 **local_shutdown_scriptpubkey, u8 **remote_shutdown_scriptpubkey);

/* WIRE: OPENINGD_DEV_MEMLEAK */
/*  master -> openingd: do you have a memleak? */
u8 *towire_openingd_dev_memleak(const tal_t *ctx);
bool fromwire_openingd_dev_memleak(const void *p);

/* WIRE: OPENINGD_DEV_MEMLEAK_REPLY */
u8 *towire_openingd_dev_memleak_reply(const tal_t *ctx, bool leak);
bool fromwire_openingd_dev_memleak_reply(const void *p, bool *leak);


#endif /* LIGHTNING_OPENINGD_OPENINGD_WIREGEN_H */
// SHA256STAMP:9c977c121d6ff95a6680804710457cf353ee1bb0686a4e51045017b7590fb428
