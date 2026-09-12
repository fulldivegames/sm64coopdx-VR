param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $source=[IO.File]::ReadAllText((Join-Path $root 'src/game/mario_step.c'))
    $a=$source.IndexOf('u32 should_strengthen_gravity_for_jump_ascent(')
    $b=$source.IndexOf('void apply_gravity(', $a)
    if($a -lt 0 -or $b -le $a){throw 'Extraction failed'}
    [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_swing_ascent.inc'),$source.Substring($a,$b-$a))
    & $Compiler -std=gnu11 -O2 -DNON_MATCHING -DAVOID_UB -DTARGET_PC -DVERSION_US -I. -Iinclude -Isrc -Ilib/lua/include tests/test_vr_swing_ascent.c -o build/test_vr_swing_ascent.exe
    if($LASTEXITCODE){throw 'Compile failed'}
    & ./build/test_vr_swing_ascent.exe
    if($LASTEXITCODE){throw 'Test failed'}
} finally { Pop-Location }
