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

void init(int *argc, char ***argv)
{
	chainparams = chainparams_for_network("bitcoin");
	common_setup("fuzzer");
}

void run(const uint8_t *data, size_t size)
{
	uint8_t *script_pubkey = tal_dup_arr(tmpctx, uint8_t, data, size, 0);

	encode_scriptpubkey_to_addr(tmpctx, chainparams, script_pubkey);

	clean_tmpctx();
}
