#include "config.h"
#include <bitcoin/base58.c>
#include <bitcoin/chainparams.c>
#include <bitcoin/psbt.c>
#include <bitcoin/pubkey.c>
#include <bitcoin/script.c>
#include <bitcoin/shadouble.c>
#include <bitcoin/tx.c>
#include <bitcoin/varint.c>

#include <common/amount.c>
#include <common/channel_id.c>
#include <tests/fuzz/libfuzz.h>
#include <tests/fuzz/libfuzz-signature.c>
#include <wire/fromwire.c>
#include <wire/towire.c>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE;
		struct channel_id chan_id;
		struct pubkey basepoint_1, basepoint_2;
		struct bitcoin_outpoint outpoint;
		const uint8_t **v1_chunks, **v2_chunks, **marshal_chunks;
		const uint8_t *wire_ptr;
		size_t wire_max;
		uint8_t *wire_buf;

		/* 32 (txid) + 4 (vout) */
		if (fuzzsize < 36)
			continue;

		v1_chunks = fuzz_input_chunks(NULL, fuzzbuf, fuzzsize, 36);
		for (size_t i = 0; i < tal_count(v1_chunks); i++) {
			wire_ptr = v1_chunks[i];
			wire_max = 36;
			fromwire_bitcoin_outpoint(&wire_ptr, &wire_max, &outpoint);
			assert(wire_ptr);
			derive_channel_id(&chan_id, &outpoint);
		}
		tal_free(v1_chunks);

		v2_chunks = fuzz_input_chunks(NULL, fuzzbuf, fuzzsize,
					    PUBKEY_CMPR_LEN * 2);
		for (size_t i = 0; i < tal_count(v2_chunks); i++) {
			wire_ptr = v2_chunks[i];
			wire_max = PUBKEY_CMPR_LEN;
			fromwire_pubkey(&wire_ptr, &wire_max, &basepoint_1);
			if (!wire_ptr)
				continue;

			wire_max = PUBKEY_CMPR_LEN;
			fromwire_pubkey(&wire_ptr, &wire_max, &basepoint_2);
			if (!wire_ptr)
				continue;

			derive_channel_id_v2(&chan_id, &basepoint_1, &basepoint_2);
		}
		tal_free(v2_chunks);

		marshal_chunks = fuzz_input_chunks(NULL, fuzzbuf, fuzzsize, 32);
		for (size_t i = 0; i < tal_count(marshal_chunks); i++) {
			wire_ptr = marshal_chunks[i];
			wire_max = tal_count(marshal_chunks[i]);
			fromwire_channel_id(&wire_ptr, &wire_max, &chan_id);
			wire_buf = tal_arr(NULL, uint8_t, tal_count(marshal_chunks[i]));
			towire_channel_id(&wire_buf, &chan_id);
			tal_free(wire_buf);
		}
		tal_free(marshal_chunks);
	}
	fuzz_shutdown();
}
