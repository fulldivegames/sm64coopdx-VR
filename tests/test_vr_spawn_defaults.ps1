param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $source=Get-Content src/game/vr_hand_interaction.c -Raw
    $start=$source.IndexOf('enum VrBoxReward vr_special_moves_roll_box_reward(')
    $end=$source.IndexOf('bool vr_special_moves_spawn_cheat_fire_flower', $start)
    $roll=$source.Substring($start,$end-$start)
    if(!$roll.Contains('vr_spawn_effective_weight(50, false)')) {throw 'Original reward weight is not on the same scale'}
    foreach($power in @('FireFlower','HammerSuit','SonicShoes','BigHands','Propeller','PowerStar')) {
        $strong=if($power -in @('BigHands','PowerStar')) {'true'} else {'false'}
        if(!$roll.Contains("vr_spawn_effective_weight(configVrSpawnWeight$power, $strong)")) {throw "Incorrect category: $power"}
        if(!$roll.Contains("if (configVrSpawnPool$power)")) {throw "Missing disable guard: $power"}
    }
    if($roll.Contains('configVrBigHandsLongTimer')) {throw 'Timer extension must not change rarity category'}
    & $Compiler -std=gnu11 -O2 tests/test_vr_spawn_weights.c -o build/test_vr_spawn_weights.exe
    if($LASTEXITCODE){throw 'Spawn test compile failed'}
    & ./build/test_vr_spawn_weights.exe
    if($LASTEXITCODE){throw 'Spawn test failed'}
    $config=Get-Content src/pc/configfile.c -Raw
    $menu=Get-Content src/pc/djui/djui_panel_vr.c -Raw
    $spawnMenu=$menu.Substring($menu.IndexOf('static void djui_panel_vr_spawn_pool_create('))
    $specialIndex=$spawnMenu.IndexOf('"Specials - Half Spawn Weight"')
    $bigIndex=$spawnMenu.IndexOf('"Big Hands"')
    $starIndex=$spawnMenu.IndexOf('"Power Star"')
    $gesturesIndex=$spawnMenu.IndexOf('"Gestures"')
    if(!($specialIndex -ge 0 -and $specialIndex -lt $bigIndex -and $bigIndex -lt $starIndex -and $starIndex -lt $gesturesIndex)) {
        throw 'Specials grouping/order regressed'
    }
    foreach($gesture in @('Jumping','Swimming')) {
        if($config -notmatch "bool\s+configVrPhysical$gesture\s*=\s*true;") {throw "Wrong fresh default: $gesture"}
        if($menu -notmatch "configVrPhysical$gesture\s*=\s*true;") {throw "Wrong reset default: $gesture"}
        if($config -notmatch "\.boolValue = &configVrPhysical$gesture") {throw "Saved preference not registered: $gesture"}
    }
    foreach($file in @('README.md','docs/PLAYER-GUIDE.txt','docs/PC-VR-PLAYER-GUIDE.txt','src/pc/djui/djui_panel_vr.c')) {
        if((Get-Content $file -Raw) -match 'Physical (Jumping|Swimming).{0,15}(is |are |\()?off by default') {throw "Stale tutorial default: $file"}
    }
    $packager=Get-Content tools/package-vr-windows.ps1 -Raw
    foreach($guide in @('PHYSICAL-JUMPING.txt','PHYSICAL-SWIMMING.txt','POWER-UP-SPAWN-WEIGHTS.txt','PROPELLER-MUSHROOM.txt')) {
        if(!$packager.Contains("docs/$guide")) {throw "Guide omitted from PC package: $guide"}
    }
    Write-Output 'PASS: production reward categories, disable guards, timer independence, fresh/reset defaults and saved preference registration'
} finally { Pop-Location }
