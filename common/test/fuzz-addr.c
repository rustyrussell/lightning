#include "config.h"

#include <bitcoin/base58.c>
#include <bitcoin/chainparams.c>
#include <bitcoin/psbt.c>
#include <bitcoin/pubkey.c>
#include <bitcoin/script.c>
#include <bitcoin/shadouble.c>
#include <bitcoin/tx.c>
#include <bitcoin/varint.c>
#include <common/addr.c>
#include <common/amount.c>
#include <common/bech32.c>
#include <common/setup.h>
#include <common/utils.h>
#include <tests/fuzz/libfuzz.h>
#include <tests/fuzz/libfuzz-signature.c>
#include <wire/fromwire.c>
#include <wire/towire.c>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf;

	fuzz_setup(argv[0]);
	/* FIXME: Randomize params. */
	chainparams = chainparams_for_network("bitcoin");

	fuzzbuf = FUZZ_SETUP_BUFFER(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE;
		/* FIXME! Put len in encode_scriptpubkey_to_addr() fn */
		u8 *inp = tal_dup_arr(tmpctx, u8, fuzzbuf, fuzzsize, 0);
		encode_scriptpubkey_to_addr(tmpctx, chainparams, inp);
		clean_tmpctx();
	}
	fuzz_shutdown();
}
