#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary="${TMPDIR:-/tmp}/infiltrasense-policy-test"

c++ -std=c++17 -Wall -Wextra -Werror \
  -I"$project_dir/tests/stubs" \
  "$project_dir/tests/policy_test.cpp" \
  -o "$test_binary"

"$test_binary"
