param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $source=[IO.File]::ReadAllText((Join-Path $root 'src/game/rendering_graph_node.c'))
    $function=[regex]::Match($source,'(?ms)^void vr_rebase_first_person_climb_anchor\(.*?^\}').Value
    if(!$function){throw 'Extraction failed'}
    [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_climb_handoff.inc'),$function)
    & $Compiler -std=gnu11 -O2 tests/test_vr_climb_handoff.c -o build/test_vr_climb_handoff.exe
    if($LASTEXITCODE){throw 'Compile failed'}
    & ./build/test_vr_climb_handoff.exe
    if($LASTEXITCODE){throw 'Test failed'}
} finally { Pop-Location }
