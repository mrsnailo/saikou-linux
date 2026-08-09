#!/usr/bin/env bash
# Starts saikou-core on a throwaway socket and checks that the RPC transport answers.
# Runs in CI and is the fastest way to tell whether the daemon still boots locally.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
core="$root/saikou-core/build/install/saikou-core/bin/saikou-core"

if [[ ! -x "$core" ]]; then
    echo "core not installed; run ./gradlew :saikou-core:installDist first" >&2
    exit 1
fi

workdir="$(mktemp -d)"
socket="$workdir/core.sock"
trap 'kill "${core_pid:-}" 2>/dev/null || true; rm -rf "$workdir"' EXIT

"$core" --socket "$socket" --debug &
core_pid=$!

for _ in $(seq 1 50); do
    [[ -S "$socket" ]] && break
    sleep 0.2
done

if [[ ! -S "$socket" ]]; then
    echo "FAIL: core never bound $socket" >&2
    exit 1
fi

response="$(printf '{"jsonrpc":"2.0","id":1,"method":"core.ping"}\n' | timeout 10 nc -U -q 1 "$socket" | head -1)"

if [[ "$response" != *'"result":"pong"'* ]]; then
    echo "FAIL: unexpected ping response: $response" >&2
    exit 1
fi

echo "OK: $response"
