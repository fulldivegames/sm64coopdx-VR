$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
function Extract-Function([string]$file, [string]$name) {
    $source = Get-Content (Join-Path $repo $file) -Raw
    $match = [regex]::Match($source, "(?m)^(?:void|struct DjuiBase\*) $name\(")
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
#include <stdlib.h>
#include <stdio.h>
struct DjuiBase;
struct DjuiBaseChild { struct DjuiBase* base; struct DjuiBaseChild* next; };
struct Interactable { bool enabled; };
struct DjuiBase {
 struct DjuiBase* parent; struct DjuiBaseChild* child;
 struct Interactable* interactable; void (*destroy)(struct DjuiBase*);
};
static bool sCursorMouseControlled=false;
static struct DjuiBase* sInputControlledBase;
static struct DjuiBase *gDjuiHovered,*gDjuiCursorDownOn,*gInteractableFocus,*gInteractableBinding,*gInteractableMouseDown;
static void djui_cursor_set_visible(bool v) { (void)v; }
'@
$fixture += "`n" + (Extract-Function 'src/pc/djui/djui_cursor.c' 'djui_cursor_input_controlled_center')
$fixture += "`n" + (Extract-Function 'src/pc/djui/djui_cursor.c' 'djui_cursor_get_input_controlled_base')
$fixture += "`n" + (Extract-Function 'src/pc/djui/djui_base.c' 'djui_base_destroy')
$fixture += @'

static void release_control(struct DjuiBase* b) {
 assert(sInputControlledBase!=b); // No dangling cursor by the time memory is freed.
 free(b);
}
static struct DjuiBase* control(void) {
 struct DjuiBase* b=calloc(1,sizeof(*b));
 b->interactable=calloc(1,sizeof(*b->interactable));
 b->interactable->enabled=true; b->destroy=release_control;
 return b;
}
int main(void) {
 for(int mouse=0;mouse<2;mouse++) {
  struct DjuiBase* b=control();
  sCursorMouseControlled=false; djui_cursor_input_controlled_center(b);
  sCursorMouseControlled=mouse; djui_cursor_input_controlled_center(NULL);
  assert(!sInputControlledBase);
  sCursorMouseControlled=false; djui_cursor_input_controlled_center(b);
  sCursorMouseControlled=mouse; djui_base_destroy(b);
  assert(!sInputControlledBase);
  // Closing a whole panel must also clear a selected nested checkbox.
  struct DjuiBase* panel=control(); b=control();
  panel->child=calloc(1,sizeof(*panel->child)); panel->child->base=b; b->parent=panel;
  sCursorMouseControlled=false; djui_cursor_input_controlled_center(b);
  sCursorMouseControlled=mouse; djui_base_destroy(panel);
  assert(!sInputControlledBase);
 }
 puts("PASS: actual cursor clearing and control/panel destruction in mouse and controller modes");
}
'@
$exe = Join-Path $repo 'build/test_menu_cursor_lifetime.exe'
$fixture | & 'C:\msys64\mingw64\bin\gcc.exe' -Wall -Wextra -Werror -x c - -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Cursor test compile failed' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Cursor lifetime test failed' }
