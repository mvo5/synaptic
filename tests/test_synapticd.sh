#!/bin/sh
# Drives synapticd the two ways it can be started: varlinkctl hands the
# socket over via LISTEN_FDS, the GUI will hand it over as stdin/stdout.
# Unprivileged the daemon cannot take the locks but must still answer.
set -eu

daemon="$1"
varlinkctl="$2"
idl="$3"

echo "# Status via LISTEN_FDS"
out="$("$varlinkctl" call "$daemon" io.github.mvo5.synaptic.Status '{}')"
case "$out" in
   *'"locked":'*) ;;
   *) echo "unexpected Status reply: $out" >&2; exit 1 ;;
esac

echo "# UpdateCache must refuse without the locks"
out="$("$varlinkctl" call --more "$daemon" io.github.mvo5.synaptic.UpdateCache '{}' 2>&1 || true)"
case "$out" in
   *NotLocked*) ;;
   *) echo "unexpected UpdateCache reply: $out" >&2; exit 1 ;;
esac

echo "# Commit validates its options before anything else"
out="$("$varlinkctl" call --more "$daemon" io.github.mvo5.synaptic.Commit \
   '{"selections":[],"options":{"conffile":"ask"}}' 2>&1 || true)"
case "$out" in
   *InvalidParameter*conffile*) ;;
   *) echo "unexpected Commit reply: $out" >&2; exit 1 ;;
esac

echo "# Commit must refuse without the locks"
out="$("$varlinkctl" call --more "$daemon" io.github.mvo5.synaptic.Commit \
   '{"selections":[{"name":"hello","arch":"amd64","action":"install","version":"1.0","auto":false}],"options":{"conffile":"keep"}}' 2>&1 || true)"
case "$out" in
   *NotLocked*) ;;
   *) echo "unexpected Commit reply: $out" >&2; exit 1 ;;
esac

echo "# Status via a socketpair on stdin/stdout"
if command -v python3 >/dev/null; then
   python3 - "$daemon" <<'PY'
import socket, subprocess, sys
ours, theirs = socket.socketpair()
p = subprocess.Popen([sys.argv[1]], stdin=theirs, stdout=theirs)
theirs.close()
ours.sendall(b'{"method":"io.github.mvo5.synaptic.Status"}\0')
reply = b""
while not reply.endswith(b"\0"):
    chunk = ours.recv(4096)
    if not chunk:
        sys.exit("daemon closed the connection: " + reply.decode())
    reply += chunk
ours.close()
if b'"locked":' not in reply:
    sys.exit("unexpected reply: " + reply.decode())
if p.wait(timeout=10) != 0:
    sys.exit("daemon exit status %d" % p.returncode)
PY
fi

echo "# the IDL in data/ must match what the daemon serves"
# comments and indentation differ between libsystemd versions
normalize() {
   sed -e 's/^[[:space:]]*//' -e '/^#/d' -e '/^$/d'
}
"$varlinkctl" introspect "$daemon" io.github.mvo5.synaptic 2>/dev/null | normalize > "${idl##*/}.served"
normalize < "$idl" | diff -u - "${idl##*/}.served"
rm -f "${idl##*/}.served"
