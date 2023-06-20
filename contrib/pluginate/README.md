# cln-pluginate.py: Run Plugins Without A Real lightningd

It runs the plugin, and gives it a command from the cmdline.

```
$ ./cln-pluginate.py --canned-dir=canned_responses ../../plugins/pay pay lnbcrt1pjfplnxsp5qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpp5wtxkappzcsrlkmgfs6g0zyct0hkhashh7hsaxz7e65slq9fkx7fsdp92pex26tdv9nk2grfwvsrq7psxycrzvp39chzucqpjs5mw7sqs7xt797yc2yqqgtut3khhhmxkrq3pvez5959kqyt9lm7j0d9vg75jsdkqrj6jatxrn3dcree09hq7u3qd394jwsjn6mc87zsq4aea28
```

If you want to populate the gossmap, you can use `--chan=src,dst,sats,srccap`:
* `src` is either the public key of the source, or a string to exapand to a private key, then a pubkey.  eg.  `B` becomes 0x42424242.... `Bob` becomes 0x426F62426F62426F62....
* `dst` is the destination (same format as `src`)
* `sats` is the capacity of the channel.
* `srccap` is the actual spendable balance on the src side (<= `sats`).

Note that this node's private key is `A` (i.e 0x414141...) so public key is `02eec7245d6b7d2ccb30380bfbe2a3648cd7a942653f5aa340edcea1f283686619`.

You can also use `--base-gossmap` to start with an existing gossmap (`--chan` will append to this).

Happy hacking!
Rusty Russell.
