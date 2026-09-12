$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $source=[IO.File]::ReadAllText((Join-Path $root 'src/game/rendering_graph_node.c'))
    $a=$source.IndexOf('static f32 *vr_camera_mario_anchor(')
    $b=$source.IndexOf('static bool vr_move_world_sample_to_current_gameplay_anchor(', $a)
    if($a -lt 0 -or $b -le $a){throw 'Extraction failed'}
    $c=$source.IndexOf('void vr_refresh_enemy_camera_anchor(void) {')
    $d=$source.IndexOf("`n}`n",$c)
    if($d -lt 0){$d=$source.IndexOf("`n}`r`n",$c)}
    if($c -lt 0 -or $d -le $c){throw 'Refresh extraction failed'}
    [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_enemy_camera.inc'),$source.Substring($a,$b-$a)+$source.Substring($c,$d-$c+3))
    & 'C:\msys64\mingw64\bin\gcc.exe' -std=gnu11 -O2 tests/test_vr_enemy_camera.c -o build/test_vr_enemy_camera.exe
    if($LASTEXITCODE){throw 'Compile failed'}
    & ./build/test_vr_enemy_camera.exe
    if($LASTEXITCODE){throw 'Test failed'}
} finally { Pop-Location }
