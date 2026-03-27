#ifndef LIGHTNING_WALLET_WALLETRPC_H
#define LIGHTNING_WALLET_WALLETRPC_H
#include "config.h"
#include <wallet/wallet.h>

struct command;
struct pubkey;
struct utxo;

/* We evaluate reserved timeouts lazily, so use this. */
bool is_reserved(const struct utxo *utxo, u32 current_height);

bool WARN_UNUSED_RESULT newaddr_inner(struct command *cmd, struct pubkey *pubkey,
				      enum addrtype addrtype);
#endif /* LIGHTNING_WALLET_WALLETRPC_H */
