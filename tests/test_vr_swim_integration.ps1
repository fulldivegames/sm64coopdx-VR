param([string]$Compiler = 'C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    # Generate a test translation unit from the actual game implementation;
    # exclude unrelated vanilla actions rather than mock hundreds of symbols.
    $source=[IO.File]::ReadAllText((Join-Path $root 'src/game/mario_actions_submerged.c'))
    $end=$source.IndexOf('#define MIN_SWIM_STRENGTH')
    if ($end -lt 0 -or !$source.Substring(0,$end).Contains('static void vr_physical_swim_step')) { throw 'Swim source extraction failed' }
    [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_swim_update.inc'),$source.Substring(0,$end))
    & $Compiler -std=gnu11 -O2 -DNON_MATCHING -DAVOID_UB -DTARGET_PC -DVERSION_US -I. -Iinclude -Isrc -Isrc/game -Ilib/lua/include -Ibuild/us_pc tests/test_vr_swim_integration.c src/engine/math_util.c -o build/test_vr_swim_integration.exe
    if ($LASTEXITCODE) { throw 'Compile failed' }
    & ./build/test_vr_swim_integration.exe
    if ($LASTEXITCODE) { throw 'Swimming integration test failed' }
} finally { Pop-Location }
