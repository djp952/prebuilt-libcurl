c: Copyright (C) 1998 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
SPDX-License-Identifier: curl
Long: http2-prior-knowledge
Tags: Versions
Protocols: HTTP
Added: 7.49.0
Mutexed: http1.1 http1.0 http2 http3
Requires: HTTP/2
Help: Use HTTP 2 without HTTP/1.1 Upgrade
Category: http
Example: --http2-prior-knowledge $URL
See-also: http2 http3
Multi: boolean
---
Tells curl to issue its non-TLS HTTP requests using HTTP/2 without HTTP/1.1
Upgrade. It requires prior knowledge that the server supports HTTP/2 straight
away. HTTPS requests will still do HTTP/2 the standard way with negotiated
protocol version in the TLS handshake.
