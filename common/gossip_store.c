#include "config.h"
#include <ccan/crc32c/crc32c.h>
#include <common/gossip_rcvd_filter.h>
#include <common/gossip_store.h>
#include <common/per_peer_state.h>
#include <common/status.h>
#include <errno.h>
#include <fcntl.h>
#include <gossipd/gossip_store_wiregen.h>
#include <inttypes.h>
#include <unistd.h>
#include <wire/peer_wire.h>

static bool timestamp_filter(const struct gossip_state *gs, u32 timestamp)
{
	/* BOLT #7:
	 *
	 *   - SHOULD send all gossip messages whose `timestamp` is greater or
	 *    equal to `first_timestamp`, and less than `first_timestamp` plus
	 *    `timestamp_range`.
	 */
	/* Note that we turn first_timestamp & timestamp_range into an inclusive range */
	return timestamp >= gs->timestamp_min
		&& timestamp <= gs->timestamp_max;
}

/* Not all the data we expected was there: rewind file */
static void failed_read(int fd, int len)
{
	if (len < 0) {
		/* Grab errno before lseek overrides it */
		const char *err = strerror(errno);
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "gossip_store: failed read @%"PRIu64": %s",
			      (u64)lseek(fd, 0, SEEK_CUR), err);
	}

	lseek(fd, -len, SEEK_CUR);
}

static void reopen_gossip_store(int *gossip_store_fd, const u8 *msg)
{
	u64 equivalent_offset;
	int newfd;

	if (!fromwire_gossip_store_ended(msg, &equivalent_offset))
		status_failed(STATUS_FAIL_GOSSIP_IO,
			      "Bad gossipd GOSSIP_STORE_ENDED msg: %s",
			      tal_hex(tmpctx, msg));

	newfd = open(GOSSIP_STORE_FILENAME, O_RDONLY);
	if (newfd < 0)
		status_failed(STATUS_FAIL_INTERNAL_ERROR,
			      "Cannot open %s: %s",
			      GOSSIP_STORE_FILENAME,
			      strerror(errno));

	status_debug("gossip_store at end, new fd moved to %"PRIu64,
		     equivalent_offset);
	lseek(newfd, equivalent_offset, SEEK_SET);

	close(*gossip_store_fd);
	*gossip_store_fd = newfd;
}

u8 *gossip_store_iter(const tal_t *ctx,
		      int *gossip_store_fd,
		      struct gossip_state *gs,
		      struct gossip_rcvd_filter *grf,
		      size_t *off)
{
	u8 *msg = NULL;

	while (!msg) {
		struct gossip_hdr hdr;
		u32 msglen, checksum, timestamp;
		bool push;
		int type, r;

		if (off)
			r = pread(*gossip_store_fd, &hdr, sizeof(hdr), *off);
		else
			r = read(*gossip_store_fd, &hdr, sizeof(hdr));
		if (r != sizeof(hdr)) {
			/* We expect a 0 read here at EOF */
			if (r != 0 && off)
				failed_read(*gossip_store_fd, r);
			return NULL;
		}

		msglen = be32_to_cpu(hdr.len);
		push = (msglen & GOSSIP_STORE_LEN_PUSH_BIT);
		msglen &= GOSSIP_STORE_LEN_MASK;

		/* Skip any deleted entries. */
		if (be32_to_cpu(hdr.len) & GOSSIP_STORE_LEN_DELETED_BIT) {
			/* Skip over it. */
			if (off)
				*off += r + msglen;
			else
				lseek(*gossip_store_fd, msglen, SEEK_CUR);
			continue;
		}

		checksum = be32_to_cpu(hdr.crc);
		timestamp = be32_to_cpu(hdr.timestamp);
		msg = tal_arr(ctx, u8, msglen);
		if (off)
			r = pread(*gossip_store_fd, msg, msglen, *off + r);
		else
			r = read(*gossip_store_fd, msg, msglen);
		if (r != msglen) {
			if (!off)
				failed_read(*gossip_store_fd, r);
			return NULL;
		}

		if (checksum != crc32c(be32_to_cpu(hdr.timestamp), msg, msglen))
			status_failed(STATUS_FAIL_INTERNAL_ERROR,
				      "gossip_store: bad checksum offset %"
				      PRIi64": %s",
				      off ? (s64)*off :
				      (s64)lseek(*gossip_store_fd,
						 0, SEEK_CUR) - msglen,
				      tal_hex(tmpctx, msg));

		/* Definitely processing it now */
		if (off)
			*off += sizeof(hdr) + msglen;

		/* Don't send back gossip they sent to us! */
		if (gossip_rcvd_filter_del(grf, msg)) {
			msg = tal_free(msg);
			continue;
		}

		type = fromwire_peektype(msg);
		if (type == WIRE_GOSSIP_STORE_ENDED)
			reopen_gossip_store(gossip_store_fd, msg);
		/* Ignore gossipd internal messages. */
		else if (type != WIRE_CHANNEL_ANNOUNCEMENT
		    && type != WIRE_CHANNEL_UPDATE
		    && type != WIRE_NODE_ANNOUNCEMENT)
			msg = tal_free(msg);
		else if (!push && !timestamp_filter(gs, timestamp))
			msg = tal_free(msg);
	}

	return msg;
}

size_t find_gossip_store_end(int gossip_store_fd, size_t off)
{
	/* We cheat and read first two bytes of message too. */
	struct {
		struct gossip_hdr hdr;
		be16 type;
	} buf;
	int r;

	while ((r = read(gossip_store_fd, &buf,
			 sizeof(buf.hdr) + sizeof(buf.type)))
	       == sizeof(buf.hdr) + sizeof(buf.type)) {
		u32 msglen = be32_to_cpu(buf.hdr.len) & GOSSIP_STORE_LEN_MASK;

		/* Don't swallow end marker! */
		if (buf.type == CPU_TO_BE16(WIRE_GOSSIP_STORE_ENDED))
			break;

		off += sizeof(buf.hdr) + msglen;
		lseek(gossip_store_fd, off, SEEK_SET);
	}
	return off;
}
