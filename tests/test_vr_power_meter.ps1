param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $source=[IO.File]::ReadAllText((Join-Path $root 'src/game/hud.c'))
    $a=$source.IndexOf('void patch_hud_interpolated(')
    $b=$source.IndexOf('/**', $a)
    $c=$source.IndexOf('void render_dl_power_meter(')
    $d=$source.IndexOf('/**', $c)
    if($a -lt 0 -or $b -le $a -or $c -lt 0 -or $d -le $c){throw 'Extraction failed'}
    [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_power_meter.inc'),$source.Substring($a,$b-$a)+$source.Substring($c,$d-$c))
    & $Compiler -std=gnu11 -O2 tests/test_vr_power_meter.c -o build/test_vr_power_meter.exe
    if($LASTEXITCODE){throw 'Compile failed'}
    & ./build/test_vr_power_meter.exe
    if($LASTEXITCODE){throw 'Test failed'}
} finally { Pop-Location }
