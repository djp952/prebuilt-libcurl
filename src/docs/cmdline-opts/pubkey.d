c: Copyright (C) 1998 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
SPDX-License-Identifier: curl
Long: pubkey
Arg: <key>
Protocols: SFTP SCP
Help: SSH Public key file name
Category: sftp scp auth
Example: --pubkey file.pub sftp://example.com/
Added: 7.16.2
See-also: pass
Multi: single
---
Public key file name. Allows you to provide your public key in this separate
file.

(As of 7.39.0, curl attempts to automatically extract the public key from the
private key file, so passing this option is generally not required. Note that
this public key extraction requires libcurl to be linked against a copy of
libssh2 1.2.8 or higher that is itself linked against OpenSSL.)
