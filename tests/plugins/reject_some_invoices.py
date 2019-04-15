#!/usr/bin/env python3
"""Simple plugin to test the invoice_payment_hook.

We just refuse to let them pay invoices with preimages divisible by 16.
"""

from lightning import Plugin, PluginError

plugin = Plugin()


@plugin.hook('invoice_payment')
def on_payment(payment, plugin):
    print("label={}".format(payment['label']))
    print("msat={}".format(payment['msat']))
    print("preimage={}".format(payment['preimage']))

    if payment['preimage'].endswith('0'):
        # FIXME: Define this!
        WIRE_TEMPORARY_NODE_FAILURE = 0x2002
        raise PluginError("Don't like you!",
                          data={'failure_code': WIRE_TEMPORARY_NODE_FAILURE})

    return {}


plugin.run()
