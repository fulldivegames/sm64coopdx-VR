param([string]$Compiler = 'C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$source = Get-Content (Join-Path $root 'src/pc/djui/djui_flow_layout.c') -Raw
function Extract-Function([string]$name) {
    $match = [regex]::Match($source, "(?m)^(?:static )?(?:struct DjuiBase\*|bool) $name\(")
    if (!$match.Success) { throw "Missing $name" }
    $open = $source.IndexOf('{', $match.Index)
    $depth = 1; $end = $open + 1
    while ($depth -gt 0) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
        $end++
    }
    $source.Substring($match.Index, $end - $match.Index)
}
$fixture = @'
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
typedef signed char s8;
struct DjuiBase;
struct DjuiBaseChild { struct DjuiBase* base; struct DjuiBaseChild* next; };
struct Interactable { bool enabled; };
struct DjuiBase {
    bool visible; struct Interactable* interactable;
    struct DjuiBase* parent; struct DjuiBaseChild* child;
    bool (*render)(struct DjuiBase*);
};
struct DjuiFlowLayout { struct DjuiBase base; bool orderedNavigation; };
static bool djui_flow_layout_render(struct DjuiBase* base) { return base != NULL; }
'@
$fixture += "`n" + (Extract-Function 'djui_flow_first_control')
$fixture += "`n" + (Extract-Function 'djui_flow_layout_navigate')
$config = Get-Content (Join-Path $root 'src/pc/configfile.c') -Raw
$migration = [regex]::Match($config, '(?s)if \(configVrSpawnWeightScale == 0\) \{.*?configVrSpawnWeightScale = 1;\s+\}')
if (!$migration.Success) { throw 'Missing spawn migration' }
$fixture += @'

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
static unsigned configVrSpawnWeightFireFlower, configVrSpawnWeightHammerSuit;
static unsigned configVrSpawnWeightSonicShoes, configVrSpawnWeightBigHands, configVrSpawnWeightScale;
static void migrate(unsigned suppliedSpawnWeights) {
'@
$fixture += $migration.Value + "`n}`n"
$fixture += @'

int main(void) {
    configVrSpawnWeightFireFlower=configVrSpawnWeightHammerSuit=5;
    configVrSpawnWeightSonicShoes=configVrSpawnWeightBigHands=5;
    migrate(15);
    assert(configVrSpawnWeightFireFlower==50 && configVrSpawnWeightHammerSuit==50);
    assert(configVrSpawnWeightSonicShoes==50 && configVrSpawnWeightBigHands==50);
    migrate(15); assert(configVrSpawnWeightBigHands==50); // Only once.
    configVrSpawnWeightScale=0; migrate(0); assert(configVrSpawnWeightBigHands==50); // No old keys.
    configVrSpawnWeightScale=0; configVrSpawnWeightBigHands=1;
    migrate(8); assert(configVrSpawnWeightBigHands==10 && configVrSpawnWeightFireFlower==50);
    configVrSpawnWeightScale=0; configVrSpawnWeightBigHands=~0U;
    migrate(8); assert(configVrSpawnWeightBigHands==100);
    struct DjuiFlowLayout list = { .base = {.visible=true, .render=djui_flow_layout_render}, .orderedNavigation=true };
    struct DjuiBase rows[17]={0}, inputs[17]={0};
    struct DjuiBaseChild children[17]={0}, nested[17]={0};
    struct Interactable controls[17]={0};
    list.base.child=children;
    for (int i=0; i<17; ++i) {
        controls[i].enabled=true;
        rows[i]=(struct DjuiBase){.visible=true, .parent=&list.base, .child=&nested[i]};
        inputs[i]=(struct DjuiBase){.visible=true, .parent=&rows[i], .interactable=&controls[i]};
        children[i]=(struct DjuiBaseChild){&rows[i], i<16 ? &children[i+1] : NULL};
        nested[i]=(struct DjuiBaseChild){&inputs[i], NULL};
    }
    struct DjuiBase* pick=NULL;
    // Off-screen input geometry is intentionally absent. Order must suffice.
    for (int pass=0; pass<8; ++pass) {
        for (int i=0; i<16; ++i) {
            pick=NULL; assert(djui_flow_layout_navigate(&inputs[i],1,&pick)); assert(pick==&inputs[i+1]);
        }
        for (int i=16; i>0; --i) {
            pick=NULL; assert(djui_flow_layout_navigate(&inputs[i],-1,&pick)); assert(pick==&inputs[i-1]);
        }
    }
    pick=NULL; assert(djui_flow_layout_navigate(&inputs[0],-1,&pick) && !pick);
    pick=NULL; assert(djui_flow_layout_navigate(&inputs[16],1,&pick) && !pick);
    controls[1].enabled=false; rows[2].visible=false;
    pick=NULL; assert(djui_flow_layout_navigate(&inputs[0],1,&pick) && pick==&inputs[3]);
    pick=NULL; assert(djui_flow_layout_navigate(&inputs[3],-1,&pick) && pick==&inputs[0]);
    list.orderedNavigation=false;
    assert(!djui_flow_layout_navigate(&inputs[3],-1,&pick));
    puts("PASS: real navigation functions, 17 nested rows, repeated up/down, bounds, hidden/disabled rows, opt-in isolation; actual one-time spawn migration");
}
'@
$exe = Join-Path $root 'build/test_menu_navigation.exe'
$fixture | & $Compiler -x c - -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Navigation test compile failed' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Navigation test failed' }
