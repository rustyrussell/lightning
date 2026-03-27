#ifndef LIGHTNING_LIGHTNINGD_OPENING_CONTROL_H
#define LIGHTNING_LIGHTNINGD_OPENING_CONTROL_H
#include "config.h"
#include <ccan/build_assert/build_assert.h>
#include <ccan/compiler/compiler.h>
#include <ccan/crypto/shachain/shachain.h>
#include <ccan/crypto/siphash24/siphash24.h>
#include <ccan/list/list.h>
#include <common/channel_config.h>
#include <common/htlc.h>
#include <common/htlc_state.h>
#include <common/json_parse.h>
#include <common/node_id.h>
#include <common/onion_encode.h>
#include <common/wireaddr.h>
#include <lightningd/peer_control.h>

struct channel_id;
struct crypto_state;
struct json_stream;
struct lightningd;
struct peer_fd;
struct uncommitted_channel;

void NON_NULL_ARGS(2, 4) json_add_uncommitted_channel(struct command *cmd,
						      struct json_stream *response,
						      const struct uncommitted_channel *uc,
						      const struct peer *peer);

bool peer_start_openingd(struct peer *peer,
			 struct peer_fd *peer_fd);

struct subd *peer_get_owning_subd(struct peer *peer);

#endif /* LIGHTNING_LIGHTNINGD_OPENING_CONTROL_H */
