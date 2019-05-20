#include "gossip_store.h"

#include <ccan/array_size/array_size.h>
#include <ccan/crc/crc.h>
#include <ccan/endian/endian.h>
#include <ccan/noerr/noerr.h>
#include <ccan/read_write_all/read_write_all.h>
#include <common/gossip_store.h>
#include <common/status.h>
#include <common/utils.h>
#include <errno.h>
#include <fcntl.h>
#include <gossipd/gen_gossip_peerd_wire.h>
#include <gossipd/gen_gossip_store.h>
#include <gossipd/gen_gossip_wire.h>
#include <stdio.h>
#include <sys/uio.h>
#include <unistd.h>
#include <wire/gen_peer_wire.h>
#include <wire/wire.h>

#define GOSSIP_STORE_FILENAME "gossip_store"
#define GOSSIP_STORE_TEMP_FILENAME "gossip_store.tmp"

struct gossip_store {
	/* This is false when we're loading */
	bool writable;

	int fd;
	u8 version;

	/* Offset of current EOF */
	u64 len;

	/* Counters for entries in the gossip_store entries. This is used to
	 * decide whether we should rewrite the on-disk store or not */
	size_t count;

	/* Handle to the routing_state to retrieve additional information,
	 * should it be needed */
	struct routing_state *rstate;

	/* Disable compaction if we encounter an error during a prior
	 * compaction */
	bool disable_compaction;
};

static void gossip_store_destroy(struct gossip_store *gs)
{
	close(gs->fd);
}

static bool append_msg(int fd, const u8 *msg, u32 timestamp, u64 *len)
{
	struct gossip_hdr hdr;
	u32 msglen;
	struct iovec iov[2];

	msglen = tal_count(msg);
	hdr.len = cpu_to_be32(msglen);
	hdr.crc = cpu_to_be32(crc32c(timestamp, msg, msglen));
	hdr.timestamp = cpu_to_be32(timestamp);

	if (len)
		*len += sizeof(hdr) + msglen;

	/* Use writev so it will appear in store atomically */
	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = (void *)msg;
	iov[1].iov_len = msglen;
	return writev(fd, iov, ARRAY_SIZE(iov)) == sizeof(hdr) + msglen;
}

#ifdef COMPAT_V070
static u32 get_timestamp(const u8 *msg)
{
	secp256k1_ecdsa_signature sig;
	struct bitcoin_blkid chain_hash;
	struct short_channel_id scid;
	u32 timestamp;
	u8 u8_ignore;
	u16 u16_ignore;
	u32 u32_ignore;
	struct amount_msat msat;
	struct node_id id;
	u8 rgb_color[3], alias[32];
	u8 *features, *addresses;

	if (fromwire_channel_update(msg, &sig, &chain_hash, &scid,
				    &timestamp, &u8_ignore, &u8_ignore,
				    &u16_ignore, &msat, &u32_ignore,
				    &u32_ignore))
		return timestamp;

	if (fromwire_node_announcement(tmpctx, msg, &sig, &features, &timestamp,
				       &id, rgb_color, alias, &addresses))
		return timestamp;

	return 0;
}

/* Now we have an update, we have a timestamp for the previous announce */
static bool flush_channel_announce(int outfd, u8 **channel_announce,
				   struct amount_sat sat,
				   u32 timestamp)
{
	u8 *amt;
	if (!*channel_announce)
		return true;

	if (!append_msg(outfd, *channel_announce, timestamp, NULL))
		return false;

	amt = towire_gossip_store_channel_amount(tmpctx, sat);
	if (!append_msg(outfd, amt, 0, NULL))
		return false;
	*channel_announce = tal_free(*channel_announce);
	return true;
}

static bool upgrade_gs(struct gossip_store *gs)
{
	beint32_t oldhdr[2];
	size_t off = gs->len;
	int newfd;
	u8 *channel_announce = NULL;
	struct amount_sat channel_sat;
	const u8 newversion = GOSSIP_STORE_VERSION;

	if (gs->version < 3 || gs->version > 4)
		return false;

	newfd = open(GOSSIP_STORE_TEMP_FILENAME,
		     O_RDWR|O_APPEND|O_CREAT|O_TRUNC,
		     0600);
	if (newfd < 0) {
		status_broken("gossip_store: can't create temp %s: %s",
			       GOSSIP_STORE_TEMP_FILENAME, strerror(errno));
		return false;
	}

	if (!write_all(newfd, &newversion, sizeof(newversion))) {
		status_broken("gossip_store: can't write header to %s: %s",
			       GOSSIP_STORE_TEMP_FILENAME, strerror(errno));
		close(newfd);
		return false;
	}

	while (pread(gs->fd, oldhdr, sizeof(oldhdr), off) == sizeof(oldhdr)) {
		u32 msglen, checksum;
		u8 *msg, *gossip_msg;
		u32 timestamp;

		msglen = be32_to_cpu(oldhdr[0]);
		checksum = be32_to_cpu(oldhdr[1]);
		msg = tal_arr(tmpctx, u8, msglen);

		if (pread(gs->fd, msg, msglen, off+sizeof(oldhdr)) != msglen) {
			status_unusual("gossip_store: truncated file @%zu?",
				       off + sizeof(oldhdr));
			goto fail;
		}

		if (checksum != crc32c(0, msg, msglen)) {
			status_unusual("gossip_store: checksum failed");
			goto fail;
		}

		/* v3 needs unwrapping, v4 just needs timestamp info. */
		if (fromwire_gossip_store_v3_channel_announcement(msg, msg,
								  &gossip_msg,
								  &channel_sat)) {
			channel_announce = tal_steal(gs, gossip_msg);
			goto next;
		} else if (fromwire_peektype(msg) == WIRE_CHANNEL_ANNOUNCEMENT) {
			channel_announce = tal_steal(gs, msg);
			goto next;
		} else if (fromwire_gossip_store_channel_amount(msg,
								&channel_sat)) {
			/* We'll write this after the channel_announce */
			goto next;
		}

		/* v3 channel_update: use this timestamp for channel_announce
		 * then unwrap. */
		if (fromwire_gossip_store_v3_channel_update(msg, msg,
							    &gossip_msg)) {
			timestamp = get_timestamp(gossip_msg);
			if (!flush_channel_announce(newfd,
						    &channel_announce,
						    channel_sat,
						    timestamp)
			    || !append_msg(newfd, gossip_msg, timestamp, NULL))
				goto write_fail;
		/* Other v3 messages just need unwrapping */
		} else if (fromwire_gossip_store_v3_node_announcement(msg,
								      msg,
								      &gossip_msg)
			   || fromwire_gossip_store_v3_local_add_channel(msg,
									 msg,
									 &gossip_msg)) {
			timestamp = get_timestamp(gossip_msg);
			if (!append_msg(newfd, gossip_msg, timestamp, NULL))
				goto write_fail;
		} else {
			timestamp = get_timestamp(msg);
			if (fromwire_peektype(msg) == WIRE_CHANNEL_UPDATE
			    && !flush_channel_announce(newfd,
						       &channel_announce,
						       channel_sat,
						       timestamp))
				goto write_fail;
			if (!append_msg(newfd, msg, timestamp, NULL))
				goto write_fail;
		}
	next:
		off += sizeof(oldhdr) + msglen;
		clean_tmpctx();
	}

	if (rename(GOSSIP_STORE_TEMP_FILENAME, GOSSIP_STORE_FILENAME) == -1) {
		status_broken(
		    "Error swapping compacted gossip_store into place: %s",
		    strerror(errno));
		goto fail;
	}

	status_info("Upgraded gossip_store from version %u to %u",
		    gs->version, newversion);
	close(gs->fd);
	gs->fd = newfd;
	gs->version = newversion;
	return true;

write_fail:
	status_unusual("gossip_store: write failed for upgrade: %s",
		       strerror(errno));
fail:
	close(newfd);
	return false;
}
#else /* ! COMPAT_V070 */
static bool upgrade_gs(struct gossip_store *gs)
{
	return false;
}
#endif /* COMPAT_V070 */

struct gossip_store *gossip_store_new(struct routing_state *rstate)
{
	struct gossip_store *gs = tal(rstate, struct gossip_store);
	gs->count = 0;
	gs->writable = true;
	gs->fd = open(GOSSIP_STORE_FILENAME, O_RDWR|O_APPEND|O_CREAT, 0600);
	gs->rstate = rstate;
	gs->disable_compaction = false;
	gs->len = sizeof(gs->version);

	tal_add_destructor(gs, gossip_store_destroy);

	/* Try to read the version, write it if this is a new file, or truncate
	 * if the version doesn't match */
	if (read(gs->fd, &gs->version, sizeof(gs->version))
	    == sizeof(gs->version)) {
		/* Version match?  All good */
		if (gs->version == GOSSIP_STORE_VERSION)
			return gs;

		if (upgrade_gs(gs))
			return gs;

		status_unusual("Gossip store version %u not %u: removing",
			       gs->version, GOSSIP_STORE_VERSION);
		if (ftruncate(gs->fd, 0) != 0)
			status_failed(STATUS_FAIL_INTERNAL_ERROR,
				      "Truncating store: %s", strerror(errno));
	}
	/* Empty file, write version byte */
	gs->version = GOSSIP_STORE_VERSION;
	if (write(gs->fd, &gs->version, sizeof(gs->version))
	    != sizeof(gs->version))
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "Writing version to store: %s", strerror(errno));
	return gs;
}

/* Returns bytes transferred, or 0 on error */
static size_t transfer_store_msg(int from_fd, size_t from_off, int to_fd,
				 int *type)
{
	struct gossip_hdr hdr;
	u32 msglen;
	u8 *msg;
	const u8 *p;
	size_t tmplen;

	if (pread(from_fd, &hdr, sizeof(hdr), from_off) != sizeof(hdr)) {
		status_broken("Failed reading header from to gossip store @%zu"
			      ": %s",
			      from_off, strerror(errno));
		return 0;
	}

	msglen = be32_to_cpu(hdr.len);
	if (msglen & GOSSIP_STORE_LEN_DELETED_BIT) {
		status_broken("Can't transfer deleted msg from gossip store @%zu",
			      from_off);
		return 0;
	}

	/* FIXME: Reuse buffer? */
	msg = tal_arr(tmpctx, u8, sizeof(hdr) + msglen);
	memcpy(msg, &hdr, sizeof(hdr));
	if (pread(from_fd, msg + sizeof(hdr), msglen, from_off + sizeof(hdr))
	    != msglen) {
		status_broken("Failed reading %u from to gossip store @%zu"
			      ": %s",
			      msglen, from_off, strerror(errno));
		return 0;
	}

	if (write(to_fd, msg, msglen + sizeof(hdr)) != msglen + sizeof(hdr)) {
		status_broken("Failed writing to gossip store: %s",
			      strerror(errno));
		return 0;
	}

	/* Can't use peektype here, since we have header on front */
	p = msg + sizeof(hdr);
	tmplen = msglen;
	*type = fromwire_u16(&p, &tmplen);
	if (!p)
		*type = -1;
	tal_free(msg);
	return sizeof(hdr) + msglen;
}

/* Local unannounced channels don't appear in broadcast map, but we need to
 * remember them anyway, so we manually append to the store.
 *
 * Note these do *not* add to gs->count, since that's compared with
 * the broadcast map count.
*/
static bool add_local_unnannounced(int in_fd, int out_fd,
				   struct node *self,
				   u64 *len)
{
	struct chan_map_iter i;
	struct chan *c;

	for (c = first_chan(self, &i); c; c = next_chan(self, &i)) {
		struct node *peer = other_node(self, c);
		const u8 *msg;

		/* Ignore already announced. */
		if (is_chan_public(c))
			continue;

		msg = towire_gossipd_local_add_channel(tmpctx, &c->scid,
						       &peer->id, c->sat);
		if (!append_msg(out_fd, msg, 0, len))
			return false;

		for (size_t i = 0; i < 2; i++) {
			size_t len_with_header;
			int type;

			if (!is_halfchan_defined(&c->half[i]))
				continue;

			len_with_header = transfer_store_msg(in_fd,
							     c->half[i].bcast.index,
							     out_fd,
							     &type);
			if (!len_with_header)
				return false;

			c->half[i].bcast.index = *len;

			*len += len_with_header;
		}
	}

	return true;
}

/**
 * Rewrite the on-disk gossip store, compacting it along the way
 *
 * Creates a new file, writes all the updates from the `broadcast_state`, and
 * then atomically swaps the files.
 *
 * Returns the amount of shrinkage in @offset on success, otherwise @offset
 * is unchanged.
 */
bool gossip_store_compact(struct gossip_store *gs,
			  struct broadcast_state **bs,
			  u32 *offset)
{
	size_t count = 0;
	int fd;
	struct node *self;
	u64 len = sizeof(gs->version);
	struct broadcastable *bcast;
	struct broadcast_state *oldb = *bs;
	struct broadcast_state *newb;
	u32 idx = 0;

	if (gs->disable_compaction)
		return false;

	assert(oldb);
	status_trace(
	    "Compacting gossip_store with %zu entries, %zu of which are stale",
	    gs->count, gs->count - oldb->count);

	newb = new_broadcast_state(gs->rstate, gs, oldb->peers);
	fd = open(GOSSIP_STORE_TEMP_FILENAME, O_RDWR|O_APPEND|O_CREAT, 0600);

	if (fd < 0) {
		status_broken(
		    "Could not open file for gossip_store compaction");
		goto disable;
	}

	if (write(fd, &gs->version, sizeof(gs->version))
	    != sizeof(gs->version)) {
		status_broken("Writing version to store: %s", strerror(errno));
		goto unlink_disable;
	}

	/* Copy entries one at a time. */
	while ((bcast = next_broadcast_raw(oldb, &idx)) != NULL) {
		u64 old_index = bcast->index;
		int msgtype;
		size_t msg_len;

		msg_len = transfer_store_msg(gs->fd, bcast->index, fd, &msgtype);
		if (msg_len == 0)
			goto unlink_disable;

		broadcast_del(oldb, bcast);
		bcast->index = len;
		insert_broadcast_nostore(newb, bcast);
		len += msg_len;
		count++;

		/* channel_announcement always followed by amount: copy too */
		if (msgtype == WIRE_CHANNEL_ANNOUNCEMENT) {
			msg_len = transfer_store_msg(gs->fd, old_index + msg_len,
						     fd, &msgtype);
			if (msg_len == 0)
				goto unlink_disable;
			if (msgtype != WIRE_GOSSIP_STORE_CHANNEL_AMOUNT) {
				status_broken("gossip_store: unexpected type %u",
					      msgtype);
				goto unlink_disable;
			}
			len += msg_len;
			/* This amount field doesn't add to count. */
		}
	}

	/* Local unannounced channels are not in the store! */
	self = get_node(gs->rstate, &gs->rstate->local_id);
	if (self && !add_local_unnannounced(gs->fd, fd, self, &len)) {
		status_broken("Failed writing unannounced to gossip store: %s",
			      strerror(errno));
		goto unlink_disable;
	}

	if (rename(GOSSIP_STORE_TEMP_FILENAME, GOSSIP_STORE_FILENAME) == -1) {
		status_broken(
		    "Error swapping compacted gossip_store into place: %s",
		    strerror(errno));
		goto unlink_disable;
	}

	status_trace(
	    "Compaction completed: dropped %zu messages, new count %zu, len %"PRIu64,
	    gs->count - count, count, len);
	gs->count = count;
	*offset = gs->len - len;
	gs->len = len;
	close(gs->fd);
	gs->fd = fd;

	tal_free(oldb);
	*bs = newb;
	return true;

unlink_disable:
	unlink(GOSSIP_STORE_TEMP_FILENAME);
disable:
	status_trace("Encountered an error while compacting, disabling "
		     "future compactions.");
	gs->disable_compaction = true;
	tal_free(newb);
	return false;
}

bool gossip_store_maybe_compact(struct gossip_store *gs,
				struct broadcast_state **bs,
				u32 *offset)
{
	*offset = 0;

	/* Don't compact while loading! */
	if (!gs->writable)
		return false;
	if (gs->count < 1000)
		return false;
	if (gs->count < (*bs)->count * 1.25)
		return false;

	return gossip_store_compact(gs, bs, offset);
}

u64 gossip_store_add(struct gossip_store *gs, const u8 *gossip_msg,
		     u32 timestamp,
		     const u8 *addendum)
{
	u64 off = gs->len;

	/* Should never get here during loading! */
	assert(gs->writable);

	if (!append_msg(gs->fd, gossip_msg, timestamp, &gs->len)) {
		status_broken("Failed writing to gossip store: %s",
			      strerror(errno));
		return 0;
	}
	if (addendum && !append_msg(gs->fd, addendum, 0, &gs->len)) {
		status_broken("Failed writing addendum to gossip store: %s",
			      strerror(errno));
		return 0;
	}

	gs->count++;
	return off;
}

u64 gossip_store_add_private_update(struct gossip_store *gs, const u8 *update)
{
	/* A local update for an unannounced channel: not broadcastable, but
	 * otherwise the same as a normal channel_update */
	const u8 *pupdate = towire_gossip_store_private_update(tmpctx, update);
	return gossip_store_add(gs, pupdate, 0, NULL);
}

void gossip_store_delete(struct gossip_store *gs,
			 struct broadcastable *bcast,
			 int type)
{
	beint32_t belen;
	int flags;

	if (!bcast->index)
		return;

	/* Should never get here during loading! */
	assert(gs->writable);

#if DEVELOPER
	const u8 *msg = gossip_store_get(tmpctx, gs, bcast->index);
	assert(fromwire_peektype(msg) == type);
#endif
	if (pread(gs->fd, &belen, sizeof(belen), bcast->index) != sizeof(belen))
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "Failed reading len to delete @%u: %s",
			      bcast->index, strerror(errno));

	assert((be32_to_cpu(belen) & GOSSIP_STORE_LEN_DELETED_BIT) == 0);
	belen |= cpu_to_be32(GOSSIP_STORE_LEN_DELETED_BIT);
	/* From man pwrite(2):
	 *
	 * BUGS
	 *  POSIX requires that opening a file with the O_APPEND flag  should
	 *  have no  effect  on the location at which pwrite() writes data.
	 *  However, on Linux, if a file is opened with O_APPEND, pwrite()
	 *  appends data to  the end of the file, regardless of the value of
	 *  offset.
	 */
	flags = fcntl(gs->fd, F_GETFL);
	fcntl(gs->fd, F_SETFL, flags & ~O_APPEND);
	if (pwrite(gs->fd, &belen, sizeof(belen), bcast->index) != sizeof(belen))
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "Failed writing len to delete @%u: %s",
			      bcast->index, strerror(errno));
	fcntl(gs->fd, F_SETFL, flags);
}

const u8 *gossip_store_get(const tal_t *ctx,
			   struct gossip_store *gs,
			   u64 offset)
{
	struct gossip_hdr hdr;
	u32 msglen, checksum;
	u8 *msg;

	if (offset == 0)
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "gossip_store: can't access offset %"PRIu64,
			      offset);
	if (pread(gs->fd, &hdr, sizeof(hdr), offset) != sizeof(hdr)) {
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "gossip_store: can't read hdr offset %"PRIu64
			      "/%"PRIu64": %s",
			      offset, gs->len, strerror(errno));
	}

	/* FIXME: We should skip over these deleted entries! */
	msglen = be32_to_cpu(hdr.len) & ~GOSSIP_STORE_LEN_DELETED_BIT;
	checksum = be32_to_cpu(hdr.crc);
	msg = tal_arr(ctx, u8, msglen);
	if (pread(gs->fd, msg, msglen, offset + sizeof(hdr)) != msglen)
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "gossip_store: can't read len %u offset %"PRIu64
			      "/%"PRIu64, msglen, offset, gs->len);

	if (checksum != crc32c(be32_to_cpu(hdr.timestamp), msg, msglen))
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "gossip_store: bad checksum offset %"PRIu64": %s",
			      offset, tal_hex(tmpctx, msg));

	return msg;
}

const u8 *gossip_store_get_private_update(const tal_t *ctx,
					  struct gossip_store *gs,
					  u64 offset)
{
	const u8 *pmsg = gossip_store_get(tmpctx, gs, offset);
	u8 *msg;

	if (!fromwire_gossip_store_private_update(ctx, pmsg, &msg))
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "Failed to decode private update @%"PRIu64": %s",
			      offset, tal_hex(tmpctx, pmsg));
	return msg;
}

int gossip_store_readonly_fd(struct gossip_store *gs)
{
	int fd = open(GOSSIP_STORE_FILENAME, O_RDONLY);

	/* Skip over version header */
	if (fd != -1 && lseek(fd, 1, SEEK_SET) != 1) {
		close_noerr(fd);
		fd = -1;
	}
	return fd;
}

void gossip_store_load(struct routing_state *rstate, struct gossip_store *gs)
{
	struct gossip_hdr hdr;
	u32 msglen, checksum;
	u8 *msg;
	struct amount_sat satoshis;
	struct short_channel_id scid;
	const char *bad;
	size_t stats[] = {0, 0, 0, 0};
	struct timeabs start = time_now();
	const u8 *chan_ann = NULL;
	u64 chan_ann_off;

	gs->writable = false;
	while (pread(gs->fd, &hdr, sizeof(hdr), gs->len) == sizeof(hdr)) {
		msglen = be32_to_cpu(hdr.len) & ~GOSSIP_STORE_LEN_DELETED_BIT;
		checksum = be32_to_cpu(hdr.crc);
		msg = tal_arr(tmpctx, u8, msglen);

		if (pread(gs->fd, msg, msglen, gs->len+sizeof(hdr)) != msglen) {
			status_unusual("gossip_store: truncated file?");
			goto truncate_nomsg;
		}

		if (checksum != crc32c(be32_to_cpu(hdr.timestamp), msg, msglen)) {
			bad = "Checksum verification failed";
			goto truncate;
		}

		/* Skip deleted entries */
		if (be32_to_cpu(hdr.len) & GOSSIP_STORE_LEN_DELETED_BIT)
			goto next;

		switch (fromwire_peektype(msg)) {
		case WIRE_GOSSIP_STORE_CHANNEL_AMOUNT:
			if (!fromwire_gossip_store_channel_amount(msg,
								  &satoshis)) {
				bad = "Bad gossip_store_channel_amount";
				goto truncate;
			}
			/* Previous channel_announcement may have been deleted */
			if (!chan_ann)
				break;
			if (!routing_add_channel_announcement(rstate,
							      take(chan_ann),
							      satoshis,
							      chan_ann_off)) {
				bad = "Bad channel_announcement";
				goto truncate;
			}
			chan_ann = NULL;
			stats[0]++;
			break;
		case WIRE_CHANNEL_ANNOUNCEMENT:
			if (chan_ann) {
				bad = "channel_announcement without amount";
				goto truncate;
			}
			/* Save for channel_amount (next msg) */
			chan_ann = tal_steal(gs, msg);
			chan_ann_off = gs->len;
			break;
		case WIRE_GOSSIP_STORE_PRIVATE_UPDATE:
			if (!fromwire_gossip_store_private_update(tmpctx, msg, &msg)) {
				bad = "invalid gossip_store_private_update";
				goto truncate;
			}
			/* fall thru */
		case WIRE_CHANNEL_UPDATE:
			if (!routing_add_channel_update(rstate,
							take(msg), gs->len)) {
				bad = "Bad channel_update";
				goto truncate;
			}
			stats[1]++;
			break;
		case WIRE_NODE_ANNOUNCEMENT:
			if (!routing_add_node_announcement(rstate,
							   take(msg), gs->len)) {
				bad = "Bad node_announcement";
				goto truncate;
			}
			stats[2]++;
			break;
#ifdef COMPAT_V070
		/* These are easy to handle, so we just do so */
		case WIRE_GOSSIP_STORE_V4_CHANNEL_DELETE:
			if (!fromwire_gossip_store_v4_channel_delete(msg, &scid)) {
				bad = "Bad channel_delete";
				goto truncate;
			}
			struct chan *c = get_channel(rstate, &scid);
			if (!c) {
				bad = "Bad channel_delete scid";
				goto truncate;
			}
			free_chan(rstate, c);
			stats[3]++;
			break;
#endif
		case WIRE_GOSSIPD_LOCAL_ADD_CHANNEL:
			if (!handle_local_add_channel(rstate, msg, gs->len)) {
				bad = "Bad local_add_channel";
				goto truncate;
			}
			break;
		default:
			bad = "Unknown message";
			goto truncate;
		}

		if (fromwire_peektype(msg) != WIRE_GOSSIP_STORE_CHANNEL_AMOUNT)
			gs->count++;
	next:
		gs->len += sizeof(hdr) + msglen;
		clean_tmpctx();
	}
	goto out;

truncate:
	status_unusual("gossip_store: %s (%s) truncating to %"PRIu64,
		       bad, tal_hex(msg, msg), gs->len);
truncate_nomsg:
	/* FIXME: We would like to truncate to known_good, except we would
	 * miss channel_delete msgs.  If we put block numbers into the store
	 * as we process them, we can know how far we need to roll back if we
	 * truncate the store */
	if (ftruncate(gs->fd, gs->len) != 0)
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "Truncating store: %s", strerror(errno));
out:
	status_trace("total store load time: %"PRIu64" msec",
		     time_to_msec(time_between(time_now(), start)));
	status_trace("gossip_store: Read %zu/%zu/%zu/%zu cannounce/cupdate/nannounce/cdelete from store in %"PRIu64" bytes",
		     stats[0], stats[1], stats[2], stats[3],
		     gs->len);
	gs->writable = true;
}
