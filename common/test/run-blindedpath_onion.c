/* Pipe through jq for test vector! */
#include "../bigsize.c"
#include "../blindedpath.c"
#include "../blinding.c"
#include "../hmac.c"
#include "../onion.c"
#include "../sphinx.c"
#include "../type_to_string.c"
#include <common/setup.h>
#include <wire/peer_wire.h>
#include <stdio.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for amount_asset_is_main */
bool amount_asset_is_main(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_is_main called!\n"); abort(); }
/* Generated stub for amount_asset_to_sat */
struct amount_sat amount_asset_to_sat(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_to_sat called!\n"); abort(); }
/* Generated stub for amount_msat */
struct amount_msat amount_msat(u64 millisatoshis UNNEEDED)
{ fprintf(stderr, "amount_msat called!\n"); abort(); }
/* Generated stub for amount_msat_eq */
bool amount_msat_eq(struct amount_msat a UNNEEDED, struct amount_msat b UNNEEDED)
{ fprintf(stderr, "amount_msat_eq called!\n"); abort(); }
/* Generated stub for amount_sat */
struct amount_sat amount_sat(u64 satoshis UNNEEDED)
{ fprintf(stderr, "amount_sat called!\n"); abort(); }
/* Generated stub for amount_sat_add */
 bool amount_sat_add(struct amount_sat *val UNNEEDED,
				       struct amount_sat a UNNEEDED,
				       struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_add called!\n"); abort(); }
/* Generated stub for amount_sat_eq */
bool amount_sat_eq(struct amount_sat a UNNEEDED, struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_eq called!\n"); abort(); }
/* Generated stub for amount_sat_greater_eq */
bool amount_sat_greater_eq(struct amount_sat a UNNEEDED, struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_greater_eq called!\n"); abort(); }
/* Generated stub for amount_sat_sub */
 bool amount_sat_sub(struct amount_sat *val UNNEEDED,
				       struct amount_sat a UNNEEDED,
				       struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_sub called!\n"); abort(); }
/* Generated stub for amount_sat_to_asset */
struct amount_asset amount_sat_to_asset(struct amount_sat *sat UNNEEDED, const u8 *asset UNNEEDED)
{ fprintf(stderr, "amount_sat_to_asset called!\n"); abort(); }
/* Generated stub for amount_tx_fee */
struct amount_sat amount_tx_fee(u32 fee_per_kw UNNEEDED, size_t weight UNNEEDED)
{ fprintf(stderr, "amount_tx_fee called!\n"); abort(); }
/* Generated stub for fromwire_amount_msat */
struct amount_msat fromwire_amount_msat(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_amount_msat called!\n"); abort(); }
/* Generated stub for fromwire_amount_sat */
struct amount_sat fromwire_amount_sat(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_amount_sat called!\n"); abort(); }
/* Generated stub for fromwire_channel_id */
void fromwire_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			 struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_channel_id called!\n"); abort(); }
/* Generated stub for fromwire_node_id */
void fromwire_node_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct node_id *id UNNEEDED)
{ fprintf(stderr, "fromwire_node_id called!\n"); abort(); }
/* Generated stub for new_onionreply */
struct onionreply *new_onionreply(const tal_t *ctx UNNEEDED, const u8 *contents TAKES UNNEEDED)
{ fprintf(stderr, "new_onionreply called!\n"); abort(); }
/* Generated stub for pubkey_from_node_id */
bool pubkey_from_node_id(struct pubkey *key UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "pubkey_from_node_id called!\n"); abort(); }
/* Generated stub for towire_amount_msat */
void towire_amount_msat(u8 **pptr UNNEEDED, const struct amount_msat msat UNNEEDED)
{ fprintf(stderr, "towire_amount_msat called!\n"); abort(); }
/* Generated stub for towire_amount_sat */
void towire_amount_sat(u8 **pptr UNNEEDED, const struct amount_sat sat UNNEEDED)
{ fprintf(stderr, "towire_amount_sat called!\n"); abort(); }
/* Generated stub for towire_channel_id */
void towire_channel_id(u8 **pptr UNNEEDED, const struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "towire_channel_id called!\n"); abort(); }
/* Generated stub for towire_node_id */
void towire_node_id(u8 **pptr UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "towire_node_id called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

static void json_strfield(const char *name, const char *val)
{
	printf("\t\"%s\": \"%s\",\n", name, val);
}

#define ALICE 0
#define BOB 1
#define CAROL 2
#define DAVE 3

/* Updated each time, as we pretend to be Alice, Bob, Carol */
static const struct privkey *ecdh_key;

void ecdh(const struct pubkey *point, struct secret *ss)
{
	if (secp256k1_ecdh(secp256k1_ctx, ss->data, &point->pubkey,
			   ecdh_key->secret.data, NULL, NULL) != 1)
		abort();
}

static u8 *next_onion(const tal_t *ctx, u8 *omsg,
		      const struct privkey *nodekey,
		      const struct pubkey *expected_blinding)
{
	struct onionpacket *op;
	struct pubkey blinding, ephemeral;
	struct pubkey next_node, next_blinding;
	struct tlv_onionmsg_payload *om;
	struct secret ss, onion_ss;
	const u8 *cursor;
	size_t max, maxlen;
	struct route_step *rs;

	assert(fromwire_onion_message(tmpctx, omsg, &blinding, &omsg));
	assert(pubkey_eq(&blinding, expected_blinding));

	op = parse_onionpacket(tmpctx, omsg, tal_bytelen(omsg), NULL);
	ephemeral = op->ephemeralkey;

	ecdh_key = nodekey;
	assert(unblind_onion(&blinding, ecdh, &ephemeral, &ss));

	ecdh(&ephemeral, &onion_ss);
	rs = process_onionpacket(tmpctx, op, &onion_ss, NULL, 0, false);
	assert(rs);

	/* The raw payload is prepended with length in the modern onion. */
	cursor = rs->raw_payload;
	max = tal_bytelen(rs->raw_payload);
	maxlen = fromwire_bigsize(&cursor, &max);
	om = tlv_onionmsg_payload_new(tmpctx);
	assert(fromwire_onionmsg_payload(&cursor, &maxlen, om));

	if (rs->nextcase == ONION_END)
		return NULL;

	assert(decrypt_enctlv(&blinding, &ss, om->encrypted_data_tlv, &next_node,
			      &next_blinding));
	return towire_onion_message(ctx, &next_blinding,
				    serialize_onionpacket(tmpctx, rs->next));
}

int main(int argc, char *argv[])
{
	struct privkey nodekey[4], blinding[4], override_blinding;
	struct pubkey id[4], blinding_pub[4], override_blinding_pub, alias[4];
	struct secret self_id;
	u8 *enctlv[4];
	u8 *onionmsg_payload[4];
	u8 *omsg;
	struct sphinx_path *sphinx_path;
	struct secret *path_secrets;
	struct onionpacket *op;
	const char *names[] = {"Alice", "Bob", "Carol", "Dave"};

	common_setup(argv[0]);

	for (size_t i = 0; i < 4; i++) {
		memset(&nodekey[i], names[i][0], sizeof(nodekey[i]));
		pubkey_from_privkey(&nodekey[i], &id[i]);
	}

	/* Make enctlvs as per enctlv test vectors */
	memset(&blinding[ALICE], 5, sizeof(blinding[ALICE]));
	pubkey_from_privkey(&blinding[ALICE], &blinding_pub[ALICE]);

	enctlv[ALICE] = create_enctlv(tmpctx, &blinding[ALICE],
				      &id[ALICE], &id[BOB],
				      0, NULL, &blinding[BOB], &alias[ALICE]);

	pubkey_from_privkey(&blinding[BOB], &blinding_pub[BOB]);

	/* We override blinding for Carol. */
	memset(&override_blinding, 7, sizeof(override_blinding));
	pubkey_from_privkey(&override_blinding, &override_blinding_pub);
	enctlv[BOB] = create_enctlv(tmpctx, &blinding[BOB],
				    &id[BOB], &id[CAROL],
				    0, &override_blinding_pub,
				    &blinding[CAROL], &alias[BOB]);

	/* That replaced the blinding */
	blinding[CAROL] = override_blinding;
	blinding_pub[CAROL] = override_blinding_pub;

	enctlv[CAROL] = create_enctlv(tmpctx, &blinding[CAROL],
				      &id[CAROL], &id[DAVE],
				      35, NULL, &blinding[DAVE], &alias[CAROL]);

	for (size_t i = 0; i < sizeof(self_id); i++)
		self_id.data[i] = i+1;

	enctlv[DAVE] = create_final_enctlv(tmpctx, &blinding[DAVE], &id[DAVE],
					   0, &self_id, &alias[DAVE]);
	pubkey_from_privkey(&blinding[DAVE], &blinding_pub[DAVE]);

	/* Create an onion which encodes this. */
	sphinx_path = sphinx_path_new(tmpctx, NULL);
	for (size_t i = 0; i < 4; i++) {
		struct tlv_onionmsg_payload *payload
			= tlv_onionmsg_payload_new(tmpctx);
		payload->encrypted_data_tlv = enctlv[i];
		onionmsg_payload[i] = tal_arr(tmpctx, u8, 0);
		towire_onionmsg_payload(&onionmsg_payload[i], payload);
		sphinx_add_modern_hop(sphinx_path, &alias[i],
				      onionmsg_payload[i]);
	}
	op = create_onionpacket(tmpctx, sphinx_path, ROUTING_INFO_SIZE,
				&path_secrets);

	/* And finally, the onion message as handed to Alice */
	omsg = towire_onion_message(tmpctx, &blinding_pub[ALICE],
				    serialize_onionpacket(tmpctx, op));

	printf("{\n");
	json_strfield("description",
		      "Onion message encoding enctlv as per enctlvs.json");
	printf("\t\"onionmsgs\": [");
	for (size_t i = 0; i < 4; i++) {
		printf("{\n");
		json_strfield("node name", names[i]);
		json_strfield("node_secret",
			      type_to_string(tmpctx, struct privkey,
					     &nodekey[i]));
		json_strfield("node_id",
			      type_to_string(tmpctx, struct pubkey, &id[i]));

		printf("\t\"onion_message\": {");
		json_strfield("raw", tal_hex(tmpctx, omsg));
		json_strfield("blinding_secret",
			      type_to_string(tmpctx, struct privkey,
					     &blinding[i]));
		json_strfield("blinding",
			      type_to_string(tmpctx, struct pubkey, &blinding_pub[i]));
		json_strfield("blinded_alias",
			      type_to_string(tmpctx, struct pubkey, &alias[i]));
		json_strfield("onionmsg_payload",
			      tal_hex(tmpctx, onionmsg_payload[i]));
		printf("\"enctlv\": \"%s\"}\n", tal_hex(tmpctx, enctlv[i]));

		printf("}");
		if (i != DAVE)
			printf(",\n");

		/* Unwrap for next hop */
		omsg = next_onion(tmpctx, omsg, &nodekey[i], &blinding_pub[i]);
	}
	assert(!omsg);
	printf("\n]}\n");

	common_shutdown();
}
