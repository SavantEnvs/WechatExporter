#!/usr/bin/env bash
#
# mayhem/test.sh — run the known-answer self-test of WechatExporter's core (built by build.sh with
# normal flags). Upstream ships no Linux-runnable tests; these checks assert exact outputs of the
# template engine, the XML/XPath wrapper, the string helpers and the Manifest.db index loader.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

EXPECTED=12
RUNNER="$SRC/mayhem/build/core_selftest"
if [ ! -x "$RUNNER" ]; then
  echo "core_selftest missing — run mayhem/build.sh first" >&2
  emit_ctrf core_selftest 0 "$EXPECTED"; exit 1
fi
out="$("$RUNNER" 2>&1)"; rc=$?
echo "$out"
passed=$(grep -c '^PASS ' <<<"$out" || true)
failed=$(grep -c '^FAIL ' <<<"$out" || true)
# Anything that did not report PASS (crash, early exit, no output) counts as failed.
missing=$(( EXPECTED - passed - failed )); [ "$missing" -gt 0 ] && failed=$(( failed + missing ))
[ "$rc" -ne 0 ] && [ "$failed" -eq 0 ] && failed=1
emit_ctrf core_selftest "$passed" "$failed"
