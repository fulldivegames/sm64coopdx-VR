param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $source=[IO.File]::ReadAllText((Join-Path $root 'src/game/vr_propeller.inc.h'))
    # Exercise the real timer/action/music/pickup-allocation code. Only pickup
    # collision queries are stubbed; the separate included helper is engine-built.
    $source=$source.Replace('#include "vr_propeller_pickup.inc.h"','static void vr_special_moves_update_propeller_pickup(struct MarioState* m) { (void)m; }')
    [IO.File]::WriteAllText((Join-Path $root 'build/test_vr_propeller_state.inc'),$source)
    & $Compiler -std=gnu11 -O2 tests/test_vr_propeller_state.c -o build/test_vr_propeller_state.exe
    if($LASTEXITCODE){throw 'Compile failed'}
    & ./build/test_vr_propeller_state.exe
    if($LASTEXITCODE){throw 'Test failed'}
    $gravity=Get-Content src/game/mario_step.c -Raw
    if(!$gravity.Contains('if (stepResult == AIR_STEP_LANDED && m->floor)') -or
       !$gravity.Contains('if (stepResultOverride == AIR_STEP_LANDED && m->floor)')) {throw 'Native/mod landing recharge hook missing'}
    if(!$gravity.Contains('m->action == ACT_TWIRLING && (m->actionArg & VR_PROPELLER_ACTION_ARG)')) {throw 'Propeller gravity lost its action marker guard'}
    $visual=Get-Content src/game/mario.c -Raw
    if(!$visual.Contains('(action & ACT_GROUP_MASK) == ACT_GROUP_SUBMERGED || action == ACT_LAVA_BOOST')) {throw 'Water/lava transition recharge missing'}
    if(!$visual.Contains('fastDescent = propeller && (m->input & INPUT_Z_DOWN) != 0')) {throw 'Ordinary twirl affected by crouch'}
    Write-Output 'PASS: regular twirl retains separate gravity and visual controls'
    $interactions=Get-Content src/game/interaction.c -Raw
    if(([regex]::Matches($interactions,'const bool propellerStomp = vr_propeller_enemy_stomp')).Count -ne 2) {throw 'Unexpected stomp hook count'}
    foreach($name in @('interact_hit_from_below','interact_bounce_top')) {
        $body=[regex]::Match($interactions,"(?s)u32 $name\(.*?(?=\r?\nu32 |\z)").Value
        if(!$body.Contains('const bool propellerStomp = vr_propeller_enemy_stomp')) {throw "Missing enemy stomp hook: $name"}
    }
    $renderer=Get-Content src/game/rendering_graph_node.c -Raw
    if(!$renderer.Contains('vr_append_hammer_back_shell')) {throw 'Hammer body attachment missing'}
    $accessories=Get-Content src/game/vr_body_accessories.inc.h -Raw
    if($accessories.Contains('gCurGraphNodeMarioState ==')) {throw 'Native Mario must not require the optional player-render flag'}
    foreach($name in @('body_accessories','propeller_stomp')) {
        & $Compiler -std=gnu11 -O2 "tests/test_vr_$name.c" -o "build/test_vr_$name.exe"
        if($LASTEXITCODE){throw "Compile failed: $name"}
        & "./build/test_vr_$name.exe"
        if($LASTEXITCODE){throw "Test failed: $name"}
    }
} finally { Pop-Location }
