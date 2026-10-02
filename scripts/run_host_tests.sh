#!/usr/bin/env bash
# Builds and runs the hardware-independent unit tests with plain g++ + Unity.
# This is an alternative to `pio test -e native` for machines without PlatformIO.
# Usage: UNITY_DIR=/path/to/Unity scripts/run_host_tests.sh
# (Unity: git clone https://github.com/ThrowTheSwitch/Unity)
set -euo pipefail
cd "$(dirname "$0")/.."
UNITY_DIR="${UNITY_DIR:-./Unity}"
[ -f "$UNITY_DIR/src/unity.c" ] || { echo "Set UNITY_DIR to a Unity checkout"; exit 2; }
OUT=$(mktemp -d)
gcc -c -I"$UNITY_DIR/src" "$UNITY_DIR/src/unity.c" -o "$OUT/unity.o"
rc=0
for t in test/test_*/; do
  name=$(basename "$t")
  g++ -std=gnu++17 -Wall -Wextra -Werror -Isrc -I"$UNITY_DIR/src" \
      "$t/test_main.cpp" src/core/*.cpp "$OUT/unity.o" -o "$OUT/$name"
  echo "=== $name ==="
  "$OUT/$name" || rc=1
done
rm -rf "$OUT"
exit $rc
