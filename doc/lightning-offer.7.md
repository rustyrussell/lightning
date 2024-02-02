lightning-offer -- Command for accepting payments
=================================================

SYNOPSIS
--------

**(WARNING: experimental-offers only)**

**offer** *amount* *description* [*issuer*] [*label*] [*quantity\_max*] [*absolute\_expiry*] [*recurrence*] [*recurrence\_base*] [*recurrence\_paywindow*] [*recurrence\_limit*] [*single\_use*]

DESCRIPTION
-----------

The **offer** RPC command creates an offer (or returns an existing
one), which is a precursor to creating one or more invoices.  It
automatically enables the processing of an incoming invoice\_request,
and issuing of invoices.

Note that for making an offer to *pay* someone else, see lightning-invoicerequest(7).

RETURN VALUE
------------

[comment]: # (GENERATE-FROM-SCHEMA-START)
On success, an object is returned, containing:

- **offer\_id** (hash): the id of this offer (merkle hash of non-signature fields)
- **active** (boolean): whether this can still be used (always *true*)
- **single\_use** (boolean): whether this expires as soon as it's paid (reflects the *single\_use* parameter)
- **bolt12** (string): the bolt12 encoding of the offer
- **used** (boolean): True if an associated invoice has been paid
- **created** (boolean): false if the offer already existed
- **label** (string, optional): the (optional) user-specified label

[comment]: # (GENERATE-FROM-SCHEMA-END)

ERRORS
------

On failure, an error is returned and no offer is created. If the
lightning process fails before responding, the caller should use
lightning-listoffers(7) to query whether this offer was created or
not.

If the offer already existed, and is still active, that is returned;
if it's not active then this call fails.

The following error codes may occur:

- -1: Catchall nonspecific error.
- 1000: Offer with this offer\_id already exists (but is not active).

AUTHOR
------

Rusty Russell <<rusty@rustcorp.com.au>> is mainly responsible.

SEE ALSO
--------

lightning-listoffers(7), lightning-disableoffer(7), lightning-invoicerequest(7).

RESOURCES
---------

Main web site: <https://github.com/ElementsProject/lightning>

[comment]: # ( SHA256STAMP:9938c16e54f86deabe03d86788600f87dffc54492ea4896d4f590916683afb0a)
