#include "config.h"

#include <ccan/mem/mem.h>
#include <common/base32.c>
#include <common/base64.c>
#include <common/setup.h>
#include <common/utils.h>
#include <tests/fuzz/libfuzz.h>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE;
		char *encoded;
		uint8_t *decoded;
		/* This may require more bytes than actually are decoded! */
		char decodebuf[fuzzsize + 10];

		encoded = b32_encode(tmpctx, fuzzbuf, fuzzsize);
		decoded = b32_decode(tmpctx, encoded, strlen(encoded));
		assert(memeq(decoded, tal_bytelen(decoded),
			     fuzzbuf, fuzzsize));

		encoded = b64_encode(tmpctx, fuzzbuf, fuzzsize);
		assert(base64_decode(decodebuf, sizeof(decodebuf),
				     encoded, strlen(encoded)) == fuzzsize);
		assert(memcmp(decoded, fuzzbuf, fuzzsize) == 0);

		clean_tmpctx();
	}

	fuzz_shutdown();
}
