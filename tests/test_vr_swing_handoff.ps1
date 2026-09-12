param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $source=[IO.File]::ReadAllText((Join-Path $root 'src/game/vr_hand_interaction.c'))
    $a=$source.IndexOf('static void vr_hand_interaction_update_physical_climb_handoff(')
    $b=$source.IndexOf('static void vr_hand_interaction_maintain_physical_climb(', $a)
    if($a -lt 0 -or $b -le $a){throw 'Extraction failed'}
    [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_swing_handoff.inc'),$source.Substring($a,$b-$a))
    & $Compiler -std=gnu11 -O2 tests/test_vr_swing_handoff.c -o build/test_vr_swing_handoff.exe
    if($LASTEXITCODE){throw 'Compile failed'}
    & ./build/test_vr_swing_handoff.exe
    if($LASTEXITCODE){throw 'Test failed'}
} finally { Pop-Location }
