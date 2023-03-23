/* Include this ONCE in all fuzz tests! */
#ifndef LIGHTNING_TESTS_FUZZ_LIBFUZZ_H
#define LIGHTNING_TESTS_FUZZ_LIBFUZZ_H

#include "config.h"
#include "libfuzz-fndecls.h"
#include <assert.h>
#include <ccan/err/err.h>
#include <ccan/short_types/short_types.h>
#include <ccan/tal/grab_file/grab_file.h>
#include <ccan/tal/tal.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

/* Use this header like so:
 *
 * #include <tests/fuzz/libfuzz.h>
 *
 * int main(int argc, char *argv)
 * {
 *	 const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);
 *... any other setup.
 *
 *	while (FUZZ_LOOP) {
 *		size_t fuzzsize = FUZZ_SIZE;
 *		...
 *		Must return to pristine state in loop (i.e. free everything!)
 *	}
 *	fuzz_shutdown();
 * }
 *
 * This way, if run without fuzzing, it takes a single corpus on the cmdline
 * and runs once with that.  This allows for easier debugging under gdb.
 */

__AFL_FUZZ_INIT();

static size_t nonfuzz_len = -1;

static inline const u8 *nonfuzz_setup(int argc, char *argv[])
{
	u8 *buf = grab_file(NULL, argv[1]);
	assert(buf);
	nonfuzz_len = tal_bytelen(buf) - 1;
	return buf;
}

static inline bool nonfuzz_loop(void)
{
	static bool once = true;
	if (once) {
		once = false;
		return true;
	}
	return false;
}

/* Sometimes you want to do some more setup before __AFL_INIT: */
#define FUZZ_SETUP_BUFFER(argc, argv) \
	({ if (argc != 2) { __AFL_INIT(); }; (argc == 2 ? nonfuzz_setup(argc, argv) : __AFL_FUZZ_TESTCASE_BUF); })

#define FUZZ_SETUP(argc, argv) \
	(fuzz_setup(argv[0]), FUZZ_SETUP_BUFFER(argc, argv))

#define FUZZ_LOOP (nonfuzz_len == -1 ? __AFL_LOOP(1000) : nonfuzz_loop())
#define FUZZ_SIZE (nonfuzz_len == -1 ? __AFL_FUZZ_TESTCASE_LEN : nonfuzz_len)

#endif /* LIGHTNING_TESTS_FUZZ_LIBFUZZ_H */
