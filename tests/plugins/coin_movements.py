#!/usr/bin/env python3

from pyln.client import Plugin

plugin = Plugin()


@plugin.init()
def init(configuration, options, plugin):
    plugin.coin_moves = []


@plugin.subscribe("coin_movement")
def notify_coin_movement(plugin, coin_movement, **kwargs):
    idx = coin_movement['movement_idx']
    plugin.log("{} coins movement version: {}".format(idx, coin_movement['version']))
    plugin.log("{} coins node: {}".format(idx, coin_movement['node_id']))
    plugin.log("{} coins mvt_type: {}".format(idx, coin_movement['type']))
    plugin.log("{} coins account: {}".format(idx, coin_movement['account_id']))
    plugin.log("{} coins credit: {}".format(idx, coin_movement['credit']))
    plugin.log("{} coins debit: {}".format(idx, coin_movement['debit']))
    plugin.log("{} coins tag: {}".format(idx, coin_movement['tag']))
    plugin.log("{} coins blockheight: {}".format(idx, coin_movement['blockheight']))
    plugin.log("{} coins timestamp: {}".format(idx, coin_movement['timestamp']))
    plugin.log("{} coins unit_of_account: {}".format(idx, coin_movement['unit_of_account']))

    for f in ['payment_hash', 'utxo_txid', 'vout', 'txid', 'part_id']:
        if f in coin_movement:
            plugin.log("{} coins {}: {}".format(idx, f, coin_movement[f]))

    plugin.coin_moves.append(coin_movement)


@plugin.method('listcoinmoves_plugin')
def return_moves(plugin):
    return {'coin_moves': plugin.coin_moves}


plugin.run()
