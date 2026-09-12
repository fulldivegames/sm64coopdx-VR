param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
 $source=Get-Content src/game/vr_hand_interaction.c -Raw
 $start=$source.IndexOf('void vr_hand_interaction_update_roomscale_body(')
 $end=$source.IndexOf('bool vr_hand_interaction_validate_headset_damage_contact(', $start)
 if($start -lt 0 -or $end -le $start){throw 'Function extraction failed'}
 [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_roomscale_shell.inc'),$source.Substring($start,$end-$start))
 & $Compiler -std=c11 -O2 tests/test_vr_roomscale_shell.c -o build/test_vr_roomscale_shell.exe
 if($LASTEXITCODE){throw 'Compile failed'}
 & ./build/test_vr_roomscale_shell.exe
 if($LASTEXITCODE){throw 'Test failed'}
}finally{Pop-Location}
