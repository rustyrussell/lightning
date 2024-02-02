lightning-recover -- Reinitialize Your Node for Recovery
========================================================

SYNOPSIS
--------

**recover** *hsmsecret*

DESCRIPTION
-----------

The **recover** RPC command wipes your node and restarts it with
the `--recover` option.  This is only permitted if the node is unused:
no channels, no bitcoin addresses issued (you can use `check` to see
if recovery is possible).

*hsmsecret* is either a codex32 secret starting with "cl1" as returned
by `hsmtool getcodexsecret`, or a raw 64 character hex string.

NOTE: this command only currently works with the `sqlite3` database backend.

RETURN VALUE
------------

[comment]: # (GENERATE-FROM-SCHEMA-START)
On success, an object is returned, containing:

- **result** (string) (always "Recovery restart in progress") *(added v24.05)*

[comment]: # (GENERATE-FROM-SCHEMA-END)

AUTHOR
------

Rusty Russell <<rusty@blockstream.com>> is mainly responsible.

SEE ALSO
--------

lightning-hsmtool(7)

RESOURCES
---------

Main web site: <https://github.com/ElementsProject/lightning>

[comment]: # ( SHA256STAMP:f60d9428ffc0af0d902098eeada40fce9d60083271bc62954ae7be7b881fe7f7)
