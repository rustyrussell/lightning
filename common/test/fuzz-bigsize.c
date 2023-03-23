#include "config.h"

#include <common/bigsize.c>
#include <tests/fuzz/libfuzz.h>
#include <wire/fromwire.c>
#include <wire/towire.c>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE;
		uint8_t *wire_buff, buff[BIGSIZE_MAX_LEN];
		const uint8_t **wire_chunks, *wire_ptr;
		size_t wire_max;

		wire_chunks = fuzz_input_chunks(tmpctx, fuzzbuf, fuzzsize, 8);
		for (size_t i = 0; i < tal_count(wire_chunks); i++) {
			wire_max = tal_count(wire_chunks[i]);
			wire_ptr = wire_chunks[i];

			bigsize_t bs = fromwire_bigsize(&wire_ptr, &wire_max);
			if (bs != 0) {
				/* We have a valid bigsize type, now we should not error. */
				assert(bigsize_put(buff, bs) > 0);
				assert(bigsize_len(bs));

				wire_buff = tal_arr(tmpctx, uint8_t, 8);
				towire_bigsize(&wire_buff, bs);
			}
		}
		clean_tmpctx();
	}

	fuzz_shutdown();
}
