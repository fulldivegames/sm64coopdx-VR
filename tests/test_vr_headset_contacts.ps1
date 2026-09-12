param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $hand=[IO.File]::ReadAllText((Join-Path $root 'src/game/vr_hand_interaction.c'))
    $collision=[IO.File]::ReadAllText((Join-Path $root 'src/game/object_collision.c'))
    $start=$hand.IndexOf('static void vr_hand_interaction_apply_headset_collider(')
    $end=$hand.IndexOf('static void vr_hand_interaction_clear_tracked_hold(', $start)
    $cs=$collision.IndexOf('s32 detect_object_hitbox_overlap(')
    $ce=$collision.IndexOf('s32 detect_object_hurtbox_overlap(', $cs)
    $radius=[regex]::Match($hand,'(?m)^#define VR_HEADSET_INTERACTION_RADIUS .+$').Value
    $height=[regex]::Match($hand,'(?m)^#define VR_HEADSET_INTERACTION_HEIGHT .+$').Value
    if (!$radius -or !$height -or $start -lt 0 -or $end -le $start) { throw 'Extraction failed' }
    [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_headset_contacts.inc'), "$radius`n$height`n"+$hand.Substring($start,$end-$start)+$collision.Substring($cs,$ce-$cs))
    & $Compiler -std=gnu11 -O2 -DNON_MATCHING -DAVOID_UB -DTARGET_PC -DVERSION_US -I. -Iinclude -Isrc -Ilib/lua/include -Ibuild/us_pc tests/test_vr_headset_contacts.c -o build/test_vr_headset_contacts.exe
    if ($LASTEXITCODE) { throw 'Compile failed' }
    & ./build/test_vr_headset_contacts.exe
    if ($LASTEXITCODE) { throw 'Contacts failed' }
    $order=[IO.File]::ReadAllText((Join-Path $root 'src/game/object_list_processor.c'))
    if ($order.IndexOf('vr_hand_interaction_update_headset_collider(gMarioState);') -gt $order.IndexOf('detect_object_collisions();')) { throw 'Collider updated too late' }
} finally { Pop-Location }
