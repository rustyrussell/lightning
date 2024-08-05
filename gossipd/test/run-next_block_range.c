#include "config.h"
#include "../seeker.c"
#include <ccan/err/err.h>
#include <common/blinding.h>
#include <common/channel_type.h>
#include <common/ecdh.h>
#include <common/json_stream.h>
#include <common/onionreply.h>
#include <common/sciddir_or_pubkey.h>
#include <common/setup.h>
#include <common/wireaddr.h>
#include <stdio.h>

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
/* Generated stub for find_peer */
struct peer *find_peer(struct daemon *daemon UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "find_peer called!\n"); abort(); }
/* Generated stub for first_random_peer */
struct peer *first_random_peer(struct daemon *daemon UNNEEDED,
			       struct peer_node_id_map_iter *it UNNEEDED)
{ fprintf(stderr, "first_random_peer called!\n"); abort(); }
/* Generated stub for fromwire_sciddir_or_pubkey */
void fromwire_sciddir_or_pubkey(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
				struct sciddir_or_pubkey *sciddpk UNNEEDED)
{ fprintf(stderr, "fromwire_sciddir_or_pubkey called!\n"); abort(); }
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
/* Generated stub for gossmap_manage_get_gossmap */
struct gossmap *gossmap_manage_get_gossmap(struct gossmap_manage *gm UNNEEDED)
{ fprintf(stderr, "gossmap_manage_get_gossmap called!\n"); abort(); }
/* Generated stub for gossmap_max_node_idx */
u32 gossmap_max_node_idx(const struct gossmap *map UNNEEDED)
{ fprintf(stderr, "gossmap_max_node_idx called!\n"); abort(); }
/* Generated stub for gossmap_node_byidx */
struct gossmap_node *gossmap_node_byidx(const struct gossmap *map UNNEEDED, u32 idx UNNEEDED)
{ fprintf(stderr, "gossmap_node_byidx called!\n"); abort(); }
/* Generated stub for gossmap_nth_chan */
struct gossmap_chan *gossmap_nth_chan(const struct gossmap *map UNNEEDED,
				      const struct gossmap_node *node UNNEEDED,
				      u32 n UNNEEDED,
				      int *which_half UNNEEDED)
{ fprintf(stderr, "gossmap_nth_chan called!\n"); abort(); }
/* Generated stub for gossmap_nth_node */
struct gossmap_node *gossmap_nth_node(const struct gossmap *map UNNEEDED,
				      const struct gossmap_chan *chan UNNEEDED,
				      int n UNNEEDED)
{ fprintf(stderr, "gossmap_nth_node called!\n"); abort(); }
/* Generated stub for gossmap_num_chan_updates */
size_t gossmap_num_chan_updates(const struct gossmap *map UNNEEDED)
{ fprintf(stderr, "gossmap_num_chan_updates called!\n"); abort(); }
/* Generated stub for gossmap_num_chans */
size_t gossmap_num_chans(const struct gossmap *map UNNEEDED)
{ fprintf(stderr, "gossmap_num_chans called!\n"); abort(); }
/* Generated stub for gossmap_num_node_announcements */
size_t gossmap_num_node_announcements(const struct gossmap *map UNNEEDED)
{ fprintf(stderr, "gossmap_num_node_announcements called!\n"); abort(); }
/* Generated stub for memleak_scan_intmap_ */
void memleak_scan_intmap_(struct htable *memtable UNNEEDED, const struct intmap *m UNNEEDED)
{ fprintf(stderr, "memleak_scan_intmap_ called!\n"); abort(); }
/* Generated stub for new_reltimer_ */
struct oneshot *new_reltimer_(struct timers *timers UNNEEDED,
			      const tal_t *ctx UNNEEDED,
			      struct timerel expire UNNEEDED,
			      void (*cb)(void *) UNNEEDED, void *arg UNNEEDED)
{ fprintf(stderr, "new_reltimer_ called!\n"); abort(); }
/* Generated stub for next_random_peer */
struct peer *next_random_peer(struct daemon *daemon UNNEEDED,
			      const struct peer *first UNNEEDED,
			      struct peer_node_id_map_iter *it UNNEEDED)
{ fprintf(stderr, "next_random_peer called!\n"); abort(); }
/* Generated stub for query_channel_range */
bool query_channel_range(struct daemon *daemon UNNEEDED,
			 struct peer *peer UNNEEDED,
			 u32 first_blocknum UNNEEDED, u32 number_of_blocks UNNEEDED,
			 enum query_option_flags qflags UNNEEDED,
			 void (*cb)(struct peer *peer_ UNNEEDED,
				    u32 first_blocknum_ UNNEEDED,
				    u32 number_of_blocks_ UNNEEDED,
				    const struct range_query_reply *replies_))
{ fprintf(stderr, "query_channel_range called!\n"); abort(); }
/* Generated stub for query_short_channel_ids */
bool query_short_channel_ids(struct daemon *daemon UNNEEDED,
			     struct peer *peer UNNEEDED,
			     const struct short_channel_id *scids UNNEEDED,
			     const u8 *query_flags UNNEEDED,
			     void (*cb)(struct peer *peer_ UNNEEDED, bool complete))
{ fprintf(stderr, "query_short_channel_ids called!\n"); abort(); }
/* Generated stub for queue_peer_msg */
void queue_peer_msg(struct daemon *daemon UNNEEDED,
		    const struct node_id *peer UNNEEDED,
		    const u8 *msg TAKES UNNEEDED)
{ fprintf(stderr, "queue_peer_msg called!\n"); abort(); }
/* Generated stub for status_failed */
void status_failed(enum status_failreason code UNNEEDED,
		   const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "status_failed called!\n"); abort(); }
/* Generated stub for status_fmt */
void status_fmt(enum log_level level UNNEEDED,
		const struct node_id *peer UNNEEDED,
		const char *fmt UNNEEDED, ...)

{ fprintf(stderr, "status_fmt called!\n"); abort(); }
/* Generated stub for towire_sciddir_or_pubkey */
void towire_sciddir_or_pubkey(u8 **pptr UNNEEDED,
			      const struct sciddir_or_pubkey *sciddpk UNNEEDED)
{ fprintf(stderr, "towire_sciddir_or_pubkey called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

static void test_block_range(struct seeker *seeker,
			     u32 blockheight,
			     u32 first, u32 last,
			     ...)
{
	int start, end, num;
	u32 first_blocknum, number_of_blocks;
	va_list ap;

	seeker->daemon->current_blockheight = blockheight;
	seeker->scid_probe_start = first;
	seeker->scid_probe_end = last;

	num = last - first + 1;
	va_start(ap, last);
	while ((start = va_arg(ap, int)) != -1) {
		end = va_arg(ap, int);
		if (!next_block_range(seeker, num,
				      &first_blocknum, &number_of_blocks))
			abort();
		if (first_blocknum != start
		    || number_of_blocks != end - start + 1) {
			errx(1, "Expected %u-%u but got %u-%u",
			     start, end,
			     first_blocknum, first_blocknum+number_of_blocks-1);
		}
		num = end - start + 1;
	}
	if (next_block_range(seeker, num, &first_blocknum, &number_of_blocks))
		abort();
	va_end(ap);
}

int main(int argc, char *argv[])
{
	struct seeker *seeker = tal(NULL, struct seeker);

	common_setup(argv[0]);

	seeker->daemon = tal(seeker, struct daemon);

	/* Case where we start at beginning */
	test_block_range(seeker, 100,
			 0, 0,
			 1, 2,
			 3, 6,
			 7, 14,
			 15, 30,
			 31, 62,
			 63, 100,
			 -1);

	/* Case where we start at end */
	test_block_range(seeker, 100,
			 100, 100,
			 98, 99,
			 94, 97,
			 86, 93,
			 70, 85,
			 38, 69,
			 0, 37,
			 -1);

	/* Start in the middle. */
	test_block_range(seeker, 100,
			 50, 59,
			 30, 49,
			 0, 29,
			 60, 100,
			 -1);

	tal_free(seeker);
	common_shutdown();
}
