#!/usr/bin/env bash
# Build the tools, recompile your EBOOT with the decompiled functions in
# src/ left out, and build the game with them linked in (Linux / macOS).
#   tools/build.sh "/path/to/Sly 4 dump" [gen dir]
set -euo pipefail
dump="${1:?usage: tools/build.sh <path to disc dump> [gen dir]}"
cd "$(dirname "$0")/.."
root="$PWD"
gen="${2:-$root/gen}"
eboot="$dump/PS3_GAME/USRDIR/EBOOT.elf"
if [[ ! -f "$eboot" ]]; then
  echo "Decrypted executable not found: $eboot" >&2
  echo "Decrypt EBOOT.BIN with RPCS3 (Utilities > Decrypt PS3 Binaries) and save it as EBOOT.elf." >&2
  exit 1
fi

cmake -S . -B build/tools -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/tools
./build/tools/opentit_tests
recomp=build/tools/opentit-recomp

"$recomp" recompile "$eboot" -o "$gen" --decomp src --symbols symbols.txt

# Optional LLE firmware modules (decrypted with RPCS3), e.g. firmware/libsre.prx
base=$((0x01800000))
for prx in "$dump"/firmware/*.prx "$dump"/*.prx; do
  [[ -f "$prx" ]] || continue
  name="$(basename "$prx" .prx)"
  "$recomp" recompile "$prx" -o "$gen/modules/$name" --base "$(printf '%X' $base)" --name "$name"
  base=$((base + 0x00400000))
done

# SPU code: images in the EBOOT and firmware, plus anything recorded with --spu-record.
spu_inputs=("$eboot")
for f in "$dump"/firmware/*.prx "$dump"/*.prx "$dump"/*.spurec; do [[ -f "$f" ]] && spu_inputs+=("$f"); done
"$recomp" spu-recompile "${spu_inputs[@]}" -o "$gen/spu"

cmake -S . -B build/game -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPENTIT_BUILD_TESTS=OFF \
      -DOPENTIT_GENERATED_DIR="$(cd "$gen" && pwd)"
cmake --build build/game
echo
echo "Done. Run:  ./build/game/opentit \"$dump\" --log info"
echo "A/B:       ./build/game/opentit \"$dump\" --no-decomp"
