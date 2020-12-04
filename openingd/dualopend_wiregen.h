/* This file was generated by generate-wire.py */
/* Do not modify this file! Modify the _csv file it was generated from. */
/* Original template can be found at tools/gen/header_template */

#ifndef LIGHTNING_OPENINGD_DUALOPEND_WIREGEN_H
#define LIGHTNING_OPENINGD_DUALOPEND_WIREGEN_H
#include <ccan/tal/tal.h>
#include <wire/tlvstream.h>
#include <wire/wire.h>
#include <bitcoin/chainparams.h>
#include <bitcoin/psbt.h>
#include <common/cryptomsg.h>
#include <common/channel_config.h>
#include <common/channel_id.h>
#include <common/derive_basepoints.h>
#include <common/features.h>
#include <common/penalty_base.h>
#include <common/per_peer_state.h>

enum dualopend_wire {
        WIRE_DUALOPEND_INIT = 7000,
        /*  dualopend->master: they offered channel */
        WIRE_DUALOPEND_GOT_OFFER = 7005,
        /*  master->dualopend: reply back with our first funding info/contribs */
        WIRE_DUALOPEND_GOT_OFFER_REPLY = 7105,
        /*  dualopend->master: ready to commit channel open to database and */
        /*                     get some signatures for the funding_tx. */
        WIRE_DUALOPEND_COMMIT_RCVD = 7007,
        /*  dualopend->master: peer updated the psbt */
        WIRE_DUALOPEND_PSBT_CHANGED = 7107,
        /*  master->dualopend: we updated the psbt */
        WIRE_DUALOPEND_PSBT_UPDATED = 7108,
        /*  master->dualopend: fail this channel open */
        WIRE_DUALOPEND_FAIL = 7003,
        /*  dualopend->master: we failed to negotiate channel */
        WIRE_DUALOPEND_FAILED = 7004,
        /*  master->dualopend: hello */
        WIRE_DUALOPEND_OPENER_INIT = 7200,
        /*  master -> dualopend: do you have a memleak? */
        WIRE_DUALOPEND_DEV_MEMLEAK = 7033,
        WIRE_DUALOPEND_DEV_MEMLEAK_REPLY = 7133,
};

const char *dualopend_wire_name(int e);

/**
 * Determine whether a given message type is defined as a message.
 *
 * Returns true if the message type is part of the message definitions we have
 * generated parsers for, false if it is a custom message that cannot be
 * handled internally.
 */
bool dualopend_wire_is_defined(u16 type);


/* WIRE: DUALOPEND_INIT */
u8 *towire_dualopend_init(const tal_t *ctx, const struct chainparams *chainparams, const struct feature_set *our_feature_set, const u8 *their_init_features, const struct channel_config *our_config, u32 max_to_self_delay, struct amount_msat min_effective_htlc_capacity_msat, const struct per_peer_state *pps, const struct basepoints *our_basepoints, const struct pubkey *our_funding_pubkey, u32 minimum_depth, u32 min_feerate, u32 max_feerate, const u8 *msg);
bool fromwire_dualopend_init(const tal_t *ctx, const void *p, const struct chainparams **chainparams, struct feature_set **our_feature_set, u8 **their_init_features, struct channel_config *our_config, u32 *max_to_self_delay, struct amount_msat *min_effective_htlc_capacity_msat, struct per_peer_state **pps, struct basepoints *our_basepoints, struct pubkey *our_funding_pubkey, u32 *minimum_depth, u32 *min_feerate, u32 *max_feerate, u8 **msg);

/* WIRE: DUALOPEND_GOT_OFFER */
/*  dualopend->master: they offered channel */
u8 *towire_dualopend_got_offer(const tal_t *ctx, struct amount_sat opener_funding, struct amount_sat dust_limit_satoshis, struct amount_msat max_htlc_value_in_flight_msat, struct amount_msat htlc_minimum_msat, u32 feerate_funding_max, u32 feerate_funding_min, u32 feerate_funding_best, u32 feerate_per_kw, u16 to_self_delay, u16 max_accepted_htlcs, u8 channel_flags, u32 locktime, const u8 *shutdown_scriptpubkey);
bool fromwire_dualopend_got_offer(const tal_t *ctx, const void *p, struct amount_sat *opener_funding, struct amount_sat *dust_limit_satoshis, struct amount_msat *max_htlc_value_in_flight_msat, struct amount_msat *htlc_minimum_msat, u32 *feerate_funding_max, u32 *feerate_funding_min, u32 *feerate_funding_best, u32 *feerate_per_kw, u16 *to_self_delay, u16 *max_accepted_htlcs, u8 *channel_flags, u32 *locktime, u8 **shutdown_scriptpubkey);

/* WIRE: DUALOPEND_GOT_OFFER_REPLY */
/*  master->dualopend: reply back with our first funding info/contribs */
u8 *towire_dualopend_got_offer_reply(const tal_t *ctx, struct amount_sat accepter_funding, u32 feerate_funding, const struct wally_psbt *psbt, const u8 *our_shutdown_scriptpubkey);
bool fromwire_dualopend_got_offer_reply(const tal_t *ctx, const void *p, struct amount_sat *accepter_funding, u32 *feerate_funding, struct wally_psbt **psbt, u8 **our_shutdown_scriptpubkey);

/* WIRE: DUALOPEND_COMMIT_RCVD */
/*  dualopend->master: ready to commit channel open to database and */
/*                     get some signatures for the funding_tx. */
u8 *towire_dualopend_commit_rcvd(const tal_t *ctx, const struct channel_config *their_config, const struct bitcoin_tx *remote_first_commit, const struct penalty_base *pbase, const struct bitcoin_signature *first_commit_sig, const struct wally_psbt *psbt, const struct channel_id *channel_id, const struct per_peer_state *pps, const struct pubkey *revocation_basepoint, const struct pubkey *payment_basepoint, const struct pubkey *htlc_basepoint, const struct pubkey *delayed_payment_basepoint, const struct pubkey *their_per_commit_point, const struct pubkey *remote_fundingkey, const struct bitcoin_txid *funding_txid, u16 funding_txout, struct amount_sat funding_satoshis, struct amount_sat our_funding_sats, u8 channel_flags, u32 feerate_per_kw, const u8 *commitment_msg, struct amount_sat our_channel_reserve_satoshis, const u8 *local_shutdown_scriptpubkey, const u8 *remote_shutdown_scriptpubkey);
bool fromwire_dualopend_commit_rcvd(const tal_t *ctx, const void *p, struct channel_config *their_config, struct bitcoin_tx **remote_first_commit, struct penalty_base **pbase, struct bitcoin_signature *first_commit_sig, struct wally_psbt **psbt, struct channel_id *channel_id, struct per_peer_state **pps, struct pubkey *revocation_basepoint, struct pubkey *payment_basepoint, struct pubkey *htlc_basepoint, struct pubkey *delayed_payment_basepoint, struct pubkey *their_per_commit_point, struct pubkey *remote_fundingkey, struct bitcoin_txid *funding_txid, u16 *funding_txout, struct amount_sat *funding_satoshis, struct amount_sat *our_funding_sats, u8 *channel_flags, u32 *feerate_per_kw, u8 **commitment_msg, struct amount_sat *our_channel_reserve_satoshis, u8 **local_shutdown_scriptpubkey, u8 **remote_shutdown_scriptpubkey);

/* WIRE: DUALOPEND_PSBT_CHANGED */
/*  dualopend->master: peer updated the psbt */
u8 *towire_dualopend_psbt_changed(const tal_t *ctx, const struct channel_id *channel_id, u64 funding_serial, const struct wally_psbt *psbt);
bool fromwire_dualopend_psbt_changed(const tal_t *ctx, const void *p, struct channel_id *channel_id, u64 *funding_serial, struct wally_psbt **psbt);

/* WIRE: DUALOPEND_PSBT_UPDATED */
/*  master->dualopend: we updated the psbt */
u8 *towire_dualopend_psbt_updated(const tal_t *ctx, const struct wally_psbt *psbt);
bool fromwire_dualopend_psbt_updated(const tal_t *ctx, const void *p, struct wally_psbt **psbt);

/* WIRE: DUALOPEND_FAIL */
/*  master->dualopend: fail this channel open */
u8 *towire_dualopend_fail(const tal_t *ctx, const wirestring *reason);
bool fromwire_dualopend_fail(const tal_t *ctx, const void *p, wirestring **reason);

/* WIRE: DUALOPEND_FAILED */
/*  dualopend->master: we failed to negotiate channel */
u8 *towire_dualopend_failed(const tal_t *ctx, const wirestring *reason);
bool fromwire_dualopend_failed(const tal_t *ctx, const void *p, wirestring **reason);

/* WIRE: DUALOPEND_OPENER_INIT */
/*  master->dualopend: hello */
u8 *towire_dualopend_opener_init(const tal_t *ctx, const struct wally_psbt *psbt, struct amount_sat funding_amount, const u8 *local_shutdown_scriptpubkey, u32 feerate_per_kw, u32 feerate_per_kw_funding, u8 channel_flags);
bool fromwire_dualopend_opener_init(const tal_t *ctx, const void *p, struct wally_psbt **psbt, struct amount_sat *funding_amount, u8 **local_shutdown_scriptpubkey, u32 *feerate_per_kw, u32 *feerate_per_kw_funding, u8 *channel_flags);

/* WIRE: DUALOPEND_DEV_MEMLEAK */
/*  master -> dualopend: do you have a memleak? */
u8 *towire_dualopend_dev_memleak(const tal_t *ctx);
bool fromwire_dualopend_dev_memleak(const void *p);

/* WIRE: DUALOPEND_DEV_MEMLEAK_REPLY */
u8 *towire_dualopend_dev_memleak_reply(const tal_t *ctx, bool leak);
bool fromwire_dualopend_dev_memleak_reply(const void *p, bool *leak);


#endif /* LIGHTNING_OPENINGD_DUALOPEND_WIREGEN_H */
// SHA256STAMP:48dcbf79652c7cde9ad129b137cc60be72be6232ed20a8c2675a00f427ab6f91
