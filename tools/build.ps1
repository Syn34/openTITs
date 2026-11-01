# Build the tools, recompile your EBOOT with the decompiled functions in
# src\ left out, and build the game with them linked in (Windows / PowerShell).
#
#   .\tools\build.ps1 -Dump "D:\Dumps\Sly 4"
#
# Requires: Visual Studio 2022 (Desktop C++), CMake, Ninja on PATH, and the
# Vulkan SDK (https://vulkan.lunarg.com - Vulkan + glslang for the renderer).
# SDL2 is downloaded automatically if it is not installed.
# Run from a "Developer PowerShell for VS 2022" so cl.exe is available.
param(
    [Parameter(Mandatory = $true)][string]$Dump,
    [string]$Gen = "gen",
    [string]$Config = "Release"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $eboot = Join-Path $Dump "PS3_GAME\USRDIR\EBOOT.elf"
    if (-not (Test-Path $eboot)) {
        throw "Decrypted executable not found: $eboot`nDecrypt EBOOT.BIN with RPCS3 (Utilities > Decrypt PS3 Binaries) and save it as EBOOT.elf."
    }

    if (-not $env:VULKAN_SDK) {
        Write-Warning "VULKAN_SDK is not set: the game will build without its renderer (nothing will be drawn). Install the Vulkan SDK and reopen the shell."
    }
    cmake -S . -B build\tools -G Ninja -DCMAKE_BUILD_TYPE=$Config
    cmake --build build\tools
    .\build\tools\opentit_tests.exe
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
    cmake -S . -B build\game -G Ninja -DCMAKE_BUILD_TYPE=$Config -DOPENTIT_BUILD_TESTS=OFF "-DOPENTIT_GENERATED_DIR=$genAbs"
    cmake --build build\game

    Write-Host "`nDone. Run:  .\build\game\opentit.exe `"$Dump`" --log info"
    Write-Host "A/B:       .\build\game\opentit.exe `"$Dump`" --no-decomp"
}
finally {
    Pop-Location
}
