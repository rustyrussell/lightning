#include "config.h"

#include <common/amount.c>
#include <common/setup.h>
#include <tests/fuzz/libfuzz.h>
#include <wire/fromwire.c>
#include <wire/towire.c>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE, size = fuzzsize;
		struct amount_msat msat;
		struct amount_sat sat;
		uint8_t *buf;
		const u8 *input;
		const char *fmt_msat, *fmt_msatbtc, *fmt_sat, *fmt_satbtc;

		/* We should not crash when parsing any string. */
		parse_amount_msat(&msat, (const char *)fuzzbuf, fuzzsize);
		parse_amount_sat(&sat, (const char *)fuzzbuf, fuzzsize);

		/* Same with the wire primitives. */
		input = fuzzbuf;
		size = fuzzsize;
		buf = tal_arr(tmpctx, uint8_t, 0);
		msat = fromwire_amount_msat(&input, &size);
		towire_amount_msat(&buf, msat);
		sat = fromwire_amount_sat(&input, &size);
		towire_amount_sat(&buf, sat);

		/* Format should inconditionally produce valid amount strings according to our
		 * parser */

		fmt_msat = fmt_amount_msat(tmpctx, msat);
		fmt_msatbtc = fmt_amount_msat_btc(tmpctx, msat, true);
		assert(parse_amount_msat(&msat, fmt_msat, tal_count(fmt_msat)));
		assert(parse_amount_msat(&msat, fmt_msatbtc, tal_count(fmt_msatbtc)));

		fmt_sat = fmt_amount_sat(tmpctx, sat);
		fmt_satbtc = fmt_amount_sat_btc(tmpctx, sat, true);
		assert(parse_amount_sat(&sat, fmt_sat, tal_count(fmt_sat)));
		assert(parse_amount_sat(&sat, fmt_satbtc, tal_count(fmt_satbtc)));
		clean_tmpctx();
	}

	fuzz_shutdown();
}
