#include "config.h"
#include <bitcoin/block.h>
#include <bitcoin/privkey.h>
#include <bitcoin/pubkey.h>
#include <bitcoin/short_channel_id.h>
#include <ccan/crc32c/crc32c.h>
#include <ccan/err/err.h>
#include <ccan/read_write_all/read_write_all.h>
#include <ccan/tal/grab_file/grab_file.h>
#include <ccan/tal/str/str.h>
#include <ccan/ungraph/ungraph.h>
#include <common/amount.h>
#include <common/gossip_store.h>
#include <common/gosstest.h>
#include <common/node_id.h>
#include <gossipd/gossip_store_wiregen.h>
#include <time.h>
#include <unistd.h>
#include <wire/peer_wire.h>

struct graph_comment {
	const char *name;
	struct node_id *nodes;
	struct short_channel_id *channels;
	u8 *gossmap;
	bool verbose;
};

static void add_to_gossmap(struct graph_comment *gc, const u8 *msg)
{
	size_t len = tal_bytelen(gc->gossmap);
	struct gossip_hdr hdr;

	hdr.flags = cpu_to_be16(0);
	hdr.len = cpu_to_be16(tal_bytelen(msg));
	hdr.timestamp = cpu_to_be32(0);
	hdr.crc = cpu_to_be32(crc32c(be32_to_cpu(hdr.timestamp),
				     msg, tal_bytelen(msg)));
	tal_resize(&gc->gossmap, len + sizeof(hdr) + tal_bytelen(msg));
	memcpy(gc->gossmap + len, &hdr, sizeof(hdr));
	memcpy(gc->gossmap + len + sizeof(hdr), msg, tal_bytelen(msg));
}

static struct graph_comment **graph_comments;

static bool have_channel(const struct short_channel_id *channels,
			 const struct short_channel_id *scid)
{
	for (size_t i = 0; i < tal_count(channels); i++) {
		if (short_channel_id_eq(&channels[i], scid))
			return true;
	}
	return false;
}

static bool have_node(const struct node_id *nodes,
			 const struct node_id *id)
{
	for (size_t i = 0; i < tal_count(nodes); i++) {
		if (node_id_eq(&nodes[i], id))
			return true;
	}
	return false;
}

/* We don't do anything; we wait until mentioned on an edge */
static void *add_node(const tal_t *ctx,
                      const char *name,
                      const char **errstr,
		      struct graph_comment *gc)

{
	if (gc->verbose)
		printf("NODE: %s\n", name);
        return (void *)name;
}

static const char *add_edge(const tal_t *ctx,
                            void *source_node,
                            void *dest_node,
                            bool bidir,
                            const char **labels,
                            struct graph_comment *gc)
{
	struct amount_sat cap = AMOUNT_SAT(100000);
	struct short_channel_id scid;
	struct privkey s, d;
	struct pubkey src_pub, dst_pub;
	struct node_id src, dst, *a, *b;
	secp256k1_ecdsa_signature sig;
	u8 channel_flags;
	struct bitcoin_blkid chain_hash;
	u16 cltv_expiry_delta = 100;
	struct amount_msat htlc_minimum_msat = AMOUNT_MSAT(0);
	u32 fee_base_msat = 0;
	u32 fee_proportional_millionths = 0;
	struct amount_msat htlc_maximum_msat = AMOUNT_MSAT(0);

	if (gc->verbose)
		printf("CHANNEL: %s-%s%s\n",
		       (char *)source_node,
		       (char *)dest_node,
		       bidir ? " bidir": "");

	/* Privkey is AAAA... or BBBB... */
	memset(&s, ((char *)source_node)[0], sizeof(s));
	memset(&d, ((char *)dest_node)[0], sizeof(d));
	pubkey_from_privkey(&s, &src_pub);
	pubkey_from_privkey(&d, &dst_pub);
	node_id_from_pubkey(&src, &src_pub);
	node_id_from_pubkey(&dst, &dst_pub);

	/* Order announcement in DER order */
	/* Default scid is: <a>x<b>x1 */
	if (node_id_cmp(&src, &dst) < 0) {
		a = &src;
		b = &dst;
		if (!mk_short_channel_id(&scid, s.secret.data[0], d.secret.data[0], 1))
			abort();
		channel_flags = 0;
	} else {
		a = &dst;
		b = &src;
		if (!mk_short_channel_id(&scid, d.secret.data[0], s.secret.data[0], 1))
			abort();
		channel_flags = 1;
	}

	for (size_t i = 0; i < tal_count(labels); i++) {
		const char *v = strchr(labels[i], '=') + 1;
		if (strstarts(labels[i], "cap=")) {
			if (!parse_amount_sat(&cap, v, strlen(v)))
				errx(1, "Bad cap %s", labels[i]);
		} else if (strstarts(labels[i], "scid=")) {
			if (!short_channel_id_from_str(v, strlen(v),
						       &scid))
				errx(1, "Bad scid %s", labels[i]);
		} else if (strstarts(labels[i], "ctlv=")) {
			cltv_expiry_delta = atoi(v);
		} else if (strstarts(labels[i], "htlcmin=")) {
			if (!parse_amount_msat(&htlc_minimum_msat, v, strlen(v)))
				errx(1, "Bad htlc_minimum_msat %s", labels[i]);
		} else if (strstarts(labels[i], "htlcmax=")) {
			if (!parse_amount_msat(&htlc_maximum_msat, v, strlen(v)))
				errx(1, "Bad htlc_maximum_msat %s", labels[i]);
		} else if (strstarts(labels[i], "feebase=")) {
			fee_base_msat = atoi(v);
		} else if (strstarts(labels[i], "feeppm=")) {
			fee_proportional_millionths = atoi(v);
		} else
			errx(1, "unknown label %s", labels[i]);
	}

	/* Default max is channel capacity */
	if (amount_msat_eq(htlc_maximum_msat, AMOUNT_MSAT(0))) {
		if (!amount_sat_to_msat(&htlc_maximum_msat, cap)) {
			errx(1, "Invalid cap");
		}
	}

	/* Signatures are unchecked, so we use dummy ones */
	memset(&sig, 0, sizeof(sig));

	/* Same with chain_hash */
	memset(&chain_hash, 0, sizeof(chain_hash));

	if (!have_channel(gc->channels, &scid)) {
		add_to_gossmap(gc,
			       towire_channel_announcement(tmpctx,
							   &sig, &sig, &sig, &sig,
							   NULL, &chain_hash, &scid, a, b,
							   /* Dummy bitcoin keys */
							   &src_pub, &src_pub));
		add_to_gossmap(gc,
			       towire_gossip_store_channel_amount(tmpctx,
								  cap));
		tal_arr_expand(&gc->channels, scid);
	}

	add_to_gossmap(gc,
		       towire_channel_update(tmpctx, &sig, &chain_hash, &scid,
					     time(NULL), 0, channel_flags,
					     cltv_expiry_delta,
					     htlc_minimum_msat,
					     fee_base_msat,
					     fee_proportional_millionths,
					     htlc_maximum_msat));
	if (bidir) {
		channel_flags ^= ROUTING_FLAGS_DIRECTION;

		add_to_gossmap(gc,
			       towire_channel_update(tmpctx, &sig, &chain_hash, &scid,
						     time(NULL), 0, channel_flags,
						     cltv_expiry_delta,
						     htlc_minimum_msat,
						     fee_base_msat,
						     fee_proportional_millionths,
						     htlc_maximum_msat));
	}

	if (!have_node(gc->nodes, &src)) {
		u8 alias[32];
		strncpy((char *)alias, source_node, 32);
		tal_arr_expand(&gc->nodes, src);
		add_to_gossmap(gc,
			       towire_node_announcement(tmpctx, &sig, NULL,
							time(NULL),
							&src,
							alias,
							alias, NULL, NULL));
	}
	if (!have_node(gc->nodes, &dst)) {
		u8 alias[32];
		strncpy((char *)alias, dest_node, 32);
		tal_arr_expand(&gc->nodes, dst);
		add_to_gossmap(gc,
			       towire_node_announcement(tmpctx, &sig, NULL,
							time(NULL),
							&dst,
							alias,
							alias, NULL, NULL));
	}
        return NULL;
}

static void normalize(char **line)
{
	size_t ntabs = strcount(*line, "\t"), i, off;
	char *untabbed;

	if (!ntabs) {
		untabbed = *line;
		goto trim_front;
	}

	/* Worst case */
	untabbed = tal_arr(tmpctx, char, strlen(*line) + 7 * ntabs + 1);
	memset(untabbed, ' ', tal_count(untabbed));

	for (i = 0, off = 0; (*line)[i]; i++) {
		/* Expand tabs */
		if ((*line)[i] == '\t') {
			/* Spaces already there, skip over */
			off += 8 - (off % 8);
		} else {
			untabbed[off++] = (*line)[i];
		}
	}
	untabbed[off] = '\0';

trim_front:
	off = strspn(untabbed, " ");
	/* Move to front, including nul term */
	memmove(untabbed, untabbed + off, strlen(untabbed) - off + 1);
	*line = untabbed;
}

static void load_graph_comments(char **argv)
{
	/* Find comment in this file describing graphname, turn it
	 * into a gossmap, with amounts in sats. eg: */
	/* GRAPH abc
	 * A<------------>B
	 *    scid=1x2x3
	 *    cap=20000sat
	 */
	bool verbose;
	char **lines;
	const char *err;

	verbose = (argv[1] && streq(argv[1], "-v"));
	lines = tal_strsplit(tmpctx,
			     grab_file(tmpctx,
				       tal_fmt(tmpctx, "%s.c", argv[0])),
			     "\n", STR_EMPTY_OK);

	for (size_t i = 0; lines[i]; i++)
		normalize(&lines[i]);

	graph_comments = tal_arr(tmpctx, struct graph_comment *,0);
	for (size_t i = 0; lines[i]; i++) {
		struct graph_comment *gc;
		size_t glen;
		char *graph;

		if (!strstarts(lines[i], "/* GRAPH "))
			continue;
		gc = tal(graph_comments, struct graph_comment);
		gc->verbose = verbose;
		gc->name = tal_strdup(gc, lines[i] + strlen("/* GRAPH "));
		/* Header is simply 11, for now. */
		gc->gossmap = tal_arrz(gc, u8, 1);
		gc->gossmap[0] = 11;
		gc->nodes = tal_arr(gc, struct node_id, 0);
		gc->channels = tal_arr(gc, struct short_channel_id, 0);
		graph = tal_strdup(tmpctx, "");
		glen = 0;
		for (++i; lines[i]; i++) {
			size_t slen;
			if (streq(lines[i], "*/"))
				break;
			/* Omit leading * */
			if (lines[i][0] == '*')
				lines[i]++;
			slen = strlen(lines[i]);
			tal_resize(&graph, glen + slen + 1);
			memcpy(graph + glen, lines[i], slen);
			glen += slen;
			graph[glen++] = '\n';
		}
		/* NUL terminator */
		tal_resize(&graph, glen + 1);
		graph[glen] = '\0';
		if (gc->verbose)
			printf("Graph %s:\n%s", gc->name, graph);
		err = ungraph(tmpctx, graph, add_node, add_edge, gc);
		if (err)
			errx(1, "Cannot parse graph %s: %s", gc->name, err);
		tal_arr_expand(&graph_comments, gc);
	}
}

static struct graph_comment *find_graph_comment(const char *graphname)
{
	for (size_t i = 0; i < tal_count(graph_comments); i++) {
		if (streq(graph_comments[i]->name, graphname))
			return graph_comments[i];
	}
	errx(1, "Could not find graph %s", graphname);
}

const char *gosstest_from_comment(const tal_t *ctx,
				  char **argv,
				  const char *graphname)
{
	struct graph_comment *gc;
	char *gcfile;
	int fd;

	if (!graph_comments)
		load_graph_comments(argv);

	gc = find_graph_comment(graphname);

	gcfile = tal_strdup(ctx, "/tmp/gosstest_from_comment.XXXXXX");
	fd = mkstemp(gcfile);
	if (fd < 0)
		err(1, "Making gossmap tempfile");

	if (!write_all(fd, gc->gossmap, tal_bytelen(gc->gossmap)))
		err(1, "Writing gossmap tempfile");

	close(fd);
	return gcfile;
}

void gosstest_privkey(const char *name, struct privkey *privkey)
{
	/* Privkey is AAAA... or BBBB... */
	memset(privkey, name[0], sizeof(*privkey));
}

void gosstest_node_id(const char *name, struct node_id *id)
{
	struct privkey privkey;
	struct pubkey pubkey;

	gosstest_privkey(name, &privkey);
	pubkey_from_privkey(&privkey, &pubkey);
	node_id_from_pubkey(id, &pubkey);
}
