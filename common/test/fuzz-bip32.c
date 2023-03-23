#include "config.h"

#include <common/bip32.c>
#include <tests/fuzz/libfuzz.h>
#include <wally_bip32.h>
#include <wire/fromwire.c>
#include <wire/towire.c>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE;
		struct ext_key xkey;
		struct bip32_key_version version;
		u8 *wire_buff;
		const uint8_t **xkey_chunks, **ver_chunks, *wire_ptr;
		size_t wire_max;

		if (fuzzsize < BIP32_SERIALIZED_LEN)
			continue;

		xkey_chunks = fuzz_input_chunks(tmpctx, fuzzbuf, fuzzsize,
						BIP32_SERIALIZED_LEN);
		for (size_t i = 0; i < tal_count(xkey_chunks); i++) {
			wire_max = tal_bytelen(xkey_chunks[i]);
			wire_ptr = xkey_chunks[i];

			fromwire_ext_key(&wire_ptr, &wire_max, &xkey);
			if (wire_ptr) {
				wire_buff = tal_arr(tmpctx, uint8_t, 0);
				towire_ext_key(&wire_buff, &xkey);
				assert(memeq(wire_buff, tal_bytelen(wire_buff),
					     xkey_chunks[i],
					     tal_bytelen(xkey_chunks[i])));
			}
		}

		ver_chunks = fuzz_input_chunks(tmpctx, fuzzbuf, fuzzsize, 4);
		for (size_t i = 0; i < tal_count(ver_chunks); i++) {
			wire_max = tal_bytelen(ver_chunks[i]);
			wire_ptr = ver_chunks[i];

			fromwire_bip32_key_version(&wire_ptr, &wire_max, &version);
			if (wire_ptr) {
				wire_buff = tal_arr(tmpctx, uint8_t, 0);
				towire_bip32_key_version(&wire_buff, &version);
				assert(memeq(wire_buff, tal_bytelen(wire_buff),
					     ver_chunks[i],
					     tal_bytelen(ver_chunks[i])));
			}
		}
		clean_tmpctx();
	}
	fuzz_shutdown();
}
