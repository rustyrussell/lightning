#include "config.h"

#include <common/bech32.c>
#include <common/setup.h>
#include <stdint.h>
#include <string.h>
#include <tests/fuzz/libfuzz.h>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE;
		const char hrp_inv[5] = "lnbc\0", hrp_addr[3] = "bc\0";
		char *bech32_str, *hrp_out, *addr;
		uint8_t *data_out;
		size_t data_out_len;
		int wit_version;
		bech32_encoding benc;

		if (fuzzsize < 1)
			continue;

		/* Buffer size is defined in each function's doc comment. */
		bech32_str = malloc(fuzzsize-1 + strlen(hrp_inv) + 8);
		benc = fuzzbuf[0] ? BECH32_ENCODING_BECH32 : BECH32_ENCODING_BECH32M;
		/* Fails if any bytes in fuzzbuf are > 32! */
		if (bech32_encode(bech32_str, hrp_inv, fuzzbuf+1, fuzzsize-1, fuzzsize-1 + strlen(hrp_inv) + 8, benc) == 1) {
			hrp_out = malloc(strlen(bech32_str) - 6);
			data_out = malloc(strlen(bech32_str) - 8);
			assert(bech32_decode(hrp_out, data_out, &data_out_len, bech32_str, fuzzsize) == benc);
			free(hrp_out);
			free(data_out);
		}
		free(bech32_str);

		data_out = malloc(fuzzsize);

		/* This is also used as part of sign and check message. */
		data_out_len = 0;
		bech32_convert_bits(data_out, &data_out_len, 8, fuzzbuf, fuzzsize, 5, 1);
		data_out_len = 0;
		bech32_convert_bits(data_out, &data_out_len, 8, fuzzbuf, fuzzsize, 5, 0);

		addr = malloc(73 + strlen(hrp_addr));
		wit_version = 0;
		if (segwit_addr_encode(addr, hrp_addr, wit_version, fuzzbuf, fuzzsize) == 1)
			segwit_addr_decode(&wit_version, data_out, &data_out_len, hrp_addr, addr);
		wit_version = 1;
		if (segwit_addr_encode(addr, hrp_addr, wit_version, fuzzbuf, fuzzsize) == 1)
			segwit_addr_decode(&wit_version, data_out, &data_out_len, hrp_addr, addr);
		free(addr);
		free(data_out);
	}

	fuzz_shutdown();
}
