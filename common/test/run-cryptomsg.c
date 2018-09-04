#include <assert.h>
#include <ccan/str/hex/hex.h>
#include <ccan/tal/str/str.h>
#include <common/dev_disconnect.h>
#include <common/status.h>
#include <stdio.h>
#include <wire/peer_wire.h>
#include <wire/wire_io.h>

void status_fmt(enum log_level level UNUSED, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

#include "../cryptomsg.c"

static struct secret secret_from_hex(const char *hex)
{
	struct secret secret;
	hex += 2;
	if (!hex_decode(hex, strlen(hex), &secret, sizeof(secret)))
		abort();
	return secret;
}

static void check_result(const u8 *msg, const char *hex)
{
	assert(streq(hex, tal_hex(tmpctx, msg)));
}

int main(void)
{
	setup_locale();

	struct crypto_state cs_out, cs_in;
	struct secret sk, rk, ck;
	const void *msg;
	size_t i;

	setup_tmpctx();
	msg = tal_dup_arr(tmpctx, char, "hello", 5, 0);

	/* BOLT #8:
	 *
	 * name: transport-initiator successful handshake
	 *...
	 * # ck,temp_k3=0x919219dbb2920afa8db80f9a51787a840bcf111ed8d588caf9ab4be716e42b01,0x981a46c820fb7a241bc8184ba4bb1f01bcdfafb00dde80098cb8c38db9141520
	 * # encryptWithAD(0x981a46c820fb7a241bc8184ba4bb1f01bcdfafb00dde80098cb8c38db9141520, 0x000000000000000000000000, 0x5dcb5ea9b4ccc755e0e3456af3990641276e1d5dc9afd82f974d90a47c918660, <empty>)
	 * # t=0x8dc68b1c466263b47fdf31e560e139ba
	 * output: 0x00b9e3a702e93e3a9948c2ed6e5fd7590a6e1c3a0344cfc9d5b57357049aa22355361aa02e55a8fc28fef5bd6d71ad0c38228dc68b1c466263b47fdf31e560e139ba
	 * # HKDF(0x919219dbb2920afa8db80f9a51787a840bcf111ed8d588caf9ab4be716e42b01,zero)
	 * output: sk,rk=0x969ab31b4d288cedf6218839b27a3e2140827047f2c0f01bf5c04435d43511a9,0xbb9020b8965f4df047e07f955f3c4b88418984aadc5cdb35096b9ea8fa5c3442
	 */
	ck = secret_from_hex("0x919219dbb2920afa8db80f9a51787a840bcf111ed8d588caf9ab4be716e42b01");
	sk = secret_from_hex("0x969ab31b4d288cedf6218839b27a3e2140827047f2c0f01bf5c04435d43511a9");
	rk = secret_from_hex("0xbb9020b8965f4df047e07f955f3c4b88418984aadc5cdb35096b9ea8fa5c3442");

	cs_out.sn = cs_out.rn = cs_in.sn = cs_in.rn = 0;
	cs_out.sk = cs_in.rk = sk;
	cs_out.rk = cs_in.sk = rk;
	cs_out.s_ck = cs_out.r_ck = cs_in.s_ck = cs_in.r_ck = ck;

	for (i = 0; i < 1002; i++) {
		u8 *dec, *enc;
		u16 len;

		enc = cryptomsg_encrypt_msg(NULL, &cs_out, msg);

		/* BOLT #8:
		 *
		 *  output 0: 0xcf2b30ddf0cf3f80e7c35a6e6730b59fe802473180f396d88a8fb0db8cbcf25d2f214cf9ea1d95
		 */
		if (i == 0)
			check_result(enc, "cf2b30ddf0cf3f80e7c35a6e6730b59fe802473180f396d88a8fb0db8cbcf25d2f214cf9ea1d95");
		/* BOLT #8:
		 *
		 *  output 1: 0x72887022101f0b6753e0c7de21657d35a4cb2a1f5cde2650528bbc8f837d0f0d7ad833b1a256a1
		 */
		if (i == 1)
			check_result(enc, "72887022101f0b6753e0c7de21657d35a4cb2a1f5cde2650528bbc8f837d0f0d7ad833b1a256a1");

		/* BOLT #8:
		 *
		 *  output 500: 0x178cb9d7387190fa34db9c2d50027d21793c9bc2d40b1e14dcf30ebeeeb220f48364f7a4c68bf8
		 *  output 501: 0x1b186c57d44eb6de4c057c49940d79bb838a145cb528d6e8fd26dbe50a60ca2c104b56b60e45bd
		*/
		if (i == 500)
			check_result(enc, "178cb9d7387190fa34db9c2d50027d21793c9bc2d40b1e14dcf30ebeeeb220f48364f7a4c68bf8");
		if (i == 501)
			check_result(enc, "1b186c57d44eb6de4c057c49940d79bb838a145cb528d6e8fd26dbe50a60ca2c104b56b60e45bd");

		/* BOLT #8:
		 *
		 *  output 1000: 0x4a2f3cc3b5e78ddb83dcb426d9863d9d9a723b0337c89dd0b005d89f8d3c05c52b76b29b740f09
		 *  output 1001: 0x2ecd8c8a5629d0d02ab457a0fdd0f7b90a192cd46be5ecb6ca570bfc5e268338b1a16cf4ef2d36
		 */
		if (i == 1000)
			check_result(enc, "4a2f3cc3b5e78ddb83dcb426d9863d9d9a723b0337c89dd0b005d89f8d3c05c52b76b29b740f09");
		if (i == 1001)
			check_result(enc, "2ecd8c8a5629d0d02ab457a0fdd0f7b90a192cd46be5ecb6ca570bfc5e268338b1a16cf4ef2d36");

		if (!cryptomsg_decrypt_header(&cs_in, enc, &len))
			abort();

		/* Trim header */
		memmove(enc, enc + CRYPTOMSG_HDR_SIZE,
			tal_bytelen(enc) - CRYPTOMSG_HDR_SIZE);
		tal_resize(&enc, tal_bytelen(enc) - CRYPTOMSG_HDR_SIZE);

		dec = cryptomsg_decrypt_body(enc, &cs_in, enc);
		assert(memeq(dec, tal_bytelen(dec), msg, tal_bytelen(msg)));
	}
	tal_free(tmpctx);
	return 0;
}
