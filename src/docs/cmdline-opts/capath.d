c: Copyright (C) 1998 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
SPDX-License-Identifier: curl
Long: capath
Arg: <dir>
Help: CA directory to verify peer against
Protocols: TLS
Category: tls
See-also: cacert insecure
Example: --capath /local/directory $URL
Added: 7.9.8
Multi: single
---
Tells curl to use the specified certificate directory to verify the
peer. Multiple paths can be provided by separating them with ":" (e.g.
"path1:path2:path3"). The certificates must be in PEM format, and if curl is
built against OpenSSL, the directory must have been processed using the
c_rehash utility supplied with OpenSSL. Using --capath can allow
OpenSSL-powered curl to make SSL-connections much more efficiently than using
--cacert if the --cacert file contains many CA certificates.

If this option is set, the default capath value will be ignored.
