#include "config.h"

#include <assert.h>
#include <bitcoin/base58.c>
#include <bitcoin/block.c>
#include <bitcoin/chainparams.c>
#include <bitcoin/preimage.c>
#include <bitcoin/privkey.c>
#include <bitcoin/psbt.c>
#include <bitcoin/pubkey.c>
#include <bitcoin/script.c>
#include <bitcoin/shadouble.c>
#include <bitcoin/short_channel_id.c>
#include <bitcoin/tx.c>
#include <bitcoin/varint.c>
#include <ccan/tal/tal.h>
#include <common/amount.c>
#include <common/bigsize.c>
#include <common/blockheight_states.c>
#include <common/channel_config.c>
#include <common/channel_id.c>
#include <common/channel_type.c>
#include <common/daemon.c>
#include <common/daemon_conn.c>
#include <common/derive_basepoints.c>
#include <common/features.c>
#include <common/fee_states.c>
#include <common/htlc_state.c>
#include <common/initial_channel.c>
#include <common/initial_commit_tx.c>
#include <common/key_derive.c>
#include <common/keyset.c>
#include <common/memleak.c>
#include <common/msg_queue.c>
#include <common/node_id.c>
#include <common/permute_tx.c>
#include <common/setup.h>
#include <common/status.c>
#include <common/status_wire.c>
#include <common/status_wiregen.c>
#include <common/type_to_string.c>
#include <common/utils.h>
#include <common/version.c>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tests/fuzz/libfuzz.h>
#include <tests/fuzz/libfuzz-signature.c>
#include <wire/fromwire.c>
#include <wire/peer_wire.c>
#if EXPERIMENTAL_FEATURES
 #include <wire/peer_exp_wiregen.c>
#else
 #include <wire/peer_wiregen.c>
#endif
#include <wire/tlvstream.c>
#include <wire/towire.c>
#include <wire/wire_io.c>
#include <wire/wire_sync.c>

void init(int *argc, char ***argv)
{
	common_setup("fuzzer");
	int devnull = open("/dev/null", O_WRONLY);
	status_setup_sync(devnull);
	chainparams = chainparams_for_network("bitcoin");
}

void run(const uint8_t *data, size_t size)
{
	struct channel_id cid;
	struct bitcoin_outpoint funding;
	u32 minimum_depth;
	struct amount_sat funding_sats, max;
	struct amount_msat local_msatoshi;
	u32 feerate_per_kw, blockheight, lease_expiry;
	struct channel_config local, remote;
	struct basepoints local_basepoints, remote_basepoints;
	struct pubkey local_funding_pubkey, remote_funding_pubkey;
	bool option_static_remotekey, option_anchor_outputs, wumbo;
	struct channel_type *channel_type;
	struct channel *channel;

	fromwire_channel_id(&data, &size, &cid);
	fromwire_bitcoin_outpoint(&data, &size, &funding);
	minimum_depth = fromwire_u32(&data, &size);
	funding_sats = fromwire_amount_sat(&data, &size);
	local_msatoshi = fromwire_amount_msat(&data, &size);
	max = AMOUNT_SAT((u32)WALLY_SATOSHI_PER_BTC * WALLY_BTC_MAX);
	if (amount_sat_greater(funding_sats, max))
		funding_sats = max;
	feerate_per_kw = fromwire_u32(&data, &size);
	blockheight = fromwire_u32(&data, &size);
	lease_expiry = fromwire_u32(&data, &size);
	fromwire_channel_config(&data, &size, &local);
	fromwire_channel_config(&data, &size, &remote);
	fromwire_basepoints(&data, &size, &local_basepoints);
	fromwire_basepoints(&data, &size, &remote_basepoints);
	fromwire_pubkey(&data, &size, &local_funding_pubkey);
	fromwire_pubkey(&data, &size, &remote_funding_pubkey);
	wumbo = fromwire_bool(&data, &size);
	option_anchor_outputs = fromwire_bool(&data, &size);
	option_static_remotekey = option_anchor_outputs || fromwire_bool(&data, &size);

	if (option_anchor_outputs)
		channel_type = channel_type_anchor_outputs(tmpctx);
	else if (option_static_remotekey)
		channel_type = channel_type_static_remotekey(tmpctx);
	else
		channel_type = channel_type_none(tmpctx);

	/* TODO: determine if it makes sense to check at each step for libfuzzer
	 * to deduce pertinent inputs */
	if (!data || !size)
		return;

	for (enum side opener = 0; opener < NUM_SIDES; opener++) {
		channel = new_initial_channel(tmpctx, &cid, &funding,
					      minimum_depth,
					      take(new_height_states(NULL, opener,
								     &blockheight)),
					      lease_expiry,
					      funding_sats, local_msatoshi,
					      take(new_fee_states(NULL, opener,
								  &feerate_per_kw)),
					      &local, &remote,
					      &local_basepoints,
					      &remote_basepoints,
					      &local_funding_pubkey,
					      &remote_funding_pubkey,
					      channel_type,
					      wumbo, opener);

		/* TODO: make initial_channel_tx() work with ASAN.. */
		(void)channel;
	}

	clean_tmpctx();
}
