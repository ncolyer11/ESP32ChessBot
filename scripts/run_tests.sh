#!/bin/sh
# Runs every tests/*.in through the engine in --test mode and diffs against tests/*.out
cd "$(dirname "$0")/.." || exit 1
BIN=bin/chesp32
pass=0
fail=0
tmp=$(mktemp)
for in_file in tests/*.in; do
    name=$(basename "$in_file" .in)
    printf 'Testing %s... ' "$name"
    "$BIN" --test < "$in_file" > "$tmp"
    if diff -u "tests/$name.out" "$tmp" > /dev/null; then
        echo PASSED
        pass=$((pass + 1))
    else
        echo FAILED
        diff -u "tests/$name.out" "$tmp" | head -40
        fail=$((fail + 1))
    fi
done
rm -f "$tmp"
echo "========================="
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
