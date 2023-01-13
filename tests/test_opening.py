from fixtures import *  # noqa: F401,F403
from fixtures import TEST_NETWORK
from pyln.client import RpcError, Millisatoshi
from utils import (
    only_one, wait_for, sync_blockheight, first_channel_id, calc_lease_fee, check_coin_moves
)

from pathlib import Path
from pprint import pprint
import pytest
import re
import unittest


def find_next_feerate(node, peer):
    chan = only_one(node.rpc.listpeerchannels(peer.info['id'])['channels'])
    return chan['next_feerate']


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.openchannel('v2')
@pytest.mark.developer("requres 'dev-queryrates'")
def test_queryrates(node_factory, bitcoind):
    l1, l2 = node_factory.get_nodes(2, opts={'dev-no-reconnect': None})

    amount = 10 ** 6

    l1.fundwallet(amount * 10)
    l2.fundwallet(amount * 10)

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    with pytest.raises(RpcError, match=r'not advertising liquidity'):
        l1.rpc.dev_queryrates(l2.info['id'], amount, amount * 10)

    l2.rpc.call('funderupdate', {'policy': 'match',
                                 'policy_mod': 100,
                                 'per_channel_max_msat': '1btc',
                                 'fuzz_percent': 0,
                                 'lease_fee_base_msat': '2sat',
                                 'funding_weight': 1000,
                                 'lease_fee_basis': 140,
                                 'channel_fee_max_base_msat': '3sat',
                                 'channel_fee_max_proportional_thousandths': 101})

    wait_for(lambda: l1.rpc.listpeers()['peers'] == [])
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    result = l1.rpc.dev_queryrates(l2.info['id'], amount, amount)
    assert result['our_funding_msat'] == Millisatoshi(amount * 1000)
    assert result['their_funding_msat'] == Millisatoshi(amount * 1000)
    assert result['funding_weight'] == 1000
    assert result['lease_fee_base_msat'] == Millisatoshi(2000)
    assert result['lease_fee_basis'] == 140
    assert result['channel_fee_max_base_msat'] == Millisatoshi(3000)
    assert result['channel_fee_max_proportional_thousandths'] == 101


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.developer("uses dev-disconnect")
@pytest.mark.openchannel('v1')  # Mixed v1 + v2, v2 manually turned on
def test_multifunding_v2_best_effort(node_factory, bitcoind):
    '''
    Check that best_effort flag works.
    '''
    disconnects = ["-WIRE_INIT",
                   "-WIRE_ACCEPT_CHANNEL",
                   "-WIRE_FUNDING_SIGNED"]
    l1 = node_factory.get_node(options={'experimental-dual-fund': None},
                               allow_warning=True,
                               may_reconnect=True)
    l2 = node_factory.get_node(options={'experimental-dual-fund': None},
                               allow_warning=True,
                               may_reconnect=True)
    l3 = node_factory.get_node(disconnect=disconnects)
    l4 = node_factory.get_node()

    l1.fundwallet(2000000)

    destinations = [{"id": '{}@localhost:{}'.format(l2.info['id'], l2.port),
                     "amount": 50000},
                    {"id": '{}@localhost:{}'.format(l3.info['id'], l3.port),
                     "amount": 50000},
                    {"id": '{}@localhost:{}'.format(l4.info['id'], l4.port),
                     "amount": 50000}]

    for i, d in enumerate(disconnects):
        failed_sign = d == "-WIRE_FUNDING_SIGNED"
        # Should succeed due to best-effort flag.
        min_channels = 1 if failed_sign else 2
        l1.rpc.multifundchannel(destinations, minchannels=min_channels)

        bitcoind.generate_block(6, wait_for_mempool=1)

        # l3 should fail to have channels; l2 also fails on last attempt
        node_list = [l1, l4] if failed_sign else [l1, l2, l4]
        for node in node_list:
            node.daemon.wait_for_log(r'to CHANNELD_NORMAL')

        # There should be working channels to l2 and l4 for every run
        # but the last
        working_chans = [l4] if failed_sign else [l2, l4]
        for ldest in working_chans:
            inv = ldest.rpc.invoice(5000, 'i{}'.format(i), 'i{}'.format(i))['bolt11']
            l1.rpc.pay(inv)

        # Function to find the SCID of the channel that is
        # currently open.
        # Cannot use LightningNode.get_channel_scid since
        # it assumes the *first* channel found is the one
        # wanted, but in our case we close channels and
        # open again, so multiple channels may remain
        # listed.
        def get_funded_channel_scid(n1, n2):
            channels = n1.rpc.listpeerchannels(n2.info['id'])['channels']
            assert channels and len(channels) != 0
            for c in channels:
                state = c['state']
                if state in ('DUALOPEND_AWAITING_LOCKIN', 'CHANNELD_AWAITING_LOCKIN', 'CHANNELD_NORMAL'):
                    return c['short_channel_id']
            assert False

        # Now close channels to l2 and l4, for the next run.
        if not failed_sign:
            l1.rpc.close(get_funded_channel_scid(l1, l2))
        l1.rpc.close(get_funded_channel_scid(l1, l4))

        for node in node_list:
            node.daemon.wait_for_log(r'to CLOSINGD_COMPLETE')

    # With 2 down, it will fail to fund channel
    l2.stop()
    l3.stop()
    with pytest.raises(RpcError, match=r'(Connection refused|Bad file descriptor)'):
        l1.rpc.multifundchannel(destinations, minchannels=2)

    # This works though.
    l1.rpc.multifundchannel(destinations, minchannels=1)


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.developer("uses dev-disconnect")
@pytest.mark.openchannel('v2')
def test_v2_open_sigs_restart(node_factory, bitcoind):
    disconnects_1 = ['-WIRE_TX_SIGNATURES']
    disconnects_2 = ['+WIRE_TX_SIGNATURES']

    l1, l2 = node_factory.get_nodes(2,
                                    opts=[{'disconnect': disconnects_1,
                                           'may_reconnect': True},
                                          {'disconnect': disconnects_2,
                                           'may_reconnect': True}])

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    # Fund the channel, should appear to finish ok even though the
    # peer fails
    with pytest.raises(RpcError):
        l1.rpc.fundchannel(l2.info['id'], chan_amount)

    chan_id = first_channel_id(l1, l2)
    log = l1.daemon.is_in_log('{} psbt'.format(chan_id))
    assert log
    psbt = re.search("psbt (.*)", log).group(1)

    l1.daemon.wait_for_log('Peer has reconnected, state DUALOPEND_OPEN_INIT')
    try:
        # FIXME: why do we need to retry signed?
        l1.rpc.openchannel_signed(chan_id, psbt)
    except RpcError:
        pass

    l2.daemon.wait_for_log('Broadcasting funding tx')
    txid = l2.rpc.listpeerchannels(l1.info['id'])['channels'][0]['funding_txid']
    bitcoind.generate_block(6, wait_for_mempool=txid)

    # Make sure we're ok.
    l1.daemon.wait_for_log(r'to CHANNELD_NORMAL')
    l2.daemon.wait_for_log(r'to CHANNELD_NORMAL')


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.developer("uses dev-disconnect")
@pytest.mark.openchannel('v2')
def test_v2_open_sigs_restart_while_dead(node_factory, bitcoind):
    # Same thing as above, except the transaction mines
    # while we're asleep
    disconnects_1 = ['-WIRE_TX_SIGNATURES']
    disconnects_2 = ['+WIRE_TX_SIGNATURES']

    l1, l2 = node_factory.get_nodes(2,
                                    opts=[{'disconnect': disconnects_1,
                                           'may_reconnect': True,
                                           'may_fail': True},
                                          {'disconnect': disconnects_2,
                                           'may_reconnect': True,
                                           'may_fail': True}])

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    # Fund the channel, should appear to finish ok even though the
    # peer fails
    with pytest.raises(RpcError):
        l1.rpc.fundchannel(l2.info['id'], chan_amount)

    chan_id = first_channel_id(l1, l2)
    log = l1.daemon.is_in_log('{} psbt'.format(chan_id))
    assert log
    psbt = re.search("psbt (.*)", log).group(1)

    l1.daemon.wait_for_log('Peer has reconnected, state DUALOPEND_OPEN_INIT')
    try:
        # FIXME: why do we need to retry signed?
        l1.rpc.openchannel_signed(chan_id, psbt)
    except RpcError:
        pass

    l2.daemon.wait_for_log('Broadcasting funding tx')
    l2.daemon.wait_for_log('sendrawtx exit 0')

    l1.stop()
    l2.stop()
    bitcoind.generate_block(6)
    l1.restart()
    l2.restart()

    # Make sure we're ok.
    l2.daemon.wait_for_log(r'to CHANNELD_NORMAL')
    l1.daemon.wait_for_log(r'to CHANNELD_NORMAL')


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.openchannel('v2')
def test_v2_rbf_single(node_factory, bitcoind, chainparams):
    l1, l2 = node_factory.get_nodes(2, opts={'wumbo': None})

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount)
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')

    next_feerate = find_next_feerate(l1, l2)

    # Check that feerate info is correct
    info_1 = only_one(l1.rpc.listpeerchannels(l2.info['id'])['channels'])
    assert info_1['initial_feerate'] == info_1['last_feerate']
    rate = int(info_1['last_feerate'][:-5])
    assert int(info_1['next_feerate'][:-5]) == rate * 65 // 64

    # Initiate an RBF
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)

    # Do the bump
    bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])

    update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
    assert update['commitments_secured']

    # Check that feerate info has incremented
    info_2 = only_one(l1.rpc.listpeerchannels(l2.info['id'])['channels'])
    assert info_1['initial_feerate'] == info_2['initial_feerate']
    assert info_1['next_feerate'] == info_2['last_feerate']

    rate = int(info_2['last_feerate'][:-5])
    assert int(info_2['next_feerate'][:-5]) == rate * 65 // 64

    # Sign our inputs, and continue
    signed_psbt = l1.rpc.signpsbt(update['psbt'])['signed_psbt']

    # Fails because we didn't put enough feerate in.
    with pytest.raises(RpcError, match=r'insufficient fee'):
        l1.rpc.openchannel_signed(chan_id, signed_psbt)

    # Do it again, with a higher feerate
    info_2 = only_one(l1.rpc.listpeerchannels(l2.info['id'])['channels'])
    assert info_1['initial_feerate'] == info_2['initial_feerate']
    assert info_1['next_feerate'] == info_2['last_feerate']
    rate = int(info_2['last_feerate'][:-5])
    assert int(info_2['next_feerate'][:-5]) == rate * 65 // 64

    # We 4x the feerate to beat the min-relay fee
    next_rate = '{}perkw'.format(rate * 65 // 64 * 4)
    # Gotta unreserve the psbt and re-reserve with higher feerate
    l1.rpc.unreserveinputs(initpsbt['psbt'])
    initpsbt = l1.rpc.utxopsbt(chan_amount, next_rate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)
    # Do the bump+sign
    bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'],
                                   funding_feerate=next_rate)
    update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
    assert update['commitments_secured']
    signed_psbt = l1.rpc.signpsbt(update['psbt'])['signed_psbt']
    l1.rpc.openchannel_signed(chan_id, signed_psbt)

    bitcoind.generate_block(1)
    sync_blockheight(bitcoind, [l1])
    l1.daemon.wait_for_log(' to CHANNELD_NORMAL')

    # Check that feerate info is gone
    info_1 = only_one(l1.rpc.listpeerchannels(l2.info['id'])['channels'])
    assert 'initial_feerate' not in info_1
    assert 'last_feerate' not in info_1
    assert 'next_feerate' not in info_1

    # Shut l2 down, force close the channel.
    l2.stop()
    resp = l1.rpc.close(l2.info['id'], unilateraltimeout=1)
    assert resp['type'] == 'unilateral'
    l1.daemon.wait_for_log(' to CHANNELD_SHUTTING_DOWN')
    l1.daemon.wait_for_log('sendrawtx exit 0')


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.openchannel('v2')
def test_v2_rbf_liquidity_ad(node_factory, bitcoind, chainparams):

    opts = {'funder-policy': 'match', 'funder-policy-mod': 100,
            'lease-fee-base-sat': '100sat', 'lease-fee-basis': 100,
            'may_reconnect': True}
    l1, l2 = node_factory.get_nodes(2, opts=opts)

    # what happens when we RBF?
    feerate = 2000
    amount = 500000
    l1.fundwallet(20000000)
    l2.fundwallet(20000000)

    # l1 leases a channel from l2
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    rates = l1.rpc.dev_queryrates(l2.info['id'], amount, amount)
    wait_for(lambda: l1.rpc.listpeers()['peers'] == [])
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    chan_id = l1.rpc.fundchannel(l2.info['id'], amount, request_amt=amount,
                                 feerate='{}perkw'.format(feerate),
                                 compact_lease=rates['compact_lease'])['channel_id']

    vins = [x for x in l1.rpc.listfunds()['outputs'] if x['reserved']]
    assert only_one(vins)
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['output'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')

    est_fees = calc_lease_fee(amount, feerate, rates)

    # This should be the accepter's amount
    fundings = only_one(l1.rpc.listpeerchannels()['channels'])['funding']
    assert Millisatoshi(amount * 1000) == fundings['remote_funds_msat']
    assert Millisatoshi(est_fees + amount * 1000) == fundings['local_funds_msat']
    assert Millisatoshi(est_fees) == fundings['fee_paid_msat']
    assert 'fee_rcvd_msat' not in fundings

    # rbf the lease with a higher amount
    rate = int(find_next_feerate(l1, l2)[:-5])
    # We 4x the feerate to beat the min-relay fee
    next_feerate = '{}perkw'.format(rate * 4)

    # Initiate an RBF
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(amount, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)['psbt']

    # do the bump
    bump = l1.rpc.openchannel_bump(chan_id, amount, initpsbt,
                                   funding_feerate=next_feerate)
    update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
    assert update['commitments_secured']

    # Sign our inputs, and continue
    signed_psbt = l1.rpc.signpsbt(update['psbt'])['signed_psbt']
    l1.rpc.openchannel_signed(chan_id, signed_psbt)

    # There's data in the datastore now (l2 only)
    assert l1.rpc.listdatastore() == {'datastore': []}
    only_one(l2.rpc.listdatastore("funder/{}".format(chan_id))['datastore'])

    # what happens when the channel opens?
    bitcoind.generate_block(6)
    l1.daemon.wait_for_log('to CHANNELD_NORMAL')

    # Datastore should be cleaned up!
    assert l1.rpc.listdatastore() == {'datastore': []}
    assert l2.rpc.listdatastore() == {'datastore': []}

    # This should be the accepter's amount
    fundings = only_one(l1.rpc.listpeerchannels()['channels'])['funding']
    # The is still there!
    assert Millisatoshi(amount * 1000) == Millisatoshi(fundings['remote_funds_msat'])

    wait_for(lambda: [c['active'] for c in l1.rpc.listchannels(l1.get_channel_scid(l2))['channels']] == [True, True])

    # send some payments, mine a block or two
    inv = l2.rpc.invoice(10**4, '1', 'no_1')
    l1.rpc.pay(inv['bolt11'])

    # l2 attempts to close a channel that it leased, should succeed
    # (channel isnt leased)
    l2.rpc.close(l1.get_channel_scid(l2))
    l1.daemon.wait_for_log('State changed from CLOSINGD_SIGEXCHANGE to CLOSINGD_COMPLETE')


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.developer("uses dev-no-reconnect")
@pytest.mark.openchannel('v2')
def test_v2_rbf_multi(node_factory, bitcoind, chainparams):
    l1, l2 = node_factory.get_nodes(2,
                                    opts={'may_reconnect': True,
                                          'dev-no-reconnect': None,
                                          'allow_warning': True})

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount)
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')

    # Attempt to do abort, should fail since we've
    # already gotten an inflight
    with pytest.raises(RpcError):
        l1.rpc.openchannel_abort(chan_id)

    rate = int(find_next_feerate(l1, l2)[:-5])
    # We 4x the feerate to beat the min-relay fee
    next_feerate = '{}perkw'.format(rate * 4)

    # Initiate an RBF
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)

    # Do the bump
    bump = l1.rpc.openchannel_bump(chan_id, chan_amount,
                                   initpsbt['psbt'],
                                   funding_feerate=next_feerate)

    # Abort this open attempt! We will re-try
    aborted = l1.rpc.openchannel_abort(chan_id)
    assert not aborted['channel_canceled']
    wait_for(lambda: only_one(l1.rpc.listpeers()['peers'])['connected'] is False)

    # Do the bump, again, same feerate
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    bump = l1.rpc.openchannel_bump(chan_id, chan_amount,
                                   initpsbt['psbt'],
                                   funding_feerate=next_feerate)

    update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
    assert update['commitments_secured']

    # Sign our inputs, and continue
    signed_psbt = l1.rpc.signpsbt(update['psbt'])['signed_psbt']
    l1.rpc.openchannel_signed(chan_id, signed_psbt)

    # We 2x the feerate to beat the min-relay fee
    rate = int(find_next_feerate(l1, l2)[:-5])
    next_feerate = '{}perkw'.format(rate * 2)

    # Initiate another RBF, double the channel amount this time
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(chan_amount * 2, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)

    # Do the bump
    bump = l1.rpc.openchannel_bump(chan_id, chan_amount * 2,
                                   initpsbt['psbt'],
                                   funding_feerate=next_feerate)

    update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
    assert update['commitments_secured']

    # Sign our inputs, and continue
    signed_psbt = l1.rpc.signpsbt(update['psbt'])['signed_psbt']
    l1.rpc.openchannel_signed(chan_id, signed_psbt)

    bitcoind.generate_block(1)
    sync_blockheight(bitcoind, [l1])
    l1.daemon.wait_for_log(' to CHANNELD_NORMAL')


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.developer("uses dev-disconnect")
@pytest.mark.openchannel('v2')
def test_rbf_reconnect_init(node_factory, bitcoind, chainparams):
    disconnects = ['-WIRE_INIT_RBF',
                   '+WIRE_INIT_RBF']

    l1, l2 = node_factory.get_nodes(2,
                                    opts=[{'disconnect': disconnects,
                                           'may_reconnect': True},
                                          {'may_reconnect': True}])

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount)
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')

    next_feerate = find_next_feerate(l1, l2)

    # Initiate an RBF
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)

    # Do the bump!?
    for d in disconnects:
        l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
        with pytest.raises(RpcError):
            l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])
        assert l1.rpc.getpeer(l2.info['id']) is not None

    # This should succeed
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.developer("uses dev-disconnect")
@pytest.mark.openchannel('v2')
def test_rbf_reconnect_ack(node_factory, bitcoind, chainparams):
    disconnects = ['-WIRE_ACK_RBF',
                   '+WIRE_ACK_RBF']

    l1, l2 = node_factory.get_nodes(2,
                                    opts=[{'may_reconnect': True},
                                          {'disconnect': disconnects,
                                           'may_reconnect': True}])

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount)
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')

    next_feerate = find_next_feerate(l1, l2)

    # Initiate an RBF
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)

    # Do the bump!?
    for d in disconnects:
        l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
        with pytest.raises(RpcError):
            l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])
        assert l1.rpc.getpeer(l2.info['id']) is not None

    # This should succeed
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.developer("uses dev-disconnect")
@pytest.mark.openchannel('v2')
def test_rbf_reconnect_tx_construct(node_factory, bitcoind, chainparams):
    disconnects = ['=WIRE_TX_ADD_INPUT',  # Initial funding succeeds
                   '-WIRE_TX_ADD_INPUT',
                   '+WIRE_TX_ADD_INPUT',
                   '-WIRE_TX_ADD_OUTPUT',
                   '+WIRE_TX_ADD_OUTPUT',
                   '-WIRE_TX_COMPLETE',
                   '+WIRE_TX_COMPLETE']

    l1, l2 = node_factory.get_nodes(2,
                                    opts=[{'disconnect': disconnects,
                                           'may_reconnect': True,
                                           'dev-no-reconnect': None},
                                          {'may_reconnect': True,
                                           'dev-no-reconnect': None}])

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount)
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')

    next_feerate = find_next_feerate(l1, l2)

    # Initiate an RBF
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)

    # Run through TX_ADD wires
    for d in disconnects[1:-2]:
        l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
        with pytest.raises(RpcError):
            l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])
        wait_for(lambda: l1.rpc.getpeer(l2.info['id'])['connected'] is False)

    # Now we finish off the completes failure check
    for d in disconnects[-2:]:
        l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
        bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])
        with pytest.raises(RpcError):
            update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
        wait_for(lambda: l1.rpc.getpeer(l2.info['id'])['connected'] is False)

    # Now we succeed
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])
    update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
    assert update['commitments_secured']


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.developer("uses dev-disconnect")
@pytest.mark.openchannel('v2')
def test_rbf_reconnect_tx_sigs(node_factory, bitcoind, chainparams):
    disconnects = ['=WIRE_TX_SIGNATURES',  # Initial funding succeeds
                   '-WIRE_TX_SIGNATURES',  # When we send tx-sigs, RBF
                   '=WIRE_TX_SIGNATURES',  # When we reconnect
                   '+WIRE_TX_SIGNATURES']  # When we RBF again

    l1, l2 = node_factory.get_nodes(2,
                                    opts=[{'disconnect': disconnects,
                                           'may_reconnect': True},
                                          {'may_reconnect': True}])

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount)
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log('Broadcasting funding tx')
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')

    rate = int(find_next_feerate(l1, l2)[:-5])
    # We 4x the feerate to beat the min-relay fee
    next_feerate = '{}perkw'.format(rate * 4)

    # Initiate an RBF
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)

    bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'],
                                   funding_feerate=next_feerate)
    update = l1.rpc.openchannel_update(chan_id, bump['psbt'])

    # Sign our inputs, and continue
    signed_psbt = l1.rpc.signpsbt(update['psbt'])['signed_psbt']

    # First time we error when we send our sigs
    with pytest.raises(RpcError, match='Owning subdaemon dualopend died'):
        l1.rpc.openchannel_signed(chan_id, signed_psbt)

    # We reconnect and try again. feerate should have bumped
    rate = int(find_next_feerate(l1, l2)[:-5])
    # We 4x the feerate to beat the min-relay fee
    next_feerate = '{}perkw'.format(rate * 4)

    # Initiate an RBF
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                               prev_utxos, reservedok=True,
                               min_witness_weight=110,
                               excess_as_change=True)

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)

    # l2 gets our sigs and broadcasts them
    l2.daemon.wait_for_log('peer_in WIRE_CHANNEL_REESTABLISH')
    l2.daemon.wait_for_log('peer_in WIRE_TX_SIGNATURES')
    l2.daemon.wait_for_log('sendrawtx exit 0')

    # Wait until we've done re-establish, if we try to
    # RBF again too quickly, it'll fail since they haven't
    # had time to process our sigs yet
    l1.daemon.wait_for_log('peer_in WIRE_CHANNEL_REESTABLISH')
    l1.daemon.wait_for_log('peer_in WIRE_TX_SIGNATURES')

    # 2nd RBF
    bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'],
                                   funding_feerate=next_feerate)
    update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
    signed_psbt = l1.rpc.signpsbt(update['psbt'])['signed_psbt']

    # Second time we error after we send our sigs
    with pytest.raises(RpcError, match='Owning subdaemon dualopend died'):
        l1.rpc.openchannel_signed(chan_id, signed_psbt)

    # l2 gets our sigs
    l2.daemon.wait_for_log('peer_in WIRE_TX_SIGNATURES')
    l2.daemon.wait_for_log('sendrawtx exit 0')

    # mine a block?
    bitcoind.generate_block(1)
    sync_blockheight(bitcoind, [l1])
    l1.daemon.wait_for_log(' to CHANNELD_NORMAL')

    # Check that they have matching funding txid
    l1_funding_txid = only_one(l1.rpc.listpeerchannels()['channels'])['funding_txid']
    l2_funding_txid = only_one(l2.rpc.listpeerchannels()['channels'])['funding_txid']
    assert l1_funding_txid == l2_funding_txid


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.openchannel('v2')
def test_rbf_no_overlap(node_factory, bitcoind, chainparams):
    l1, l2 = node_factory.get_nodes(2,
                                    opts={'allow_warning': True})

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount)
    chan_id = res['channel_id']

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')

    next_feerate = find_next_feerate(l1, l2)

    # Initiate an RBF (this grabs the non-reserved utxo, which isnt the
    # one we started with)
    startweight = 42 + 172  # base weight, funding output
    initpsbt = l1.rpc.fundpsbt(chan_amount, next_feerate, startweight,
                               min_witness_weight=110,
                               excess_as_change=True)

    # Do the bump
    bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])

    with pytest.raises(RpcError, match='No overlapping input present.'):
        l1.rpc.openchannel_update(chan_id, bump['psbt'])


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.openchannel('v2')
@pytest.mark.developer("uses dev-sign-last-tx")
def test_rbf_fails_to_broadcast(node_factory, bitcoind, chainparams):
    l1, l2 = node_factory.get_nodes(2,
                                    opts={'allow_warning': True,
                                          'may_reconnect': True})

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    # Really low feerate means that the bump wont work the first time
    res = l1.rpc.fundchannel(l2.info['id'], chan_amount, feerate='253perkw')
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')
    inflights = only_one(l1.rpc.listpeerchannels()['channels'])['inflight']
    assert inflights[-1]['funding_txid'] in bitcoind.rpc.getrawmempool()

    def run_retry():
        startweight = 42 + 173
        rate = int(find_next_feerate(l1, l2)[:-5])
        # We 2x the feerate to beat the min-relay fee
        next_feerate = '{}perkw'.format(rate * 2)
        initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                                   prev_utxos, reservedok=True,
                                   min_witness_weight=110,
                                   excess_as_change=True)

        l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
        bump = l1.rpc.openchannel_bump(chan_id, chan_amount,
                                       initpsbt['psbt'],
                                       funding_feerate=next_feerate)
        # We should be able to call this with while an open is progress
        # but not yet committed
        l1.rpc.dev_sign_last_tx(l2.info['id'])
        update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
        assert update['commitments_secured']

        return l1.rpc.signpsbt(update['psbt'])['signed_psbt']

    signed_psbt = run_retry()
    l1.rpc.openchannel_signed(chan_id, signed_psbt)
    inflights = only_one(l1.rpc.listpeerchannels()['channels'])['inflight']
    assert inflights[-1]['funding_txid'] in bitcoind.rpc.getrawmempool()

    # Restart and listpeers, used to crash
    l1.restart()
    l1.rpc.listpeers()

    # We've restarted. Let's RBF
    signed_psbt = run_retry()
    l1.rpc.openchannel_signed(chan_id, signed_psbt)
    inflights = only_one(l1.rpc.listpeerchannels()['channels'])['inflight']
    assert len(inflights) == 3
    assert inflights[-1]['funding_txid'] in bitcoind.rpc.getrawmempool()

    l1.restart()

    # Are inflights the same post restart
    prev_inflights = inflights
    inflights = only_one(l1.rpc.listpeerchannels()['channels'])['inflight']
    assert prev_inflights == inflights
    assert inflights[-1]['funding_txid'] in bitcoind.rpc.getrawmempool()

    # Produce a signature for every inflight
    last_txs = l1.rpc.dev_sign_last_tx(l2.info['id'])
    assert len(last_txs['inflights']) == len(inflights)
    for last_tx, inflight in zip(last_txs['inflights'], inflights):
        assert last_tx['funding_txid'] == inflight['funding_txid']
    assert last_txs['tx']


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.openchannel('v2')
def test_rbf_broadcast_close_inflights(node_factory, bitcoind, chainparams):
    """
    Close a channel before it's mined, and the most recent transaction
    hasn't made it to the mempool. Should publish all the commitment
    transactions that we have.
    """
    l1, l2 = node_factory.get_nodes(2,
                                    opts={'allow_warning': True})

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount, feerate='7500perkw')
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')
    inflights = only_one(l1.rpc.listpeerchannels()['channels'])['inflight']
    assert inflights[-1]['funding_txid'] in bitcoind.rpc.getrawmempool()

    # Make it such that l1 and l2 cannot broadcast transactions
    # (mimics failing to reach the miner with replacement)
    def censoring_sendrawtx(r):
        return {'id': r['id'], 'result': {}}

    l1.daemon.rpcproxy.mock_rpc('sendrawtransaction', censoring_sendrawtx)
    l2.daemon.rpcproxy.mock_rpc('sendrawtransaction', censoring_sendrawtx)

    def run_retry():
        startweight = 42 + 173
        next_feerate = find_next_feerate(l1, l2)
        initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                                   prev_utxos, reservedok=True,
                                   min_witness_weight=110,
                                   excess_as_change=True)

        l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
        bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])
        update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
        assert update['commitments_secured']

        return l1.rpc.signpsbt(update['psbt'])['signed_psbt']

    signed_psbt = run_retry()
    l1.rpc.openchannel_signed(chan_id, signed_psbt)
    inflights = only_one(l1.rpc.listpeerchannels()['channels'])['inflight']
    assert inflights[-1]['funding_txid'] not in bitcoind.rpc.getrawmempool()

    cmtmt_txid = only_one(l1.rpc.listpeerchannels()['channels'])['scratch_txid']
    assert cmtmt_txid == inflights[-1]['scratch_txid']

    # l2 goes offline
    l2.stop()

    # l1 drops to chain.
    l1.daemon.rpcproxy.mock_rpc('sendrawtransaction', None)
    l1.rpc.close(chan_id, 1)
    l1.daemon.wait_for_logs(['Broadcasting txid {}'.format(inflights[0]['scratch_txid']),
                             'Broadcasting txid {}'.format(inflights[1]['scratch_txid']),
                             'sendrawtx exit 0',
                             'sendrawtx exit 25'])
    assert inflights[0]['scratch_txid'] in bitcoind.rpc.getrawmempool()
    assert inflights[1]['scratch_txid'] not in bitcoind.rpc.getrawmempool()


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.openchannel('v2')
def test_rbf_non_last_mined(node_factory, bitcoind, chainparams):
    """
    What happens if a 'non-tip' RBF transaction is mined?
    """
    l1, l2 = node_factory.get_nodes(2,
                                    opts={'allow_warning': True,
                                          'may_reconnect': True})

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    amount = 2**24
    chan_amount = 100000
    bitcoind.rpc.sendtoaddress(l1.rpc.newaddr()['bech32'], amount / 10**8 + 0.01)
    bitcoind.generate_block(1)
    # Wait for it to arrive.
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) > 0)

    res = l1.rpc.fundchannel(l2.info['id'], chan_amount, feerate='7500perkw')
    chan_id = res['channel_id']
    vins = bitcoind.rpc.decoderawtransaction(res['tx'])['vin']
    assert(only_one(vins))
    prev_utxos = ["{}:{}".format(vins[0]['txid'], vins[0]['vout'])]

    # Check that we're waiting for lockin
    l1.daemon.wait_for_log(' to DUALOPEND_AWAITING_LOCKIN')
    inflights = only_one(l1.rpc.listpeerchannels()['channels'])['inflight']
    assert inflights[-1]['funding_txid'] in bitcoind.rpc.getrawmempool()

    def run_retry():
        startweight = 42 + 173
        rate = int(find_next_feerate(l1, l2)[:-5])
        # We 2x the feerate to beat the min-relay fee
        next_feerate = '{}perkw'.format(rate * 2)
        initpsbt = l1.rpc.utxopsbt(chan_amount, next_feerate, startweight,
                                   prev_utxos, reservedok=True,
                                   min_witness_weight=110,
                                   excess_as_change=True)

        l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
        bump = l1.rpc.openchannel_bump(chan_id, chan_amount, initpsbt['psbt'])
        update = l1.rpc.openchannel_update(chan_id, bump['psbt'])
        assert update['commitments_secured']

        return l1.rpc.signpsbt(update['psbt'])['signed_psbt']

    # Make a second inflight
    signed_psbt = run_retry()
    l1.rpc.openchannel_signed(chan_id, signed_psbt)

    # Make it such that l1 and l2 cannot broadcast transactions
    # (mimics failing to reach the miner with replacement)
    def censoring_sendrawtx(r):
        return {'id': r['id'], 'result': {}}

    l1.daemon.rpcproxy.mock_rpc('sendrawtransaction', censoring_sendrawtx)
    l2.daemon.rpcproxy.mock_rpc('sendrawtransaction', censoring_sendrawtx)

    # Make a 3rd inflight that won't make it into the mempool
    signed_psbt = run_retry()
    l1.rpc.openchannel_signed(chan_id, signed_psbt)

    l1.daemon.rpcproxy.mock_rpc('sendrawtransaction', None)
    l2.daemon.rpcproxy.mock_rpc('sendrawtransaction', None)

    # We fetch out our inflights list
    inflights = only_one(l1.rpc.listpeerchannels()['channels'])['inflight']

    # l2 goes offline
    l2.stop()

    # The funding transaction gets mined (should be the 2nd inflight)
    bitcoind.generate_block(6, wait_for_mempool=1)

    # l2 comes back up
    l2.start()

    # everybody's got the right things now
    l1.daemon.wait_for_log(r'to CHANNELD_NORMAL')
    l2.daemon.wait_for_log(r'to CHANNELD_NORMAL')

    channel = only_one(l1.rpc.listpeerchannels()['channels'])
    assert channel['funding_txid'] == inflights[1]['funding_txid']
    assert channel['scratch_txid'] == inflights[1]['scratch_txid']

    # We delete inflights when the channel is in normal ops
    assert 'inflights' not in channel

    # l2 stops, again
    l2.stop()

    # l1 drops to chain.
    l1.rpc.close(chan_id, 1)
    l1.daemon.wait_for_log('Broadcasting txid {}'.format(channel['scratch_txid']))

    # The funding transaction gets mined (should be the 2nd inflight)
    bitcoind.generate_block(1, wait_for_mempool=1)
    l1.daemon.wait_for_log(r'to ONCHAIN')


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
@pytest.mark.openchannel('v2')
def test_funder_options(node_factory, bitcoind):
    l1, l2, l3 = node_factory.get_nodes(3)
    l1.fundwallet(10**7)

    # Check the default options
    funder_opts = l1.rpc.call('funderupdate')

    assert funder_opts['policy'] == 'fixed'
    assert funder_opts['policy_mod'] == 0
    assert funder_opts['min_their_funding_msat'] == Millisatoshi('10000000msat')
    assert funder_opts['max_their_funding_msat'] == Millisatoshi('4294967295000msat')
    assert funder_opts['per_channel_min_msat'] == Millisatoshi('10000000msat')
    assert funder_opts['per_channel_max_msat'] == Millisatoshi('4294967295000msat')
    assert funder_opts['reserve_tank_msat'] == Millisatoshi('0msat')
    assert funder_opts['fuzz_percent'] == 0
    assert funder_opts['fund_probability'] == 100
    assert funder_opts['leases_only']

    # l2 funds a chanenl with us. We don't contribute
    l2.rpc.connect(l1.info['id'], 'localhost', l1.port)
    l2.fundchannel(l1, 10**6)
    chan_info = only_one(l2.rpc.listpeerchannels(l1.info['id'])['channels'])
    # l1 contributed nothing
    assert chan_info['funding']['remote_funds_msat'] == Millisatoshi('0msat')
    assert chan_info['funding']['local_funds_msat'] != Millisatoshi('0msat')

    # Change all the options
    funder_opts = l1.rpc.call('funderupdate',
                              {'policy': 'available',
                               'policy_mod': 100,
                               'min_their_funding_msat': '100000msat',
                               'max_their_funding_msat': '2000000000msat',
                               'per_channel_min_msat': '8000000msat',
                               'per_channel_max_msat': '10000000000msat',
                               'reserve_tank_msat': '3000000msat',
                               'fund_probability': 99,
                               'fuzz_percent': 0,
                               'leases_only': False})

    assert funder_opts['policy'] == 'available'
    assert funder_opts['policy_mod'] == 100
    assert funder_opts['min_their_funding_msat'] == Millisatoshi('100000msat')
    assert funder_opts['max_their_funding_msat'] == Millisatoshi('2000000000msat')
    assert funder_opts['per_channel_min_msat'] == Millisatoshi('8000000msat')
    assert funder_opts['per_channel_max_msat'] == Millisatoshi('10000000000msat')
    assert funder_opts['reserve_tank_msat'] == Millisatoshi('3000000msat')
    assert funder_opts['fuzz_percent'] == 0
    assert funder_opts['fund_probability'] == 99

    # Set the fund probability back up to 100.
    funder_opts = l1.rpc.call('funderupdate',
                              {'fund_probability': 100})
    l3.rpc.connect(l1.info['id'], 'localhost', l1.port)
    l3.fundchannel(l1, 10**6)
    chan_info = only_one(l3.rpc.listpeerchannels(l1.info['id'])['channels'])
    # l1 contributed all its funds!
    assert chan_info['funding']['remote_funds_msat'] == Millisatoshi('9994945000msat')
    assert chan_info['funding']['local_funds_msat'] == Millisatoshi('1000000000msat')


@unittest.skipIf(TEST_NETWORK != 'regtest', 'elementsd doesnt yet support PSBT features we need')
def test_funder_contribution_limits(node_factory, bitcoind):
    opts = {'experimental-dual-fund': None,
            'feerates': (5000, 5000, 5000, 5000)}
    l1, l2, l3 = node_factory.get_nodes(3, opts=opts)

    # We do a lot of these, so do them all then mine all at once.
    addr, txid = l1.fundwallet(10**8, mine_block=False)
    l1msgs = ['Owning output .* txid {} CONFIRMED'.format(txid)]

    # Give l2 lots of utxos
    l2msgs = []
    for amt in (10**3,  # this one is too small to add
                10**5, 10**4, 10**4, 10**4, 10**4, 10**4):
        addr, txid = l2.fundwallet(amt, mine_block=False)
        l2msgs.append('Owning output .* txid {} CONFIRMED'.format(txid))

    # Give l3 lots of utxos
    l3msgs = []
    for amt in (10**3,  # this one is too small to add
                10**4, 10**4, 10**4, 10**4, 10**4, 10**4, 10**4, 10**4, 10**4, 10**4, 10**4):
        addr, txid = l3.fundwallet(amt, mine_block=False)
        l3msgs.append('Owning output .* txid {} CONFIRMED'.format(txid))

    bitcoind.generate_block(1)
    l1.daemon.wait_for_logs(l1msgs)
    l2.daemon.wait_for_logs(l2msgs)
    l3.daemon.wait_for_logs(l3msgs)

    # Contribute 100% of available funds to l2, all 6 utxos (smallest utxo
    # 10**3 is left out)
    l2.rpc.call('funderupdate',
                {'policy': 'available',
                 'policy_mod': 100,
                 'min_their_funding_msat': '1000msat',
                 'per_channel_min_msat': '1000000msat',
                 'fund_probability': 100,
                 'fuzz_percent': 0,
                 'leases_only': False})

    # Set our contribution to 50k sat, should only use 6 of 12 available utxos
    l3.rpc.call('funderupdate',
                {'policy': 'fixed',
                 'policy_mod': '50000sat',
                 'min_their_funding_msat': '1000msat',
                 'per_channel_min_msat': '1000sat',
                 'per_channel_max_msat': '500000sat',
                 'fund_probability': 100,
                 'fuzz_percent': 0,
                 'leases_only': False})

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    l1.fundchannel(l2, 10**7)
    assert l2.daemon.is_in_log('Policy .* returned funding amount of 141780sat')
    assert l2.daemon.is_in_log(r'calling `signpsbt` .* 6 inputs')

    l1.rpc.connect(l3.info['id'], 'localhost', l3.port)
    l1.fundchannel(l3, 10**7)
    assert l3.daemon.is_in_log('Policy .* returned funding amount of 50000sat')
    assert l3.daemon.is_in_log(r'calling `signpsbt` .* 6 inputs')


@pytest.mark.openchannel('v2')
@pytest.mark.developer("requres 'dev-disconnect'")
def test_inflight_dbload(node_factory, bitcoind):
    """Bad db field access breaks Postgresql on startup with opening leases"""
    disconnects = ["@WIRE_COMMITMENT_SIGNED"]
    l1, l2 = node_factory.get_nodes(2, opts=[{'experimental-dual-fund': None,
                                              'dev-no-reconnect': None,
                                              'may_reconnect': True,
                                              'disconnect': disconnects},
                                             {'experimental-dual-fund': None,
                                              'dev-no-reconnect': None,
                                              'may_reconnect': True,
                                              'funder-policy': 'match',
                                              'funder-policy-mod': 100,
                                              'lease-fee-base-sat': '100sat',
                                              'lease-fee-basis': 100}])

    feerate = 2000
    amount = 500000
    l1.fundwallet(20000000)
    l2.fundwallet(20000000)

    # l1 leases a channel from l2
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    rates = l1.rpc.dev_queryrates(l2.info['id'], amount, amount)
    wait_for(lambda: len(l1.rpc.listpeers(l2.info['id'])['peers']) == 0)
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    l1.rpc.fundchannel(l2.info['id'], amount, request_amt=amount,
                       feerate='{}perkw'.format(feerate),
                       compact_lease=rates['compact_lease'])
    l1.daemon.wait_for_log(r'dev_disconnect: @WIRE_COMMITMENT_SIGNED')

    l1.restart()


def test_zeroconf_mindepth(bitcoind, node_factory):
    """Check that funder/fundee can customize mindepth.

    Zeroconf will use this to set the mindepth to 0, which coupled
    with an artificial depth=0 event that will result in an immediate
    `channel_ready` being sent.

    """
    plugin_path = Path(__file__).parent / "plugins" / "zeroconf-selective.py"

    l1, l2 = node_factory.get_nodes(2, opts=[
        {},
        {
            'plugin': str(plugin_path),
            'zeroconf-allow': '0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518',
            'zeroconf-mindepth': '2',
        },
    ])

    # Try to open a mindepth=6 channel
    l1.fundwallet(10**6)

    l1.connect(l2)
    assert (int(l1.rpc.listpeers()['peers'][0]['features'], 16) >> 50) & 0x02 != 0

    # Now start the negotiation, l1 should have negotiated zeroconf,
    # and use their own mindepth=6, while l2 uses mindepth=2 from the
    # plugin
    l1.rpc.fundchannel(l2.info['id'], 'all', mindepth=6)

    assert l1.db.query('SELECT minimum_depth FROM channels') == [{'minimum_depth': 6}]
    assert l2.db.query('SELECT minimum_depth FROM channels') == [{'minimum_depth': 2}]

    bitcoind.generate_block(2, wait_for_mempool=1)  # Confirm on the l2 side.
    l2.daemon.wait_for_log(r'peer_out WIRE_CHANNEL_READY')
    # l1 should not be sending channel_ready yet, it is
    # configured to wait for 6 confirmations.
    assert not l1.daemon.is_in_log(r'peer_out WIRE_CHANNEL_READY')

    bitcoind.generate_block(4)  # Confirm on the l2 side.
    l1.daemon.wait_for_log(r'peer_out WIRE_CHANNEL_READY')

    wait_for(lambda: only_one(l1.rpc.listpeerchannels()['channels'])['state'] == "CHANNELD_NORMAL")
    wait_for(lambda: only_one(l2.rpc.listpeerchannels()['channels'])['state'] == "CHANNELD_NORMAL")


def test_zeroconf_open(bitcoind, node_factory):
    """Let's open a zeroconf channel

    Just test that both parties opting in results in a channel that is
    immediately usable.

    """
    plugin_path = Path(__file__).parent / "plugins" / "zeroconf-selective.py"

    l1, l2 = node_factory.get_nodes(2, opts=[
        {},
        {
            'plugin': str(plugin_path),
            'zeroconf-allow': '0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518'
        },
    ])

    # Try to open a mindepth=0 channel
    l1.fundwallet(10**6)

    l1.connect(l2)
    assert (int(l1.rpc.listpeers()['peers'][0]['features'], 16) >> 50) & 0x02 != 0

    # Now start the negotiation, l1 should have negotiated zeroconf,
    # and use their own mindepth=6, while l2 uses mindepth=2 from the
    # plugin
    l1.rpc.fundchannel(l2.info['id'], 'all', mindepth=0)

    assert l1.db.query('SELECT minimum_depth FROM channels') == [{'minimum_depth': 0}]
    assert l2.db.query('SELECT minimum_depth FROM channels') == [{'minimum_depth': 0}]

    l1.daemon.wait_for_logs([
        r'peer_in WIRE_CHANNEL_READY',
        r'Peer told us that they\'ll use alias=[0-9x]+ for this channel',
    ])
    l2.daemon.wait_for_logs([
        r'peer_in WIRE_CHANNEL_READY',
        r'Peer told us that they\'ll use alias=[0-9x]+ for this channel',
    ])

    wait_for(lambda: only_one(l1.rpc.listpeerchannels()['channels'])['state'] == 'CHANNELD_NORMAL')
    wait_for(lambda: only_one(l2.rpc.listpeerchannels()['channels'])['state'] == 'CHANNELD_NORMAL')
    wait_for(lambda: l2.rpc.listincoming()['incoming'] != [])

    inv = l2.rpc.invoice(10**8, 'lbl', 'desc')['bolt11']
    details = l1.rpc.decodepay(inv)
    pprint(details)
    assert('routes' in details and len(details['routes']) == 1)
    hop = details['routes'][0][0]  # First (and only) hop of hint 0
    l1alias = only_one(l1.rpc.listpeerchannels()['channels'])['alias']['local']
    assert(hop['pubkey'] == l1.info['id'])  # l1 is the entrypoint
    assert(hop['short_channel_id'] == l1alias)  # Alias has to make sense to entrypoint
    l1.rpc.pay(inv)

    # Ensure lightningd knows about the balance change before
    # attempting the other way around.
    l2.daemon.wait_for_log(r'Balance [0-9]+msat -> [0-9]+msat')

    # Inverse payments should work too
    pprint(l2.rpc.listpeers())
    inv = l1.rpc.invoice(10**5, 'lbl', 'desc')['bolt11']
    l2.rpc.pay(inv)


def test_zeroconf_public(bitcoind, node_factory, chainparams):
    """Test that we transition correctly from zeroconf to public

    The differences being that a public channel MUST use the public
    scid. l1 and l2 open a zeroconf channel, then l3 learns about it
    after 6 confirmations.

    """
    plugin_path = Path(__file__).parent / "plugins" / "zeroconf-selective.py"
    coin_mvt_plugin = Path(__file__).parent / "plugins" / "coin_movements.py"

    l1, l2, l3 = node_factory.get_nodes(3, opts=[
        {'plugin': str(coin_mvt_plugin)},
        {
            'plugin': str(plugin_path),
            'zeroconf-allow': '0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518'
        },
        {}
    ])
    # Advances blockheight to 102
    l1.fundwallet(10**6)
    push_msat = 20000 * 1000
    l1.connect(l2)
    l1.rpc.fundchannel(l2.info['id'], 'all', mindepth=0, push_msat=push_msat)

    # Wait for the update to be signed (might not be the most reliable
    # signal)
    l1.daemon.wait_for_log(r'Got WIRE_HSMD_CUPDATE_SIG_REQ')
    l2.daemon.wait_for_log(r'Got WIRE_HSMD_CUPDATE_SIG_REQ')

    l1chan = only_one(l1.rpc.listpeerchannels()['channels'])
    l2chan = only_one(l2.rpc.listpeerchannels()['channels'])
    channel_id = l1chan['channel_id']

    # We have no confirmation yet, so no `short_channel_id`
    assert('short_channel_id' not in l1chan)
    assert('short_channel_id' not in l2chan)

    # Channel is "proposed"
    chan_val = 993888000 if chainparams['elements'] else 996363000
    l1_mvts = [
        {'type': 'chain_mvt', 'credit_msat': chan_val, 'debit_msat': 0, 'tags': ['channel_proposed', 'opener']},
        {'type': 'channel_mvt', 'credit_msat': 0, 'debit_msat': 20000000, 'tags': ['pushed'], 'fees_msat': '0msat'},
    ]
    check_coin_moves(l1, l1chan['channel_id'], l1_mvts, chainparams)

    # Check that the channel_open event has blockheight of zero
    for n in [l1, l2]:
        evs = n.rpc.bkpr_listaccountevents(channel_id)['events']
        open_ev = only_one([e for e in evs if e['tag'] == 'channel_proposed'])
        assert open_ev['blockheight'] == 0

        # Call inspect, should have pending event in it
        tx = only_one(n.rpc.bkpr_inspect(channel_id)['txs'])
        assert 'blockheight' not in tx
        assert only_one(tx['outputs'])['output_tag'] == 'channel_proposed'

    # Now add 1 confirmation, we should get a `short_channel_id` (block 103)
    bitcoind.generate_block(1)
    l1.daemon.wait_for_log(r'Funding tx [a-f0-9]{64} depth 1 of 0')
    l2.daemon.wait_for_log(r'Funding tx [a-f0-9]{64} depth 1 of 0')

    l1chan = only_one(l1.rpc.listpeerchannels()['channels'])
    l2chan = only_one(l2.rpc.listpeerchannels()['channels'])
    assert('short_channel_id' in l1chan)
    assert('short_channel_id' in l2chan)

    # We also now have an 'open' event, the push event isn't re-recorded
    l1_mvts += [
        {'type': 'chain_mvt', 'credit_msat': chan_val, 'debit_msat': 0, 'tags': ['channel_open', 'opener']},
    ]
    check_coin_moves(l1, channel_id, l1_mvts, chainparams)

    # Check that there is a channel_open event w/ real blockheight
    for n in [l1, l2]:
        evs = n.rpc.bkpr_listaccountevents(channel_id)['events']
        # Still has the channel-proposed event
        only_one([e for e in evs if e['tag'] == 'channel_proposed'])
        open_ev = only_one([e for e in evs if e['tag'] == 'channel_open'])
        assert open_ev['blockheight'] == 103

        # Call inspect, should have open event in it
        tx = only_one(n.rpc.bkpr_inspect(channel_id)['txs'])
        assert tx['blockheight'] == 103
        assert only_one(tx['outputs'])['output_tag'] == 'channel_open'

    # Now make it public, we should be switching over to the real
    # scid.
    bitcoind.generate_block(5)
    # Wait for l3 to learn about the channel, it'll have checked the
    # funding outpoint, scripts, etc.
    l3.connect(l1)
    wait_for(lambda: len(l3.rpc.listchannels()['channels']) == 2)

    # Close the zerconf channel, check that we mark it as onchain_resolved ok
    l1.rpc.close(l2.info['id'])
    bitcoind.generate_block(1, wait_for_mempool=1)

    # Channel should be marked resolved
    for n in [l1, l2]:
        wait_for(lambda: only_one([x for x in n.rpc.bkpr_listbalances()['accounts'] if x['account'] == channel_id])['account_resolved'])


def test_zeroconf_forward(node_factory, bitcoind):
    """Ensure that we can use zeroconf channels in forwards.

    Test that we add routehints using the zeroconf channel, and then
    ensure that l2 uses the alias from the routehint to forward the
    payment. Then do the inverse by sending from l3 to l1, first hop
    being the zeroconf channel

    """
    plugin_path = Path(__file__).parent / "plugins" / "zeroconf-selective.py"
    opts = [
        {},
        {},
        {
            'plugin': str(plugin_path),
            'zeroconf-allow': '022d223620a359a47ff7f7ac447c85c46c923da53389221a0054c11c1e3ca31d59'
        }
    ]
    l1, l2, l3 = node_factory.get_nodes(3, opts=opts)

    l1.connect(l2)
    l1.fundchannel(l2, 10**6)
    bitcoind.generate_block(6)

    l2.connect(l3)
    l2.fundwallet(10**7)
    l2.rpc.fundchannel(l3.info['id'], 10**6, mindepth=0)
    wait_for(lambda: l3.rpc.listincoming()['incoming'] != [])

    # Make sure (esp in non-dev-mode) blockheights agree so we don't WIRE_EXPIRY_TOO_SOON...
    sync_blockheight(bitcoind, [l1, l2, l3])
    inv = l3.rpc.invoice(42 * 10**6, 'inv1', 'desc')['bolt11']
    l1.rpc.pay(inv)

    # And now try the other way around: zeroconf channel first
    # followed by a public one.
    wait_for(lambda: len(l3.rpc.listchannels()['channels']) == 4)

    # Make sure all htlcs completely settled!
    wait_for(lambda: (p['htlcs'] == [] for p in l2.rpc.listpeerchannels()['channels']))

    inv = l1.rpc.invoice(42, 'back1', 'desc')['bolt11']
    l3.rpc.pay(inv)


@pytest.mark.openchannel('v1')
def test_buy_liquidity_ad_no_v2(node_factory, bitcoind):
    """ Test that you can't actually request amt for a
    node that doesn' support v2 opens """

    l1, l2, = node_factory.get_nodes(2)
    amount = 500000
    feerate = 2000

    l1.fundwallet(amount * 100)
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)

    # l1 leases a channel from l2
    with pytest.raises(RpcError, match=r"Tried to buy a liquidity ad but we[(][?][)] don't have experimental-dual-fund enabled"):
        l1.rpc.fundchannel(l2.info['id'], amount, request_amt=amount,
                           feerate='{}perkw'.format(feerate),
                           compact_lease='029a002d000000004b2003e8')


@pytest.mark.openchannel('v2')
def test_v2_replay_bookkeeping(node_factory, bitcoind):
    """ Test that your bookkeeping for a liquidity ad is good
        even if we replay the opening and locking tx!
    """

    opts = [{'funder-policy': 'match', 'funder-policy-mod': 100,
             'lease-fee-base-sat': '100sat', 'lease-fee-basis': 100,
             'rescan': 10, 'funding-confirms': 6, 'may_reconnect': True},
            {'funder-policy': 'match', 'funder-policy-mod': 100,
             'lease-fee-base-sat': '100sat', 'lease-fee-basis': 100,
             'may_reconnect': True}]
    l1, l2, = node_factory.get_nodes(2, opts=opts)
    amount = 500000
    feerate = 2000

    l1.fundwallet(amount * 100)
    l2.fundwallet(amount * 100)

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    rates = l1.rpc.dev_queryrates(l2.info['id'], amount, amount)
    wait_for(lambda: len(l1.rpc.listpeers(l2.info['id'])['peers']) == 0)
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)

    # l1 leases a channel from l2
    l1.rpc.fundchannel(l2.info['id'], amount, request_amt=amount,
                       feerate='{}perkw'.format(feerate),
                       compact_lease=rates['compact_lease'])

    # add the funding transaction
    bitcoind.generate_block(4, wait_for_mempool=1)

    l1.restart()

    bitcoind.generate_block(2)
    l1.daemon.wait_for_log('to CHANNELD_NORMAL')

    chan_id = first_channel_id(l1, l2)
    ev_tags = [e['tag'] for e in l1.rpc.bkpr_listaccountevents(chan_id)['events']]
    assert 'lease_fee' in ev_tags

    # This should work ok
    l1.rpc.bkpr_listbalances()

    bitcoind.generate_block(2)
    sync_blockheight(bitcoind, [l1])

    l1.restart()

    chan_id = first_channel_id(l1, l2)
    ev_tags = [e['tag'] for e in l1.rpc.bkpr_listaccountevents(chan_id)['events']]
    assert 'lease_fee' in ev_tags

    l1.rpc.close(l2.info['id'], 1)
    bitcoind.generate_block(6, wait_for_mempool=1)

    l1.daemon.wait_for_log(' to ONCHAIN')
    l2.daemon.wait_for_log(' to ONCHAIN')

    # This should not crash
    l1.rpc.bkpr_listbalances()


@pytest.mark.openchannel('v2')
def test_buy_liquidity_ad_check_bookkeeping(node_factory, bitcoind):
    """ Test that your bookkeeping for a liquidity ad is good."""

    opts = [{'funder-policy': 'match', 'funder-policy-mod': 100,
             'lease-fee-base-sat': '100sat', 'lease-fee-basis': 100,
             'rescan': 10, 'disable-plugin': 'bookkeeper',
             'funding-confirms': 6, 'may_reconnect': True},
            {'funder-policy': 'match', 'funder-policy-mod': 100,
             'lease-fee-base-sat': '100sat', 'lease-fee-basis': 100,
             'may_reconnect': True}]
    l1, l2, = node_factory.get_nodes(2, opts=opts)
    amount = 500000
    feerate = 2000

    l1.fundwallet(amount * 100)
    l2.fundwallet(amount * 100)

    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)
    rates = l1.rpc.dev_queryrates(l2.info['id'], amount, amount)
    wait_for(lambda: len(l1.rpc.listpeers(l2.info['id'])['peers']) == 0)
    l1.rpc.connect(l2.info['id'], 'localhost', l2.port)

    # l1 leases a channel from l2
    l1.rpc.fundchannel(l2.info['id'], amount, request_amt=amount,
                       feerate='{}perkw'.format(feerate),
                       compact_lease=rates['compact_lease'])

    # add the funding transaction
    bitcoind.generate_block(4, wait_for_mempool=1)

    l1.stop()
    del l1.daemon.opts['disable-plugin']
    l1.start()

    bitcoind.generate_block(2)
    l1.daemon.wait_for_log('to CHANNELD_NORMAL')

    chan_id = first_channel_id(l1, l2)
    ev_tags = [e['tag'] for e in l1.rpc.bkpr_listaccountevents(chan_id)['events']]
    assert 'lease_fee' in ev_tags

    # This should work ok
    l1.rpc.bkpr_listbalances()

    l1.rpc.close(l2.info['id'], 1)
    bitcoind.generate_block(6, wait_for_mempool=1)

    l1.daemon.wait_for_log(' to ONCHAIN')
    l2.daemon.wait_for_log(' to ONCHAIN')

    # This should not crash
    l1.rpc.bkpr_listbalances()


def test_scid_alias_private(node_factory, bitcoind):
    """Test that we don't allow use of real scid for scid_alias-type channels"""
    l1, l2, l3 = node_factory.line_graph(3, fundchannel=False, opts=[{}, {},
                                                                     {'log-level': 'io'}])

    l2.fundwallet(5000000)
    l2.rpc.fundchannel(l3.info['id'], 'all', announce=False)

    bitcoind.generate_block(1, wait_for_mempool=1)
    wait_for(lambda: only_one(l2.rpc.listpeerchannels(l3.info['id'])['channels'])['state'] == 'CHANNELD_NORMAL')

    chan = only_one(l2.rpc.listpeerchannels(l3.info['id'])['channels'])
    assert chan['private'] is True
    scid23 = chan['short_channel_id']
    alias23 = chan['alias']['local']

    # Create l1<->l2 channel, make sure l3 sees it so it will routehint via
    # l2 (otherwise it sees it as a deadend!)
    l1.fundwallet(5000000)
    l1.rpc.fundchannel(l2.info['id'], 'all')
    bitcoind.generate_block(6, wait_for_mempool=1)
    wait_for(lambda: len(l3.rpc.listchannels(source=l1.info['id'])['channels']) == 1)

    chan = only_one(l1.rpc.listpeerchannels(l2.info['id'])['channels'])
    assert chan['private'] is False
    scid12 = chan['short_channel_id']

    # Make sure it sees both sides of private channel in gossmap!
    wait_for(lambda: len(l3.rpc.listchannels()['channels']) == 4)

    # BOLT #2:
    # - if `channel_type` has `option_scid_alias` set:
    #    - MUST NOT use the real `short_channel_id` in BOLT 11 `r` fields.
    inv = l3.rpc.invoice(10, 'test_scid_alias_private', 'desc')
    assert only_one(only_one(l1.rpc.decode(inv['bolt11'])['routes']))['short_channel_id'] == alias23

    # BOLT #2:
    # - if `channel_type` has `option_scid_alias` set:
    #   - MUST NOT allow incoming HTLCs to this channel using the real `short_channel_id`
    route = [{'amount_msat': 11,
              'id': l2.info['id'],
              'delay': 12,
              'channel': scid12},
             {'amount_msat': 10,
              'id': l3.info['id'],
              'delay': 6,
              'channel': scid23}]
    l1.rpc.sendpay(route, inv['payment_hash'], payment_secret=inv['payment_secret'])
    with pytest.raises(RpcError) as err:
        l1.rpc.waitsendpay(inv['payment_hash'])

    # PERM|10
    WIRE_UNKNOWN_NEXT_PEER = 0x4000 | 10
    assert err.value.error['data']['failcode'] == WIRE_UNKNOWN_NEXT_PEER
    assert err.value.error['data']['erring_node'] == l2.info['id']
    assert err.value.error['data']['erring_channel'] == scid23

    # BOLT #2
    # - MUST always recognize the `alias` as a `short_channel_id` for incoming HTLCs to this channel.
    route[1]['channel'] = alias23
    l1.rpc.sendpay(route, inv['payment_hash'], payment_secret=inv['payment_secret'])
    l1.rpc.waitsendpay(inv['payment_hash'])


def test_zeroconf_multichan_forward(node_factory):
    """The freedom to choose the forward channel bytes us when it is 0conf

    Reported by Breez, we crashed when logging in `forward_htlc` when
    the replacement channel was a zeroconf channel.

    l2 -> l3 is a double channel with the zeroconf channel having a
    higher spendable msat, which should cause it to be chosen instead.

    """
    node_id = '022d223620a359a47ff7f7ac447c85c46c923da53389221a0054c11c1e3ca31d59'
    plugin_path = Path(__file__).parent / "plugins" / "zeroconf-selective.py"
    l1, l2, l3 = node_factory.line_graph(3, opts=[
        {},
        {},
        {
            'plugin': str(plugin_path),
            'zeroconf-allow': node_id,
        }
    ], fundamount=10**6, wait_for_announce=True)

    # Just making sure the allowlisted node_id matches.
    assert l2.info['id'] == node_id

    # Now create a channel that is twice as large as the real channel,
    # and don't announce it.
    l2.fundwallet(10**7)
    zeroconf_cid = l2.rpc.fundchannel(l3.info['id'], 2 * 10**6, mindepth=0)['channel_id']

    l2.daemon.wait_for_log(r'peer_in WIRE_CHANNEL_READY')
    l3.daemon.wait_for_log(r'peer_in WIRE_CHANNEL_READY')

    inv = l3.rpc.invoice(amount_msat=10000, label='lbl1', description='desc')['bolt11']
    l1.rpc.pay(inv)

    for c in l2.rpc.listpeerchannels(l3.info['id'])['channels']:
        if c['channel_id'] == zeroconf_cid:
            zeroconf_scid = c['alias']['local']
        else:
            normal_scid = c['short_channel_id']

    assert l2.daemon.is_in_log(r'Chose a better channel than {}: {}'
                               .format(normal_scid, zeroconf_scid))


def test_zeroreserve(node_factory, bitcoind):
    """Ensure we can set the reserves.

    3 nodes:
     - l1 enforces zeroreserve
     - l2 enforces default reserve
     - l3 enforces sub-dust reserves
    """
    plugin_path = Path(__file__).parent / "plugins" / "zeroreserve.py"
    opts = [
        {
            'plugin': str(plugin_path),
            'reserve': '0sat',
            'dev-allowdustreserve': True,
        },
        {
            'dev-allowdustreserve': True,
        },
        {
            'plugin': str(plugin_path),
            'reserve': '123sat',
            'dev-allowdustreserve': True,
        }
    ]
    l1, l2, l3 = node_factory.get_nodes(3, opts=opts)

    l1.fundwallet(10**7)
    l2.fundwallet(10**7)
    l3.fundwallet(10**7)

    l1.connect(l2)
    l2.connect(l3)
    l3.connect(l1)

    l1.rpc.fundchannel(l2.info['id'], 10**6, reserve='0sat')
    l2.rpc.fundchannel(l3.info['id'], 10**6)
    l3.rpc.fundchannel(l1.info['id'], 10**6, reserve='321sat')
    bitcoind.generate_block(1, wait_for_mempool=3)
    wait_for(lambda: l1.channel_state(l2) == 'CHANNELD_NORMAL')
    wait_for(lambda: l2.channel_state(l3) == 'CHANNELD_NORMAL')
    wait_for(lambda: l3.channel_state(l1) == 'CHANNELD_NORMAL')

    # Now make sure we all agree on each others reserves
    l1c1 = l1.rpc.listpeerchannels(l2.info['id'])['channels'][0]
    l2c1 = l2.rpc.listpeerchannels(l1.info['id'])['channels'][0]
    l2c2 = l2.rpc.listpeerchannels(l3.info['id'])['channels'][0]
    l3c2 = l3.rpc.listpeerchannels(l2.info['id'])['channels'][0]
    l3c3 = l3.rpc.listpeerchannels(l1.info['id'])['channels'][0]
    l1c3 = l1.rpc.listpeerchannels(l3.info['id'])['channels'][0]

    # l1 imposed a 0sat reserve on l2, while l2 imposed the default 1% reserve on l1
    assert l1c1['their_reserve_msat'] == l2c1['our_reserve_msat'] == Millisatoshi('0sat')
    assert l1c1['our_reserve_msat'] == l2c1['their_reserve_msat'] == Millisatoshi('10000sat')

    # l2 imposed the default 1% on l3, while l3 imposed a custom 123sat fee on l2
    assert l2c2['their_reserve_msat'] == l3c2['our_reserve_msat'] == Millisatoshi('10000sat')
    assert l2c2['our_reserve_msat'] == l3c2['their_reserve_msat'] == Millisatoshi('123sat')

    # l3 imposed a custom 321sat fee on l1, while l1 imposed a custom 0sat fee on l3
    assert l3c3['their_reserve_msat'] == l1c3['our_reserve_msat'] == Millisatoshi('321sat')
    assert l3c3['our_reserve_msat'] == l1c3['their_reserve_msat'] == Millisatoshi('0sat')

    # Now do some drain tests on c1, as that should be drainable
    # completely by l2 being the fundee
    l1.rpc.keysend(l2.info['id'], 10 * 7)  # Something above dust for sure
    l2.drain(l1)

    # Remember that this is the reserve l1 imposed on l2, so l2 can drain completely
    l2c1 = l2.rpc.listpeerchannels(l1.info['id'])['channels'][0]

    # And despite us briefly being above dust (with a to_us output),
    # closing should result in the output being trimmed again since we
    # dropped below dust again.
    c = l2.rpc.close(l1.info['id'])
    decoded = bitcoind.rpc.decoderawtransaction(c['tx'])
    # Elements has a change output always
    assert len(decoded['vout']) == 1 if TEST_NETWORK == 'regtest' else 2


def test_zeroreserve_mixed(node_factory, bitcoind):
    """l1 runs with zeroreserve, l2 and l3 without, should still work

    Basically tests that l1 doesn't get upset when l2 allows us to
    drop below dust.

    """
    plugin_path = Path(__file__).parent / "plugins" / "zeroreserve.py"
    opts = [
        {
            'plugin': str(plugin_path),
            'reserve': '0sat',
            'dev-allowdustreserve': True,
        }, {
            'dev-allowdustreserve': False,
        }, {
            'dev-allowdustreserve': False,
        }
    ]
    l1, l2, l3 = node_factory.get_nodes(3, opts=opts)
    l1.fundwallet(10**7)
    l3.fundwallet(10**7)

    l1.connect(l2)
    l3.connect(l1)

    l1.rpc.fundchannel(l2.info['id'], 10**6, reserve='0sat')
    l3.rpc.fundchannel(l1.info['id'], 10**6)


def test_zeroreserve_alldust(node_factory):
    """If we allow dust reserves we need larger fundings

    This is because we might have up to

      allhtlcs = (local.max_concurrent_htlcs + remote.max_concurrent_htlcs)
      alldust = allhlcs * min(local.dust, remote.dust)

    allocated to HTLCs in flight, reducing both direct outputs to
    dust. This could leave us with no outs on the commitment, is
    therefore invalid.

    Parameters are as follows:
     - Regtest:
       - max_concurrent_htlcs = 483
       - dust = 546sat
       - minfunding = (483 * 2 + 2) * 546sat = 528528sat
     - Mainnet:
       - max_concurrent_htlcs = 30
       - dust = 546sat
       - minfunding = (30 * 2 + 2) * 546sat = 33852s
    """
    plugin_path = Path(__file__).parent / "plugins" / "zeroreserve.py"
    l1, l2 = node_factory.get_nodes(2, opts=[{
        'plugin': plugin_path,
        'reserve': '0sat',
        'dev-allowdustreserve': True
    }] * 2)
    maxhtlc = 483
    mindust = 546
    minfunding = (maxhtlc * 2 + 2) * mindust

    l1.fundwallet(10**6)
    error = (f'channel funding {minfunding}sat too small for chosen parameters: '
             f'a total of {maxhtlc * 2} HTLCs with dust value {mindust}sat would '
             f'result in a commitment_transaction without outputs')

    # This is right on the edge, and should fail
    with pytest.raises(RpcError, match=error):
        l1.connect(l2)
        l1.rpc.fundchannel(l2.info['id'], minfunding)

    # Now try with just a bit more
    l1.connect(l2)
    l1.rpc.fundchannel(l2.info['id'], minfunding + 1)


def test_coinbase_unspendable(node_factory, bitcoind):
    """ A node should not be able to spend a coinbase output
        before it's mature """

    [l1] = node_factory.get_nodes(1)

    addr = l1.rpc.newaddr()["bech32"]
    bitcoind.rpc.generatetoaddress(1, addr)

    addr2 = l1.rpc.newaddr()["bech32"]

    # Wait til money in wallet
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) == 1)
    out = only_one(l1.rpc.listfunds()['outputs'])
    assert out['status'] == 'immature'

    with pytest.raises(RpcError, match='Could not afford all using all 0 available UTXOs'):
        l1.rpc.withdraw(addr2, "all")

    # Nothing sent to the mempool!
    assert len(bitcoind.rpc.getrawmempool()) == 0

    # Mine 98 blocks
    bitcoind.rpc.generatetoaddress(98, l1.rpc.newaddr()['bech32'])
    assert len([out for out in l1.rpc.listfunds()['outputs'] if out['status'] == 'confirmed']) == 0
    with pytest.raises(RpcError, match='Could not afford all using all 0 available UTXOs'):
        l1.rpc.withdraw(addr2, "all")

    # One more and the first coinbase unlocks
    bitcoind.rpc.generatetoaddress(1, l1.rpc.newaddr()['bech32'])
    wait_for(lambda: len(l1.rpc.listfunds()['outputs']) == 100)
    assert len([out for out in l1.rpc.listfunds()['outputs'] if out['status'] == 'confirmed']) == 1
    l1.rpc.withdraw(addr2, "all")
    # One tx in the mempool now!
    assert len(bitcoind.rpc.getrawmempool()) == 1

    # Mine one block, assert one more is spendable
    bitcoind.rpc.generatetoaddress(1, l1.rpc.newaddr()['bech32'])
    assert len([out for out in l1.rpc.listfunds()['outputs'] if out['status'] == 'confirmed']) == 1
