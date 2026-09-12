param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $renderer=Get-Content src/game/rendering_graph_node.c -Raw
    $selection=[regex]::Match($renderer,'(?s)    if \(displayList == \(void\*\)mario_torso.*?\n    \}').Value
    if(!$selection){throw 'Missing per-draw outfit selection'}
    $fixture=@'
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
enum {MARIO_ANIM_PART_TORSO=3};
static int mario_torso[1],mario_propeller_torso[1],other[1];
static bool active=true,powered=true;
static bool vr_is_active(void){return active;}
static bool vr_special_moves_propeller_active(void){return powered;}
static struct {void* marioObj;} gMarioStates[1];
static struct {int currAnimPart;} body={3},*gCurMarioBodyState=&body;
static void *gCurGraphNodeProcessingObject,*gCurGraphNodeHeldObject;
static void* select_torso(void* displayList){
SELECTION
return displayList;
}
int main(void){
 gMarioStates[0].marioObj=other;gCurGraphNodeProcessingObject=other;
 assert(select_torso(mario_torso)==mario_propeller_torso);
 powered=false;assert(select_torso(mario_torso)==mario_torso);powered=true;
 active=false;assert(select_torso(mario_torso)==mario_torso);active=true;
 assert(select_torso(other)==other);
 gCurGraphNodeProcessingObject=NULL;assert(select_torso(mario_torso)==mario_torso);
 gCurGraphNodeProcessingObject=other;gCurGraphNodeHeldObject=other;
 assert(select_torso(mario_torso)==mario_torso);gCurGraphNodeHeldObject=NULL;
 body.currAnimPart=4;assert(select_torso(mario_torso)==mario_torso);
}
'@
    [IO.File]::WriteAllText((Join-Path $root 'build/test_propeller_torso.c'),$fixture.Replace('SELECTION',$selection))
    & $Compiler -std=c11 build/test_propeller_torso.c -o build/test_propeller_torso.exe
    if($LASTEXITCODE){throw 'Compile failed'}
    & ./build/test_propeller_torso.exe
    if($LASTEXITCODE){throw 'Selection regression'}
    $model=Get-Content actors/mario/model.inc.c -Raw
    $stock=[regex]::Match($model,'(?s)const Gfx mario_torso\[\] = \{.*?\n\};').Value
    $prop=[regex]::Match($model,'(?s)const Gfx mario_propeller_torso\[\] = \{.*?\n\};').Value
    if(!$stock.Contains('mario_texture_yellow_button')){throw 'Stock buttons modified'}
    if($prop.Contains('G_ON') -or $prop.Contains('SetTextureImage')){throw 'Button texture still enabled'}
    if(!$prop.Contains('gsSPDisplayList(mario_yellow_button_dl)')){throw 'Button-region surface missing'}
    $prop=[regex]::Match($model,'(?s)const Gfx mario_propeller_zipper_dl\[\] = \{.*?\n\};').Value
    if(([regex]::Matches($prop,'gsSP2Triangles')).Count -ne 6){throw 'Missing separate zipper triangles'}
    $verts=[regex]::Match($model,'(?s)mario_propeller_zipper_vtx\[\] = \{.*?\n\};').Value
    $points=@([regex]::Matches($verts,'\{\{\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)') | ForEach-Object { ,@([int]$_.Groups[1].Value,[int]$_.Groups[2].Value,[int]$_.Groups[3].Value) })
    if($points.Count -ne 14){throw 'Unexpected stripe geometry'}
    foreach($tri in [regex]::Matches($prop,'gsSP2Triangles\((\d+),(\d+),(\d+),0, (\d+),(\d+),(\d+),0\)')) {
        foreach($start in @(1,4)) {
            $a=$points[[int]$tri.Groups[$start].Value];$b=$points[[int]$tri.Groups[$start+1].Value];$c=$points[[int]$tri.Groups[$start+2].Value]
            $normalY=($b[2]-$a[2])*($c[0]-$a[0])-($b[0]-$a[0])*($c[2]-$a[2])
            if($normalY -le 0){throw 'Stripe faces inward and would be culled'}
        }
    }
    'PASS: active-only torso selection, automatic restore, remote/held/custom isolation, solid button regions and outward stripe winding'
} finally {Pop-Location}
