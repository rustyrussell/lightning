#include "config.h"

#include <common/descriptor_checksum.c>
#include <tests/fuzz/libfuzz.h>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE;
		struct descriptor_checksum checksum;

		/* We should not crash nor overflow the checksum buffer. */
		descriptor_checksum((const char *)fuzzbuf,
				    fuzzsize, &checksum);
	}
	fuzz_shutdown();
}
