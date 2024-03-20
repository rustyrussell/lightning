#include "config.h"

#include "../queries.c"
#include <ccan/str/hex/hex.h>
#include <common/blinding.h>
#include <common/channel_type.h>
#include <common/ecdh.h>
#include <common/json_parse.h>
#include <common/json_stream.h>
#include <common/onionreply.h>
#include <common/setup.h>
#include <stdio.h>

#ifdef NDEBUG
#error "assert required for tests"
#endif

/* AUTOGENERATED MOCKS START */
/* Generated stub for blinding_hash_e_and_ss */
void blinding_hash_e_and_ss(const struct pubkey *e UNNEEDED,
			    const struct secret *ss UNNEEDED,
			    struct sha256 *sha UNNEEDED)
{ fprintf(stderr, "blinding_hash_e_and_ss called!\n"); abort(); }
/* Generated stub for blinding_next_privkey */
bool blinding_next_privkey(const struct privkey *e UNNEEDED,
			   const struct sha256 *h UNNEEDED,
			   struct privkey *next UNNEEDED)
{ fprintf(stderr, "blinding_next_privkey called!\n"); abort(); }
/* Generated stub for blinding_next_pubkey */
bool blinding_next_pubkey(const struct pubkey *pk UNNEEDED,
			  const struct sha256 *h UNNEEDED,
			  struct pubkey *next UNNEEDED)
{ fprintf(stderr, "blinding_next_pubkey called!\n"); abort(); }
/* Generated stub for daemon_conn_wake */
void daemon_conn_wake(struct daemon_conn *dc UNNEEDED)
{ fprintf(stderr, "daemon_conn_wake called!\n"); abort(); }
/* Generated stub for decode_channel_update_timestamps */
struct channel_update_timestamps *decode_channel_update_timestamps(const tal_t *ctx UNNEEDED,
				 const struct tlv_reply_channel_range_tlvs_timestamps_tlv *timestamps_tlv UNNEEDED)
{ fprintf(stderr, "decode_channel_update_timestamps called!\n"); abort(); }
/* Generated stub for decode_scid_query_flags */
bigsize_t *decode_scid_query_flags(const tal_t *ctx UNNEEDED,
				   const struct tlv_query_short_channel_ids_tlvs_query_flags *qf UNNEEDED)
{ fprintf(stderr, "decode_scid_query_flags called!\n"); abort(); }
/* Generated stub for decode_short_ids */
struct short_channel_id *decode_short_ids(const tal_t *ctx UNNEEDED, const u8 *encoded UNNEEDED)
{ fprintf(stderr, "decode_short_ids called!\n"); abort(); }
/* Generated stub for first_random_peer */
struct peer *first_random_peer(struct daemon *daemon UNNEEDED,
			       struct peer_node_id_map_iter *it UNNEEDED)
{ fprintf(stderr, "first_random_peer called!\n"); abort(); }
/* Generated stub for fromwire_gossipd_dev_set_max_scids_encode_size */
bool fromwire_gossipd_dev_set_max_scids_encode_size(const void *p UNNEEDED, u32 *max UNNEEDED)
{ fprintf(stderr, "fromwire_gossipd_dev_set_max_scids_encode_size called!\n"); abort(); }
/* Generated stub for gossmap_chan_byidx */
struct gossmap_chan *gossmap_chan_byidx(const struct gossmap *map UNNEEDED, u32 idx UNNEEDED)
{ fprintf(stderr, "gossmap_chan_byidx called!\n"); abort(); }
/* Generated stub for gossmap_chan_get_announce */
u8 *gossmap_chan_get_announce(const tal_t *ctx UNNEEDED,
			      const struct gossmap *map UNNEEDED,
			      const struct gossmap_chan *c UNNEEDED)
{ fprintf(stderr, "gossmap_chan_get_announce called!\n"); abort(); }
/* Generated stub for gossmap_chan_get_update */
u8 *gossmap_chan_get_update(const tal_t *ctx UNNEEDED,
			    const struct gossmap *map UNNEEDED,
			    const struct gossmap_chan *chan UNNEEDED,
			    int dir UNNEEDED)
{ fprintf(stderr, "gossmap_chan_get_update called!\n"); abort(); }
/* Generated stub for gossmap_chan_get_update_details */
void gossmap_chan_get_update_details(const struct gossmap *map UNNEEDED,
				     const struct gossmap_chan *chan UNNEEDED,
				     int dir UNNEEDED,
				     u32 *timestamp UNNEEDED,
				     u8 *message_flags UNNEEDED,
				     u8 *channel_flags UNNEEDED,
				     u32 *fee_base_msat UNNEEDED,
				     u32 *fee_proportional_millionths UNNEEDED,
				     struct amount_msat *htlc_minimum_msat UNNEEDED,
				     struct amount_msat *htlc_maximum_msat UNNEEDED)
{ fprintf(stderr, "gossmap_chan_get_update_details called!\n"); abort(); }
/* Generated stub for gossmap_chan_scid */
struct short_channel_id gossmap_chan_scid(const struct gossmap *map UNNEEDED,
					  const struct gossmap_chan *c UNNEEDED)
{ fprintf(stderr, "gossmap_chan_scid called!\n"); abort(); }
/* Generated stub for gossmap_find_chan */
struct gossmap_chan *gossmap_find_chan(const struct gossmap *map UNNEEDED,
				       const struct short_channel_id *scid UNNEEDED)
{ fprintf(stderr, "gossmap_find_chan called!\n"); abort(); }
/* Generated stub for gossmap_find_node */
struct gossmap_node *gossmap_find_node(const struct gossmap *map UNNEEDED,
				       const struct node_id *id UNNEEDED)
{ fprintf(stderr, "gossmap_find_node called!\n"); abort(); }
/* Generated stub for gossmap_manage_get_gossmap */
struct gossmap *gossmap_manage_get_gossmap(struct gossmap_manage *gm UNNEEDED)
{ fprintf(stderr, "gossmap_manage_get_gossmap called!\n"); abort(); }
/* Generated stub for gossmap_max_chan_idx */
u32 gossmap_max_chan_idx(const struct gossmap *map UNNEEDED)
{ fprintf(stderr, "gossmap_max_chan_idx called!\n"); abort(); }
/* Generated stub for gossmap_node_get_announce */
u8 *gossmap_node_get_announce(const tal_t *ctx UNNEEDED,
			      const struct gossmap *map UNNEEDED,
			      const struct gossmap_node *n UNNEEDED)
{ fprintf(stderr, "gossmap_node_get_announce called!\n"); abort(); }
/* Generated stub for gossmap_node_get_id */
void gossmap_node_get_id(const struct gossmap *map UNNEEDED,
			 const struct gossmap_node *node UNNEEDED,
			 struct node_id *id UNNEEDED)
{ fprintf(stderr, "gossmap_node_get_id called!\n"); abort(); }
/* Generated stub for gossmap_nth_node */
struct gossmap_node *gossmap_nth_node(const struct gossmap *map UNNEEDED,
				      const struct gossmap_chan *chan UNNEEDED,
				      int n UNNEEDED)
{ fprintf(stderr, "gossmap_nth_node called!\n"); abort(); }
/* Generated stub for master_badmsg */
void master_badmsg(u32 type_expected UNNEEDED, const u8 *msg)
{ fprintf(stderr, "master_badmsg called!\n"); abort(); }
/* Generated stub for next_random_peer */
struct peer *next_random_peer(struct daemon *daemon UNNEEDED,
			      const struct peer *first UNNEEDED,
			      struct peer_node_id_map_iter *it UNNEEDED)
{ fprintf(stderr, "next_random_peer called!\n"); abort(); }
/* Generated stub for peer_supplied_good_gossip */
void peer_supplied_good_gossip(struct daemon *daemon UNNEEDED,
			       const struct node_id *source_peer UNNEEDED,
			       size_t amount UNNEEDED)
{ fprintf(stderr, "peer_supplied_good_gossip called!\n"); abort(); }
/* Generated stub for queue_peer_msg */
void queue_peer_msg(struct daemon *daemon UNNEEDED,
		    const struct node_id *peer UNNEEDED,
		    const u8 *msg TAKES UNNEEDED)
{ fprintf(stderr, "queue_peer_msg called!\n"); abort(); }
/* Generated stub for towire_warningfmt */
u8 *towire_warningfmt(const tal_t *ctx UNNEEDED,
		      const struct channel_id *channel UNNEEDED,
		      const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "towire_warningfmt called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

void status_fmt(enum log_level level UNNEEDED,
		const struct node_id *node_id UNNEEDED,
		const char *fmt UNNEEDED, ...)
{
}

/* These we can reproduce */
static const char *test_vectors_nozlib[] = {
	"{\n"
	"  \"msg\" : {\n"
	"    \"type\" : \"QueryChannelRange\",\n"
	"    \"chainHash\" : \"0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206\",\n"
	"    \"firstBlockNum\" : 100000,\n"
	"    \"numberOfBlocks\" : 1500,\n"
	"    \"extensions\" : [ ]\n"
	"  },\n"
	"  \"hex\" : \"01070f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206000186a0000005dc\"\n"
	"}\n",
	"{\n"
	"  \"msg\" : {\n"
	"    \"type\" : \"QueryChannelRange\",\n"
	"    \"chainHash\" : \"0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206\",\n"
	"    \"firstBlockNum\" : 35000,\n"
	"    \"numberOfBlocks\" : 100,\n"
	"    \"extensions\" : [ \"WANT_TIMESTAMPS | WANT_CHECKSUMS\" ]\n"
	"  },\n"
	"  \"hex\" : \"01070f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206000088b800000064010103\"\n"
	"}\n",
	"{\n"
	"  \"msg\" : {\n"
	"    \"type\" : \"ReplyChannelRange\",\n"
	"    \"chainHash\" : \"0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206\",\n"
	"    \"firstBlockNum\" : 122334,\n"
	"    \"numberOfBlocks\" : 1500,\n"
	"    \"complete\" : 1,\n"
	"    \"shortChannelIds\" : {\n"
	"      \"encoding\" : \"UNCOMPRESSED\",\n"
	"      \"array\" : [ \"0x0x12355\", \"0x7x30934\", \"0x70x57793\" ]\n"
	"    },\n"
	"    \"timestamps\" : {\n"
	"      \"encoding\" : \"UNCOMPRESSED\",\n"
	"      \"timestamps\" : [ {\n"
	"        \"timestamp1\" : 164545,\n"
	"        \"timestamp2\" : 948165\n"
	"      }, {\n"
	"        \"timestamp1\" : 489645,\n"
	"        \"timestamp2\" : 4786864\n"
	"      }, {\n"
	"        \"timestamp1\" : 46456,\n"
	"        \"timestamp2\" : 9788415\n"
	"      } ]\n"
	"    },\n"
	"    \"checksums\" : {\n"
	"      \"checksums\" : [ {\n"
	"        \"checksum1\" : 1111,\n"
	"        \"checksum2\" : 2222\n"
	"      }, {\n"
	"        \"checksum1\" : 3333,\n"
	"        \"checksum2\" : 4444\n"
	"      }, {\n"
	"        \"checksum1\" : 5555,\n"
	"        \"checksum2\" : 6666\n"
	"      } ]\n"
	"    }\n"
	"  },\n"
	"  \"hex\" : \"01080f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e22060001ddde000005dc01001900000000000000304300000000000778d6000000000046e1c1011900000282c1000e77c5000778ad00490ab00000b57800955bff031800000457000008ae00000d050000115c000015b300001a0a\"\n"
	"}\n",
	"{\n"
	"  \"msg\" : {\n"
	"    \"type\" : \"QueryShortChannelIds\",\n"
	"    \"chainHash\" : \"0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206\",\n"
	"    \"shortChannelIds\" : {\n"
	"      \"encoding\" : \"UNCOMPRESSED\",\n"
	"      \"array\" : [ \"0x0x142\", \"0x0x15465\", \"0x69x42692\" ]\n"
	"    },\n"
	"    \"extensions\" : [ ]\n"
	"  },\n"
	"  \"hex\" : \"01050f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206001900000000000000008e0000000000003c69000000000045a6c4\"\n"
	"}\n",
};

static void get_chainhash(const char *test_vector,
			  const jsmntok_t *obj,
			  struct bitcoin_blkid *chain_hash)
{
	const jsmntok_t *tok = json_get_member(test_vector, obj, "chainHash");
	size_t hexlen = tok->end - tok->start;
	assert(hex_decode(test_vector + tok->start, hexlen,
			  chain_hash, sizeof(*chain_hash)));
}

static u8 *get_scid_array(const tal_t *ctx,
			  const char *test_vector,
			  const jsmntok_t *obj)
{
	const jsmntok_t *scids, *arr, *encoding, *t;
	size_t i;
	u8 *encoded;

	scids = json_get_member(test_vector, obj, "shortChannelIds");
	arr = json_get_member(test_vector, scids, "array");

	encoded = encoding_start(ctx, true);
	encoding = json_get_member(test_vector, scids, "encoding");
	json_for_each_arr(i, t, arr) {
		struct short_channel_id scid;
		assert(json_to_short_channel_id(test_vector, t, &scid));
		encoding_add_short_channel_id(&encoded, scid);
	}
	assert(json_tok_streq(test_vector, encoding, "UNCOMPRESSED"));

	return encoded;
}

static u8 *test_query_channel_range(const char *test_vector, const jsmntok_t *obj)
{
	struct bitcoin_blkid chain_hash;
	u32 firstBlockNum, numberOfBlocks;
	const jsmntok_t *opt, *t;
	struct tlv_query_channel_range_tlvs *tlvs
		= tlv_query_channel_range_tlvs_new(NULL);
	u8 *msg;
	size_t i;

	get_chainhash(test_vector, obj, &chain_hash);
	assert(json_to_number(test_vector, json_get_member(test_vector, obj, "firstBlockNum"), &firstBlockNum));
	assert(json_to_number(test_vector, json_get_member(test_vector, obj, "numberOfBlocks"), &numberOfBlocks));
	opt = json_get_member(test_vector, obj, "extensions");
	json_for_each_arr(i, t, opt) {
		assert(json_tok_streq(test_vector, t,
				      "WANT_TIMESTAMPS | WANT_CHECKSUMS"));
		tlvs->query_option = tal(tlvs, bigsize_t);
		*tlvs->query_option =
			QUERY_ADD_TIMESTAMPS | QUERY_ADD_CHECKSUMS;
	}
	msg = towire_query_channel_range(NULL, &chain_hash, firstBlockNum, numberOfBlocks, tlvs);

	tal_free(tlvs);
	return msg;
}

static u8 *test_reply_channel_range(const char *test_vector, const jsmntok_t *obj)
{
	struct bitcoin_blkid chain_hash;
	u32 firstBlockNum, numberOfBlocks, complete;
	const jsmntok_t *opt, *t;
	size_t i;
	u8 *msg;
	u8 *encoded_scids;

	u8 *ctx = tal(NULL, u8);
	struct tlv_reply_channel_range_tlvs *tlvs
		= tlv_reply_channel_range_tlvs_new(ctx);

	get_chainhash(test_vector, obj, &chain_hash);
	assert(json_to_number(test_vector, json_get_member(test_vector, obj, "firstBlockNum"), &firstBlockNum));
	assert(json_to_number(test_vector, json_get_member(test_vector, obj, "numberOfBlocks"), &numberOfBlocks));
	assert(json_to_number(test_vector, json_get_member(test_vector, obj, "complete"), &complete));

	encoded_scids = get_scid_array(ctx, test_vector, obj);

	opt = json_get_member(test_vector, obj, "timestamps");
	if (opt) {
		const jsmntok_t *encodingtok, *tstok;
		tlvs->timestamps_tlv
			= tal(tlvs, struct tlv_reply_channel_range_tlvs_timestamps_tlv);

		tlvs->timestamps_tlv->encoded_timestamps
			= encoding_start(tlvs->timestamps_tlv, false);
		encodingtok = json_get_member(test_vector, opt, "encoding");
		tstok = json_get_member(test_vector, opt, "timestamps");
		json_for_each_arr(i, t, tstok) {
			struct channel_update_timestamps ts;
			assert(json_to_number(test_vector,
					      json_get_member(test_vector, t,
							      "timestamp1"),
					      &ts.timestamp_node_id_1));
			assert(json_to_number(test_vector,
					      json_get_member(test_vector, t,
							      "timestamp2"),
					      &ts.timestamp_node_id_2));
			encoding_add_timestamps(&tlvs->timestamps_tlv->encoded_timestamps, &ts);
		}
		assert(json_tok_streq(test_vector, encodingtok, "UNCOMPRESSED"));
		tlvs->timestamps_tlv->encoding_type = ARR_UNCOMPRESSED;
	}

	opt = json_get_member(test_vector, obj, "checksums");
	if (opt) {
		const jsmntok_t *cstok;
		tlvs->checksums_tlv
			= tal_arr(tlvs, struct channel_update_checksums, 0);

		cstok = json_get_member(test_vector, opt, "checksums");
		json_for_each_arr(i, t, cstok) {
			struct channel_update_checksums cs;
			assert(json_to_number(test_vector,
				      json_get_member(test_vector, t,
							      "checksum1"),
					      &cs.checksum_node_id_1));
			assert(json_to_number(test_vector,
					      json_get_member(test_vector, t,
							      "checksum2"),
					      &cs.checksum_node_id_2));
			tal_arr_expand(&tlvs->checksums_tlv, cs);
		}
	}

	msg = towire_reply_channel_range(
		NULL, &chain_hash, firstBlockNum, numberOfBlocks,
		complete, encoded_scids, tlvs);
	tal_free(ctx);
	return msg;
}

static struct tlv_query_short_channel_ids_tlvs_query_flags *
get_query_flags_array(const tal_t *ctx,
		      const char *test_vector,
		      const jsmntok_t *opt)
{
	const jsmntok_t *encoding, *arr, *t;
	struct tlv_query_short_channel_ids_tlvs_query_flags *tlv
		= tal(ctx, struct tlv_query_short_channel_ids_tlvs_query_flags);
	size_t i;

	arr = json_get_member(test_vector, opt, "array");

	tlv->encoded_query_flags = encoding_start(tlv, false);
	encoding = json_get_member(test_vector, opt, "encoding");
	json_for_each_arr(i, t, arr) {
		bigsize_t f;
		assert(json_to_u64(test_vector, t, &f));
		encoding_add_query_flag(&tlv->encoded_query_flags, f);
	}
	assert(json_tok_streq(test_vector, encoding, "UNCOMPRESSED"));
	tlv->encoding_type = ARR_UNCOMPRESSED;

	return tlv;
}

static u8 *test_query_short_channel_ids(const char *test_vector,
					const jsmntok_t *obj)
{
	struct bitcoin_blkid chain_hash;
	const jsmntok_t *opt;
	u8 *encoded, *msg;
	struct tlv_query_short_channel_ids_tlvs *tlvs;

	get_chainhash(test_vector, obj, &chain_hash);
	encoded = get_scid_array(NULL, test_vector, obj);

	opt = json_get_member(test_vector, obj, "extensions");
	if (opt && opt->size != 0) {
		/* Only handle a single */
		assert(opt->size == 1);
		tlvs = tlv_query_short_channel_ids_tlvs_new(encoded);
		tlvs->query_flags
			= get_query_flags_array(tlvs, test_vector, opt + 1);
	} else
		tlvs = NULL;
	msg = towire_query_short_channel_ids(NULL,
					     &chain_hash, encoded, tlvs);

	tal_free(encoded);
	return msg;
}

int main(int argc, char *argv[])
{
	jsmntok_t *toks = tal_arr(NULL, jsmntok_t, 1000);

	common_setup(argv[0]);

	for (size_t i = 0; i < ARRAY_SIZE(test_vectors_nozlib); i++) {
		jsmn_parser parser;
		u8 *m;
		const char *hex_m;
		const jsmntok_t *msg, *type, *hex;

		jsmn_init(&parser);
		assert(jsmn_parse(&parser,
				  test_vectors_nozlib[i], strlen(test_vectors_nozlib[i]),
				  toks, tal_count(toks)) > 0);

		msg = json_get_member(test_vectors_nozlib[i], toks, "msg");
		hex = json_get_member(test_vectors_nozlib[i], toks, "hex");
		type = json_get_member(test_vectors_nozlib[i], msg, "type");
		if (json_tok_streq(test_vectors_nozlib[i], type, "QueryChannelRange"))
			m = test_query_channel_range(test_vectors_nozlib[i], msg);
		else if (json_tok_streq(test_vectors_nozlib[i], type, "ReplyChannelRange"))
			m = test_reply_channel_range(test_vectors_nozlib[i], msg);
		else if (json_tok_streq(test_vectors_nozlib[i], type, "QueryShortChannelIds"))
			m = test_query_short_channel_ids(test_vectors_nozlib[i], msg);
		else
			abort();
		hex_m = tal_hex(m, m);
		assert(json_tok_streq(test_vectors_nozlib[i], hex, hex_m));
		tal_free(m);
	}
	tal_free(toks);
	common_shutdown();
	return 0;
}
