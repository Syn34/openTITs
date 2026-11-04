#!/usr/bin/env bash
# A/B check: run the game with and without the decompiled functions and
# compare the game's data/bss at the end of the run.
#   tools/ab.sh <dump> [game exe] [-- game switches...]
# Prints the differing addresses. The VBlank counter (0x010F0E00) is ignored:
# it depends on timing. AB_COVERAGE=<file> keeps the list of decompiled
# functions that ran. AB_PAGE_NOISE=<file> ignores heap pages listed there
# (make it with AB_PAGES_OUT=<file> on two runs of the same build).
set -uo pipefail
dump="${1:?usage: tools/ab.sh <dump> [game exe] [-- game args]}"
shift
exe="./build/game/opentit"
if [[ $# -gt 0 && "$1" != "--" ]]; then exe="$1"; shift; fi
game_args=()
if [[ $# -gt 0 && "$1" == "--" ]]; then shift; game_args=(-- "$@"); fi
timeout_s="${AB_TIMEOUT:-120}"

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
run() {  # <name> [extra switches]
  local name="$1"; shift
  (cd "$work" && rm -rf dev_hdd0 dev_hdd1 &&
   timeout "$timeout_s" "$exe" "$dump" --headless --dump-mem "$work/$name.bin" "$@" "${game_args[@]}" \
     > "$work/$name.log" 2>&1)
  echo "$name: exit $?, $(grep -m1 -o 'openTITs crash: [A-Z]*' "$work/$name.log" || echo 'no crash')"
}
exe="$(cd "$(dirname "$exe")" && pwd)/$(basename "$exe")"
run with --decomp-coverage "$work/coverage.txt"
run without --no-decomp

if [[ ! -f "$work/with.bin" || ! -f "$work/without.bin" ]]; then
  echo "a run produced no memory dump (see logs)"; exit 2
fi
diffs="$(cmp -l "$work/with.bin" "$work/without.bin" | awk '
function oct(s,  v, i) { v = 0; for (i = 1; i <= length(s); i++) v = v * 8 + substr(s, i, 1); return v }
{
  ea = $1 - 1 + 15794176   # data/bss starts at 0x00F10000
  if (ea >= 17763840 && ea < 17763844) next   # 0x010F0E00 VBlank counter
  printf "0x%08X  with %3d  without %3d\n", ea, oct($2), oct($3)
}')"
if [[ -f "$work/coverage.txt" ]]; then
  echo "decompiled functions that ran: $(wc -l < "$work/coverage.txt")"
  [[ -n "${AB_COVERAGE:-}" ]] && cp "$work/coverage.txt" "$AB_COVERAGE"
fi
# Heap pages (user memory) and RSX local memory, compared by hash. NOISE lists
# pages that differ between two runs of the same code (see AB_PAGE_NOISE).
if [[ -f "$work/with.bin.pages" && -f "$work/without.bin.pages" ]]; then
  pages="$(diff "$work/with.bin.pages" "$work/without.bin.pages" | awk '/^</ {print $2}')"
  if [[ -n "${AB_PAGE_NOISE:-}" && -f "$AB_PAGE_NOISE" ]]; then pages="$(grep -vxF -f "$AB_PAGE_NOISE" <<< "$pages" || true)"; fi
  if [[ -n "$pages" ]]; then
    echo "$(wc -l <<< "$pages") heap/RSX pages (64 KiB) differ: $(head -8 <<< "$pages" | tr '\n' ' ')"
    [[ -n "${AB_PAGES_OUT:-}" ]] && echo "$pages" > "$AB_PAGES_OUT"
    page_diff=1
  fi
fi
if [[ -z "$diffs" && -z "${page_diff:-}" ]]; then
  echo "identical"
else
  [[ -n "$diffs" ]] && { echo "$(wc -l <<< "$diffs") data/bss bytes differ:"; head -50 <<< "$diffs"; }
  exit 1
fi
