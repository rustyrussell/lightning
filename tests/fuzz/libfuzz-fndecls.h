/* We put declarations here, so libfuzz.c can see them! */
#ifndef LIGHTNING_TESTS_FUZZ_LIBFUZZ_FNDECLS_H
#define LIGHTNING_TESTS_FUZZ_LIBFUZZ_FNDECLS_H
#include "config.h"
#include <ccan/short_types/short_types.h>
#include <ccan/tal/tal.h>

void fuzz_setup(const char *argv0);
void fuzz_shutdown(void);

/* helpers to convert input. */
const u8 **fuzz_input_chunks(const void *ctx,
			     const u8 *input, size_t input_len,
			     size_t chunk_size);
char *fuzz_input_string(const tal_t *ctx, const u8 *input, size_t input_len);

#endif /* LIGHTNING_TESTS_FUZZ_LIBFUZZ_H */
