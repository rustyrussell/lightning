#! /usr/bin/env python
import argparse
import inspect
import json
import os
import secp256k1
import select
import socket
import subprocess
import tempfile
from collections import OrderedDict
from pyln.client import Millisatoshi
from pyln.proto import ShortChannelId

from typing import Any, Callable, Dict, List, Optional, Tuple


class JsonRPC:
    """Abstract Base Class for JSON RPC"""
    def __init__(self):
        self.jsonid = 0
        self.inbuf = b''

    def next_jsonid(self):
        self.jsonid += 1
        return self.jsonid

    def writeobj(self, obj: dict) -> None:
        s = bytes(json.dumps(obj, ensure_ascii=False) + "\n\n", encoding='utf-8')
        self.write(s)

    def send_request_params(self, method: str, params) -> str:
        """Encode and send a JSONRPC request, return json id"""
        thisid = f"cln-pluginate:{method}#{self.next_jsonid()}"
        obj = {"jsonrpc": "2.0",
               "method": method,
               "id": thisid,
               "params": params}
        self.writeobj(obj)
        return thisid

    def send_request(self, method: str, **kwargs) -> str:
        return self.send_request_params(method, kwargs)

    def send_reply(self, jsonid: str, result: dict) -> None:
        """Encode and send a reply to a JSONRPC request"""
        obj = {"jsonrpc": "2.0",
               "id": jsonid,
               "result": result}
        self.writeobj(obj)

    def readobj(self):
        msg = self.inbuf
        while True:
            # We might have more than one JSON object!
            try:
                obj = json.loads(msg)
            except json.JSONDecodeError as e:
                # Too much?
                if e.msg == 'Extra data':
                    msg = self.inbuf[:e.pos]
                else:
                    # Assume not enough
                    self.inbuf += self.read()
                    msg = self.inbuf
                continue

            # We won!  Remove msg from inbuf
            self.inbuf = self.inbuf[len(msg):]
            return obj


class Plugin:
    """A plugin process"""
    class PluginJsonRPC(JsonRPC):
        """Class for plugin's stdin/stdout (log msgs, and our requests to it)"""
        def __init__(self, stdin, stdout):
            self.stdin = stdin
            self.stdout = stdout
            super().__init__()

        def read(self):
            return self.stdout.readline()

        def write(self, buf: bytes):
            self.stdin.write(buf)
            self.stdin.flush()

    def __init__(self, pluginpath: str, deprecated_apis: bool, valgrind: bool) -> None:
        """Start plugin, send getmanifest and read reply"""
        self.inbuf = ""
        if valgrind:
            args = ['valgrind', '-q', pluginpath]
        else:
            args = [pluginpath]
        self.process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.jsonrpc = self.PluginJsonRPC(self.process.stdin, self.process.stdout)
        # Send getmanifest, expect (and ignore!) reply.
        getmanifestid = self.jsonrpc.send_request("getmanifest",
                                                  deprecated_apis=deprecated_apis)
        assert self.jsonrpc.readobj()['id'] == getmanifestid

    def send_init(self, plugindir: str, rpcfile: str) -> str:
        """Send init command, return json id to wait for reply"""
        # FIXME: support options!
        return self.jsonrpc.send_request("init",
                                         options={},
                                         configuration={"lightning-dir": plugindir,
                                                        "rpc-file": rpcfile,
                                                        "startup": True,
                                                        "network": "regtest",
                                                        "feature_set": {
                                                            "init": "02aaa2",
                                                            "node": "8000000002aaa2",
                                                            "channel": "",
                                                            "invoice": "028200"
                                                        },
                                                        "always_use_proxy": False})


class RpcSocket:
    """An RPC Socket for the plugin to connect to and send us commands"""
    class RpcSocketJsonRPC(JsonRPC):
        """Class for rpc socket (plugin makes requests to us)"""
        def __init__(self, conn):
            self.conn = conn
            super().__init__()

        def read(self):
            return self.conn.recv(65536)

        def write(self, buf: bytes):
            return self.conn.send(buf)

    def __init__(self, sockfile):
        self.path = sockfile
        self.inbuf = bytes()
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.bind(self.path)
        self.socket.listen(1)

    def accept(self):
        self.conn, _ = self.socket.accept()
        self.jsonrpc = self.RpcSocketJsonRPC(self.conn)


class DummyService:
    """Class which "services" requests from its own plugin_ methods"""
    def __init__(self, servicedir: Optional[str] = None):
        self.servicedir = servicedir

    def service_from_file(self, method: str):
        """If they give us a directory of canned responses, we try to serve those"""
        if self.servicedir is None:
            return None

        try:
            with open(os.path.join(self.servicedir, method), 'r') as file:
                return json.loads(file.read())
        except FileNotFoundError:
            return None

    def service(self, req: dict):
        """Handle incoming command from plugin"""
        func = getattr(self, "plugin_" + req['method'], None)
        if func is not None:
            return self._exec_func(func, req)
        ret = self.service_from_file(req['method'])
        if ret is not None:
            return ret
        raise NotImplementedError(req['method'])

    # These routines stolen from contrib/pyln-client/pyln/client/plugin.py
    @staticmethod
    def _coerce_arguments(
            func: Callable[..., Any],
            ba: inspect.BoundArguments) -> inspect.BoundArguments:
        args = OrderedDict()
        annotations = {}
        if hasattr(func, "__annotations__"):
            annotations = func.__annotations__

        for key, val in ba.arguments.items():
            annotation = annotations.get(key, None)
            if annotation is not None and annotation == Millisatoshi:
                args[key] = Millisatoshi(val)
            else:
                args[key] = val
        ba.arguments = args
        return ba

    def _bind_pos(self, func: Callable[..., Any], params: List[str],
                  request) -> inspect.BoundArguments:
        """Positional binding of parameters
        """
        assert(isinstance(params, list))
        sig = inspect.signature(func)

        # Collect injections so we can sort them and insert them in the right
        # order later. If we don't apply inject them in increasing order we
        # might shift away an earlier injection.
        injections: List[Tuple[int, Any]] = []
        if 'plugin' in sig.parameters:
            pos = list(sig.parameters.keys()).index('plugin')
            injections.append((pos, self))
        if 'request' in sig.parameters:
            pos = list(sig.parameters.keys()).index('request')
            injections.append((pos, request))
        injections = sorted(injections)
        for pos, val in injections:
            params = params[:pos] + [val] + params[pos:]

        ba = sig.bind(*params)
        self._coerce_arguments(func, ba)
        ba.apply_defaults()
        return ba

    def _bind_kwargs(self, func: Callable[..., Any], params: Dict[str, Any],
                     request) -> inspect.BoundArguments:
        """Keyword based binding of parameters
        """
        assert(isinstance(params, dict))
        sig = inspect.signature(func)

        # Inject additional parameters if they are in the signature.
        if 'plugin' in sig.parameters:
            params['plugin'] = self
        elif 'plugin' in params:
            del params['plugin']
        if 'request' in sig.parameters:
            params['request'] = request
        elif 'request' in params:
            del params['request']

        ba = sig.bind(**params)
        self._coerce_arguments(func, ba)
        return ba

    def _exec_func(self, func: Callable[..., Any],
                   request):
        params = request['params']
        if isinstance(params, list):
            ba = self._bind_pos(func, params, request)
            ret = func(*ba.args, **ba.kwargs)
        elif isinstance(params, dict):
            ba = self._bind_kwargs(func, params, request)
            ret = func(*ba.args, **ba.kwargs)
        else:
            raise TypeError(
                "Parameters to function call must be either a dict or a list."
            )
        return ret


class PayService(DummyService):
    """Class which implements minimum required to support "pay" plugin"""
    def plugin_log(self, level, message):
        print(f"{level}: {message}")


def mkgossipmsg(msg):
    # /**
    #  * gossip_hdr -- On-disk format header.
    #  */
    # struct gossip_hdr {
    # 	beint16_t flags; /* Length of message after header. */
    # 	beint16_t len; /* GOSSIP_STORE_xxx_BIT flags. */
    # 	beint32_t crc; /* crc of message of timestamp, after header. */
    # 	beint32_t timestamp; /* timestamp of msg. */
    # };
    return (0).to_bytes(2) + len(msg).to_bytes(2) + bytes(4) + bytes(4) + msg


class LnChannel:
    def __init__(self, src, dst, size, src_capacity=0):
        def nametoint(name):
            """Alice becomes '65'"""
            int(bytes(name[0], encoding='utf8'))
        self.scid = ShortChannelId(nametoint(src.name), nametoint(dst.name), 99)
        self.src = src
        self.dst = dst
        self.size = size
        assert src_capacity <= size
        self.src_capacity = src_capacity
        self.dst_capacity = size - src_capacity
        # We assume each side keeps 1% reserve
        self.src_reserve = src.dst_reserve = int(size / 100)

    def to_gossip(self) -> bytes:
        # BOLT #7:
        # 1. type: 256 (`channel_announcement`)
        # 2. data:
        #     * [`signature`:`node_signature_1`]
        #     * [`signature`:`node_signature_2`]
        #     * [`signature`:`bitcoin_signature_1`]
        #     * [`signature`:`bitcoin_signature_2`]
        #     * [`u16`:`len`]
        #     * [`len*byte`:`features`]
        #     * [`chain_hash`:`chain_hash`]
        #     * [`short_channel_id`:`short_channel_id`]
        #     * [`point`:`node_id_1`]
        #     * [`point`:`node_id_2`]
        # Nodes in gossip order!
        nodes = sorted([self.src.nodeid, self.dst.nodeid])
        announce = (256).tobytes(2) + bytes(64) * 4 + bytes(2) + bytes(32) + self.scid.to_bytes() + nodes[0] + nodes[1]
        # BOLT #7:
        # 1. type: 258 (`channel_update`)
        # 2. data:
        #     * [`signature`:`signature`]
        #     * [`chain_hash`:`chain_hash`]
        #     * [`short_channel_id`:`short_channel_id`]
        #     * [`u32`:`timestamp`]
        #     * [`byte`:`message_flags`]
        #     * [`byte`:`channel_flags`]
        #     * [`u16`:`cltv_expiry_delta`]
        #     * [`u64`:`htlc_minimum_msat`]
        #     * [`u32`:`fee_base_msat`]
        #     * [`u32`:`fee_proportional_millionths`]
        #     * [`u64`:`htlc_maximum_msat`]
        update = [None, None]
        for dir in (0, 1):
            update[dir] = (258).to_bytes(2) + bytes(64) + bytes(32) + self.scid.to_bytes() + bytes(4) + bytes([0]) + bytes([dir]) + bytes([0, 10]) + bytes(8) + bytes(4) + bytes(4) + self.size.to_bytes(8)

        return mkgossipmsg(announce) + mkgossipmsg(update[0]) + mkgossipmsg(update[1])

    def route(self, amount, dst):
        """Return true if we can route, and update capacity"""
        if dst == self.dst:
            if self.src_capacity > amount + self.src_reserve:
                self.src_capacity -= amount
                return True
        else:
            assert dst == self.src
            if self.dst_capacity > amount + self.dst_reserve:
                self.dst_capacity -= amount
                return True
        return False

    def unroute(self, amount, dst):
        """Payment failed, undo the effects"""
        if dst == self.dst:
            self.src_capacity += amount
            assert self.src_capacity <= self.size
        else:
            assert dst == self.src
            self.dst_capacity += amount
            assert self.dst_capacity <= self.size


class LnNode:
    @staticmethod
    def name2nodeid(name: str):
        if len(name) == 66:
            try:
                return bytes.fromhex(name).hex()
            except ValueError:
                pass
        # privkey is name repeated over and over.
        return secp256k1.PrivateKey(bytes((name * 32)[:32], encoding='utf8'), raw=True).pubkey.serialize()

    def __init__(self, name: str):
        self.nodeid = self.name2nodeid(name)
        self.channels = []

    def add_channel(self, chan: LnChannel):
        self.channels.append(chan)


def make_gossmap(plugindir, chanspecs, base, verbose):
    nodes = {}
    channels = {}
    for c in chanspecs:
        spec = c.split(',')
        srcid = LnNode.name2nodeid(spec[0])
        dstid = LnNode.name2nodeid(spec[1])
        if srcid not in nodes:
            nodes[srcid] = LnNode(spec[0])
        if dstid not in nodes:
            nodes[dstid] = LnNode(spec[1])

        src = nodes[srcid]
        dst = nodes[dstid]
        if len(spec) == 4:
            srccap = int(spec[4])
        else:
            assert len(spec) == 3
            srccap = 0

        chan = LnChannel(src, dst, int(spec[2]), srccap)
        # We assume uniqueness!
        assert chan.scid not in channels
        channels[chan.scid] = chan
        src.add_channel(chan)
        dst.add_channel(chan)

    with open(os.path.join(plugindir, "gossip_store"), "wb") as gs:
        # If --base-gossmap, copy it.
        if base:
            with open(base, "rb") as gs_in:
                gs.write(gs_in.read())
        else:
            # Header: type 0
            gs.write(bytes(0))

        for c in channels:
            gs.write(c.to_gossip())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--deprecated-apis', help='Whether to tell plugin deprecated apis are enabled', type=bool, default=False)
    parser.add_argument('--chan', help='Channel src,dst,sats,srccap', action='append', default=[])
    parser.add_argument('--verbose', help='Make lots of debugging noise')
    parser.add_argument('--valgrind', help='Run plugin under valgrind')
    parser.add_argument('--canned-dir', help='Directory of canned responses', default=None)
    parser.add_argument('--base-gossmap', help='Gossmap to start with (chan adds to this!)', default=None)
    parser.add_argument('plugin', help='The plugin to run')
    parser.add_argument('cmd', nargs=argparse.REMAINDER)

    args = parser.parse_args()
    plugin = Plugin(args.plugin, args.deprecated_apis, args.valgrind)
    service = PayService(args.canned_dir)

    with tempfile.TemporaryDirectory() as tmpdir:
        make_gossmap(tmpdir, args.chan, args.base_gossmap, args.verbose)
        # Create socket so plugin can connect to it.
        sock = RpcSocket(os.path.join(tmpdir, "lightning-rpc"))

        initid = plugin.send_init(tmpdir, sock.path)

        fds = [sock.socket, plugin.process.stdout]

        while True:
            readable, _, _ = select.select(fds, [], [])

            for s in readable:
                # If they connect, switch to listening on that incoming sock!
                if s == sock.socket:
                    sock.accept()
                    fds[0] = sock.conn
                elif s == plugin.process.stdout:
                    obj = plugin.jsonrpc.readobj()
                    if 'id' not in obj:
                        # Notification!
                        print(f"Plugin notification: {obj}")
                    elif 'method' not in obj:
                        if initid is not None:
                            assert obj['id'] == initid
                            print("Plugin initialized:")
                            initid = None
                            # Now we send our cmd from cmdline.
                            plugin.jsonrpc.send_request_params(args.cmd[0], args.cmd[1:])
                        else:
                            print(obj)
                    else:
                        # Really we only get log methods from this.
                        assert obj['method'] == 'log'
                        service.service(obj)
                else:
                    assert s == sock.conn
                    obj = sock.jsonrpc.readobj()
                    reply = service.service(obj)
                    sock.jsonrpc.send_reply(obj['id'], reply)
