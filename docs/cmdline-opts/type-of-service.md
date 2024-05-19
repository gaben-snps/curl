---
c: Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
SPDX-License-Identifier: curl
Long: type-of-service
Arg: <string>
Help: Configure IP Type of Service (TOS)
Added: 8.9.0
Category: connection
Protocols: All
Multi: single
See-also:
  - tcp-nodelay
Example:
  - --type-of-service CS5 $URL
---

# `--type-of-service`

Set IP Type of Service (TOS). (Added in 8.9.0).

The values allowed for \<string\> can be a numeric value between 1 and 255
or one of the following:

* CS0
* CS1
* CS2
* CS3
* CS4
* CS5
* CS6
* CS7
* AF11
* AF12
* AF13
* AF21
* AF22
* AF23
* AF31
* AF32
* AF33
* AF41
* AF42
* AF43
* EF
* ECT1
* ECT0
* CE
* LOWCOST
* LOWDELAY
* THROUGHPUT
* RELIABILITY
* MINCOST

This option is limited to IPv4 connections only. When curl uses IPv6, this option does nothing.
