HOWTO USE TOR WITH C-LIGHTNING

to use tor you have to have tor installed an running.

i.e.
sudo apt install tor
/etc/init.d/tor start

if new to tor you might not change the default setting
# The safe default with minimal harassment (See tor FAQ)
ExitPolicy reject *:* # no exits allowed

this does not effect c-ln connect listen etc.
it will only prevent that you become a full exitpoint
Only enable this if you are sure about the implications.


if you want an auto service created
edit the torconfig file /etc/tor/torrc

set
ControlPort 9051
CookieAuthentication 1
CookieAuthFileGroupReadable 1

or create a password with

cmdline
tor --hash-password yourepassword

this returns an line like
16:533E3963988E038560A8C4EE6BBEE8DB106B38F9C8A7F81FE38D2A3B1F

put this in the /etc/tor/torrc file

i.e.
HashedControlPassword 16:533E3963988E038560A8C4EE6BBEE8DB106B38F9C8A7F81FE38D2A3B1F

save
and
/etc/init.d/tor restart

then you can use c-lightning with following options

--tor-service-password=yourpassword to access the tor service at 9051

--proxy=127.0.0.1:9050 : set the Tor proxy to use

or the password for the service if cookiefile is not accessable

--tor-auto-listen true : try to generate an temp V2 onion addr

NOTE if --tor-proxy set all traffic will be rooted over the proxy
if --ipaddr left empty only the auto generated onion addr will be used for your node

you can also set a fixed onion addr by option
--tor-external = xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.onion  (V2 or V3 is allowed)

this addr can be created by

HiddenServiceDir /var/lib/tor/bitcoin-service_v2/
HiddenServiceVersion 2
HiddenServicePort 8333 127.0.0.1:8333


HiddenServiceDir /var/lib/tor/other_hidden_service_v3/
HiddenServiceVersion 3
HiddenServicePort 9735 127.0.0.1:9735

in /etc/tor/torrc

the addr for
the --tor-external option

you find after /etc/init.d/tor restart

i.e.
in /var/lib/tor/other_hidden_service_v3/hostname


to see your onion addr use
cli/lightning-cli getinfo

some examples:

sudo lightningd/lightningd --network=testnet --ipaddr=127.0.0.1 --port=1234
--proxy=127.0.0.1:9050 --tor-auto-listen true

this will try to generate an V2 auto hidden-service by reading the tor cookie and
also create local ipaddr at port 1234
so the node is accessableby connect peerid xxxxxxxxxxxxxxxx.onion 1234
or local by connect ID 127.0.0.1 1234

lightningd/lightningd --network=testnet --port=1234
--proxy=127.0.0.1:9050 --tor-service-password testpassword --tor-auto-listen true

this will try to generate an V2 auto temp hidden-service addr by using the password to access tor service api
so the node accessable by connect peerid xxxxxxxxxxxxxxxxxxx.onion 1234


lightningd/lightningd --network=testnet --port=1234
--proxy=127.0.0.1:9050 --tor-external=xxxxxxxxxxxxxxxxxxxxxxxxxxxx.onion --port 1234

this will use the hidden-service set by /etc/tor/torrc and use the hidden service
so the node is  accessable by connect peerid xxxxxxxxxxxxxxxxxxxxxxxx.onion 1234
or
lightningd/lightningd --network=testnet --port=1234
--proxy=127.0.0.1:9050 --ipaddr=xxxxxxxxxxxxxxxxxxxxxxxxxxxx.onion --port 1234
this will use the hidden-service set by /etc/tor/torrc and use the hidden service
so the node is only accessable by connect peerid xxxxxxxxxxxxxxxxxxxxxxxonion 1234

for connects you can use
i.e cli/lightning-cli connect peerID xxxxxxxxxxxxxxxxxxxxxxx.onion 1234



