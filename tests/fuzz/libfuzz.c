#include "config.h"

#include <assert.h>
#include <ccan/isaac/isaac64.h>
#include <common/pseudorand.h>
#include <common/setup.h>
#include <tests/fuzz/libfuzz-fndecls.h>

/* Provide a non-random pseudo-random function to speed fuzzing. */
static isaac64_ctx isaac64;

uint64_t pseudorand(uint64_t max)
{
	assert(max);
	return isaac64_next_uint(&isaac64, max);
}

const u8 **fuzz_input_chunks(const void *ctx,
			     const u8 *input, size_t input_len,
			     size_t chunk_size)
{
	size_t n_chunks = input_len / chunk_size;
	const uint8_t **chunks = tal_arr(ctx, const uint8_t *, n_chunks);

	for (size_t i = 0; i < n_chunks; i++)
		chunks[i] = tal_dup_arr(chunks, const uint8_t,
					input + i * chunk_size, chunk_size, 0);
	return chunks;
}

char *fuzz_input_string(const tal_t *ctx, const u8 *input, size_t input_len)
{
	char *string = tal_arr(ctx, char, input_len + 1);

	for (size_t i = 0; i < input_len; i++)
		string[i] = input[i] % (CHAR_MAX + 1);
	string[input_len] = '\0';

	return string;
}

void fuzz_setup(const char *argv0)
{
	common_setup(argv0);
	isaac64_init(&isaac64, NULL, 0);
}

void fuzz_shutdown(void)
{
	common_shutdown();
}
