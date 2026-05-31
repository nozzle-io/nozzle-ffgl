#!/usr/bin/env bash
set -euo pipefail

grep -F '"NZR1"' src/receive/nozzle_receive.cpp
grep -F '"NZS1"' src/send/nozzle_send.cpp
grep -F '| NozzleReceive | Source | `NZR1` |' docs/plugin-ids.md
grep -F '| NozzleSend | Effect | `NZS1` |' docs/plugin-ids.md

if grep -R -n -E '"(RS|RE|RM)[0-9]{2}"' src; then
  echo 'copied FFGL example plugin ID found in src' >&2
  exit 1
fi
