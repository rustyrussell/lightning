#include "config.h"

#include <ccan/mem/mem.h>
#include <common/hsm_encryption.c>
#include <tests/fuzz/libfuzz.h>

int main(int argc, char *argv[])
{
	const u8 *fuzzbuf = FUZZ_SETUP(argc, argv);

	while (FUZZ_LOOP) {
		size_t fuzzsize = FUZZ_SIZE;

		/* 4294967295 is crypto_pwhash_argon2id_PASSWD_MAX. libfuzzer won't
		 * generate inputs that large in practice, but hey. */
		if (fuzzsize > 32 && fuzzsize < 4294967295) {
			struct secret *hsm_secret, decrypted_hsm_secret, encryption_key;
			char *passphrase;
			struct encrypted_hsm_secret encrypted_secret;
			char *emsg;

			/* Take the first 32 bytes as the plaintext hsm_secret seed,
			 * and the remaining ones as the passphrase. */
			hsm_secret = (struct secret *)tal_dup_arr(NULL, u8, fuzzbuf, 32, 0);
			passphrase = fuzz_input_string(NULL, fuzzbuf + 32, fuzzsize - 32);

			/* A valid seed, a valid passphrase. This should not fail. */
			assert(!hsm_secret_encryption_key_with_exitcode(passphrase, &encryption_key, &emsg));
			/* Roundtrip */
			assert(encrypt_hsm_secret(&encryption_key, hsm_secret,
						  &encrypted_secret));
			assert(decrypt_hsm_secret(&encryption_key, &encrypted_secret,
						  &decrypted_hsm_secret));
			assert(memeq(hsm_secret->data, sizeof(hsm_secret->data),
				     decrypted_hsm_secret.data,
				     sizeof(decrypted_hsm_secret.data)));

			tal_free(hsm_secret);
			tal_free(passphrase);
		}
	}
	fuzz_shutdown();
}
