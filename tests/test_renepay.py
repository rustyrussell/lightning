from fixtures import *  # noqa: F401,F403
from fixtures import TEST_NETWORK
from pathlib import Path
from io import BytesIO
from pyln.client import RpcError, Millisatoshi
from pyln.proto.onion import TlvPayload
from pyln.testing.utils import EXPERIMENTAL_DUAL_FUND, FUNDAMOUNT, scid_to_int
from utils import (
    DEVELOPER, wait_for, only_one, sync_blockheight, TIMEOUT,
    EXPERIMENTAL_FEATURES, VALGRIND, mine_funding_to_announce, first_scid
)
import copy
import os
import pytest
import random
import re
import string
import struct
import subprocess
import time
import unittest

def test_simple(node_factory):
    '''Testing simply paying a peer.'''
    l1, l2 = node_factory.line_graph(2)
    inv = l2.rpc.invoice(123000, 'test_renepay', 'description')['bolt11']
    before = int(time.time())
    details = l1.rpc.call('renepay',{'invstring':inv})
    after = time.time()
    assert details['status'] == 'complete'
    assert details['amount_msat'] == Millisatoshi(123000)
    assert details['destination'] == l2.info['id']

def test_mpp(node_factory):
    '''Test paying a remote node using two routes.
    1----2----4
    |         |
    3----5----6
    Try paying 1.2M sats from 1 to 6.
    '''
    opts = [
        {'disable-mpp': None, 'fee-base':0, 'fee-per-satoshi':0},
    ]
    l1, l2, l3, l4, l5,l6 = node_factory.get_nodes(6, opts=opts*6)
    node_factory.join_nodes([l1, l2, l4, l6],
        wait_for_announce=True,fundamount=1000000)
    node_factory.join_nodes([l1, l3, l5, l6], 
        wait_for_announce=True,fundamount=1000000)
    
    send_amount = Millisatoshi('1200000sat')
    inv = l6.rpc.invoice(send_amount, 'test_renepay', 'description')['bolt11']
    details = l1.rpc.call('renepay',{'invstring':inv})
    assert details['status'] == 'complete'
    assert details['amount_msat'] == send_amount
    assert details['destination'] == l6.info['id']

def test_errors(node_factory):
    opts = [
        {'disable-mpp': None, 'fee-base':0, 'fee-per-satoshi':0},
    ]
    l1, l2, l3, l4, l5,l6 = node_factory.get_nodes(6, opts=opts*6)
    send_amount = Millisatoshi('21sat')
    inv = l6.rpc.invoice(send_amount, 'test_renepay', 'description')['bolt11']
    
    failmsg = r'We don\'t have any channels'
    with pytest.raises(RpcError, match=failmsg) as err:
        l1.rpc.call('renepay',{'invstring':inv})
    node_factory.join_nodes([l1, l2, l4],
        wait_for_announce=True,fundamount=1000000)
    node_factory.join_nodes([l1, l3, l5], 
        wait_for_announce=True,fundamount=1000000)
    
    failmsg = r'Destination is unreacheable in the network gossip.'
    with pytest.raises(RpcError, match=failmsg) as err:
        l1.rpc.call('renepay',{'invstring':inv})
    
    node_factory.join_nodes([l4,l6],
        wait_for_announce=True,fundamount=1000000)
    node_factory.join_nodes([l5,l6], 
        wait_for_announce=True,fundamount=1000000)
    
    details = l1.rpc.call('renepay',{'invstring':inv, 'use_shadow':False})
    assert details['status'] == 'complete'
    assert details['amount_msat'] == send_amount
    assert details['destination'] == l6.info['id']
		

### SAME TESTS USED IN THE STANDARD PAY COMMAND ###

@pytest.mark.developer("needs to deactivate shadow routing")
@pytest.mark.openchannel('v1')
@pytest.mark.openchannel('v2')
def test_pay(node_factory):
    l1, l2 = node_factory.line_graph(2)

    inv = l2.rpc.invoice(123000, 'test_pay', 'description')['bolt11']
    before = int(time.time())
    details = l1.rpc.call('renepay',{'invstring':inv,'use_shadow':False})
    after = time.time()
    preimage = details['payment_preimage']
    assert details['status'] == 'complete'
    assert details['amount_msat'] == Millisatoshi(123000)
    assert details['destination'] == l2.info['id']
    assert details['created_at'] >= before
    assert details['created_at'] <= after

    invoices = l2.rpc.listinvoices('test_pay')['invoices']
    assert len(invoices) == 1
    invoice = invoices[0]
    assert invoice['status'] == 'paid' and invoice['paid_at'] >= before and invoice['paid_at'] <= after

    # Repeat payments are NOPs (if valid): we can hand null.
    l1.rpc.call('renepay',{'invstring':inv,'use_shadow':False})
    # This won't work: can't provide an amount (even if correct!)
    with pytest.raises(RpcError):
        l1.rpc.call('renepay',{'invstring':inv,'amount_msat':123000})
    with pytest.raises(RpcError):
        l1.rpc.call('renepay',{'invstring':inv,'amount_msat':122000})

    # Check pay_index is not null
    outputs = l2.db_query('SELECT pay_index IS NOT NULL AS q FROM invoices WHERE label="label";')
    assert len(outputs) == 1 and outputs[0]['q'] != 0

    # Check payment of any-amount invoice.
    for i in range(5):
        label = "any{}".format(i)
        inv2 = l2.rpc.invoice("any", label, 'description')['bolt11']
        # Must provide an amount!
        with pytest.raises(RpcError):
            l1.rpc.call('renepay',{'invstring':inv2,'use_shadow':False})
        
        l1.rpc.call('renepay',{'invstring':inv2,'use_shadow':False,'amount_msat':random.randint(1000,999999)})

    # Should see 6 completed payments
    assert len(l1.rpc.listsendpays()['payments']) == 6

    # Test listsendpays indexed by bolt11.
    payments = l1.rpc.listsendpays(inv)['payments']
    assert len(payments) == 1 and payments[0]['payment_preimage'] == preimage
    
    # let's buy some time to let bkpr receive coin_movement notification
    ret = l1.rpc.call('renepaystatus')

    # Check channels apy summary view of channel activity
    apys_1 = l1.rpc.bkpr_channelsapy()['channels_apy']
    apys_2 = l2.rpc.bkpr_channelsapy()['channels_apy']

    assert apys_1[0]['channel_start_balance_msat'] == apys_2[0]['channel_start_balance_msat']
    assert apys_1[0]['channel_start_balance_msat'] == apys_1[0]['our_start_balance_msat']
    assert apys_2[0]['our_start_balance_msat'] == Millisatoshi(0)
    assert apys_1[0]['routed_out_msat'] == apys_2[0]['routed_in_msat']
    assert apys_1[0]['routed_in_msat'] == apys_2[0]['routed_out_msat']


@pytest.mark.developer("needs to deactivate shadow routing")
def test_amounts(node_factory):
    l1, l2 = node_factory.line_graph(2)
    inv = l2.rpc.invoice(Millisatoshi(123456), 'test_pay_amounts', 'description')['bolt11']

    invoice = only_one(l2.rpc.listinvoices('test_pay_amounts')['invoices'])

    assert isinstance(invoice['amount_msat'], Millisatoshi)
    assert invoice['amount_msat'] == Millisatoshi(123456)

    l1.rpc.call('renepay',{'invstring':inv, 'use_shadow':False})

    invoice = only_one(l2.rpc.listinvoices('test_pay_amounts')['invoices'])
    assert isinstance(invoice['amount_received_msat'], Millisatoshi)
    assert invoice['amount_received_msat'] >= Millisatoshi(123456)


@pytest.mark.developer("needs to deactivate shadow routing")
def test_limits(node_factory):
    '''
    Topology:
    1----2----4
    |         |
    3----5----6
    '''
    opts = [
        {'disable-mpp': None, 'fee-base':0, 'fee-per-satoshi':100},
    ]
    l1, l2, l3, l4, l5,l6 = node_factory.get_nodes(6, opts=opts*6)
    node_factory.join_nodes([l1, l2, l4, l6],
        wait_for_announce=True,fundamount=1000000)
    node_factory.join_nodes([l1, l3, l5, l6], 
        wait_for_announce=True,fundamount=1000000)
    
    inv = l4.rpc.invoice('any','any','description')
    l2.rpc.call('pay', {'bolt11': inv['bolt11'], 'amount_msat':500000000})
    inv = l5.rpc.invoice('any','any','description')
    l3.rpc.call('pay', {'bolt11': inv['bolt11'], 'amount_msat':500000000})
    
    # FIXME: pylightning should define these!
    # PAY_STOPPED_RETRYING = 210
    PAY_ROUTE_NOT_FOUND=205

    inv = l6.rpc.invoice("any", "any", 'description')

    # Fee too high.
    failmsg = r'Fee exceeds our fee budget'
    with pytest.raises(RpcError, match=failmsg) as err:
        l1.rpc.call('renepay', {'invstring': inv['bolt11'], 'amount_msat': 1000000, 'maxfee': 1})
    assert err.value.error['code'] == PAY_ROUTE_NOT_FOUND
    # TODO(eduardo): which error code shall we use here?

    # TODO(eduardo): shall we list attempts in renepay?
    # status = l1.rpc.call('renepaystatus', {'invstring':inv['bolt11']})['paystatus'][0]['attempts']

    failmsg = r'CLTV delay exceeds our CLTV budget'
    # Delay too high.
    with pytest.raises(RpcError, match=failmsg) as err:
        l1.rpc.call('renepay', {'invstring': inv['bolt11'], 'amount_msat': 1000000, 'maxdelay': 0})
    assert err.value.error['code'] == PAY_ROUTE_NOT_FOUND
    
    inv2 = l6.rpc.invoice("800000sat", "inv2", 'description')
    failmsg = r'Probability is too small'
    with pytest.raises(RpcError, match=failmsg) as err:
        l1.rpc.call('renepay', {'invstring': inv2['bolt11'],'min_prob_success': '0.5'})
    assert err.value.error['code'] == PAY_ROUTE_NOT_FOUND
    
    # if we try again we can finish this payment
    l1.rpc.call('renepay', {'invstring': inv2['bolt11'],'min_prob_success':0})
    invoice = only_one(l6.rpc.listinvoices('inv2')['invoices'])
    assert isinstance(invoice['amount_received_msat'], Millisatoshi)
    assert invoice['amount_received_msat'] >= Millisatoshi('800000sat')

