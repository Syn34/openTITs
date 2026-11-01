# Build openTIT's tools, recompile your EBOOT with the decompiled functions in
# src\ left out, and build the game with them linked in (Windows / PowerShell).
#
#   .\tools\build.ps1 -Dump "D:\Dumps\Sly 4"
#
# Requires what openTIT does: Visual Studio 2022 (Desktop C++), CMake, Ninja on
# PATH and the Vulkan SDK. Run from a "Developer PowerShell for VS 2022".
param(
    [Parameter(Mandatory = $true)][string]$Dump,
    [string]$Gen = "gen",
    [string]$Config = "Release",
    [string]$Engine = ""
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    if (-not $Engine) { $Engine = if ($env:OPENTIT_DIR) { $env:OPENTIT_DIR } else { Join-Path $root "openTIT" } }
    if (-not (Test-Path (Join-Path $Engine "CMakeLists.txt"))) {
        throw "openTIT not found at $Engine. Run: git submodule update --init"
    }
    $eboot = Join-Path $Dump "PS3_GAME\USRDIR\EBOOT.elf"
    if (-not (Test-Path $eboot)) {
        throw "Decrypted executable not found: $eboot`nDecrypt EBOOT.BIN with RPCS3 (Utilities > Decrypt PS3 Binaries) and save it as EBOOT.elf."
    }

    cmake -S $Engine -B build\tools -G Ninja -DCMAKE_BUILD_TYPE=$Config
    cmake --build build\tools
    $recomp = ".\build\tools\opentit-recomp.exe"

    & $recomp recompile $eboot -o $Gen --decomp src --symbols symbols.txt

    # Optional LLE firmware modules (decrypted with RPCS3), e.g. firmware\libsre.prx
    $base = 0x01800000
    $prxs = @(Get-ChildItem (Join-Path $Dump "firmware") -Filter *.prx -ErrorAction SilentlyContinue) +
            @(Get-ChildItem $Dump -Filter *.prx -ErrorAction SilentlyContinue)
    foreach ($prx in $prxs | Sort-Object Name -Unique) {
        $name = $prx.BaseName
        & $recomp recompile $prx.FullName -o (Join-Path $Gen "modules\$name") --base ('{0:X}' -f $base) --name $name
        $base += 0x00400000
    }

    # SPU code: images in the EBOOT and firmware, plus anything recorded with --spu-record.
    $spuInputs = @($eboot) + @($prxs | ForEach-Object { $_.FullName }) +
                 @(Get-ChildItem $Dump -Filter *.spurec -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
    & $recomp spu-recompile @spuInputs -o (Join-Path $Gen "spu")

    $genAbs = (Resolve-Path $Gen).Path
    $srcAbs = (Resolve-Path "src").Path
    cmake -S $Engine -B build\game -G Ninja -DCMAKE_BUILD_TYPE=$Config -DOPENTIT_BUILD_TESTS=OFF `
          "-DOPENTIT_GENERATED_DIR=$genAbs" "-DOPENTIT_DECOMP_DIR=$srcAbs"
    cmake --build build\game

    Write-Host "`nDone. Run:  .\build\game\opentit.exe `"$Dump`" --log info"
    Write-Host "A/B:       .\build\game\opentit.exe `"$Dump`" --no-decomp"
}
finally {
    Pop-Location
}
