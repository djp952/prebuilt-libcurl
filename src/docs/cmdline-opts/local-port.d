c: Copyright (C) 1998 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
SPDX-License-Identifier: curl
Long: local-port
Arg: <num/range>
Help: Force use of RANGE for local port numbers
Added: 7.15.2
Category: connection
Example: --local-port 1000-3000 $URL
See-also: globoff
---
Set a preferred single number or range (FROM-TO) of local port numbers to use
for the connection(s).  Note that port numbers by nature are a scarce resource
that will be busy at times so setting this range to something too narrow might
cause unnecessary connection setup failures.
