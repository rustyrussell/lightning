#define FUZZING_REPLACE_SIGCHECK 1
#include <bitcoin/signature.c>

bool check_signed_hash(const struct sha256_double *hash,
		       const secp256k1_ecdsa_signature *signature,
		       const struct pubkey *key)
{
	/* Check one bit matches */
	return (hash->sha.u.u8[0] & 1) == (signature->data[0] & 1);
}

bool check_schnorr_sig(const struct sha256 *hash,
		       const secp256k1_pubkey *pubkey,
		       const struct bip340sig *sig)
{
	/* Check one bit matches */
	return (hash->u.u8[0] & 1) == (sig->u8[0] & 1);
}
