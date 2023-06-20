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

def test_pay_to_peer(node_factory):
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
