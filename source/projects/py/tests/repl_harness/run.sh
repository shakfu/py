#!/bin/sh
# Build and run the py_repl harness: py.c's repl section against embedded CPython.
# Usage: run.sh [work dir]. Exits 77 (ctest skip) without python3-config.
set -e
here=$(cd "$(dirname "$0")" && pwd)
py=$(cd "$here/../.." && pwd)
work=${1:-${TMPDIR:-/tmp}/py_repl_harness}
command -v python3-config >/dev/null 2>&1 || { echo "python3-config not found"; exit 77; }
mkdir -p "$work"
awk '/^\/\* py_repl interface \(py_repl_abi\.h/ {f=1} /^\/\* end py_repl interface/ {f=0} f' \
    "$py/py.c" > "$work/repl_section.c"
test -s "$work/repl_section.c" || { echo "repl section markers not found in py.c"; exit 1; }
cc -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -I"$py" -I"$work" \
    $(python3-config --includes) "$here/harness.c" -o "$work/harness" \
    $(python3-config --ldflags --embed)
"$work/harness"
