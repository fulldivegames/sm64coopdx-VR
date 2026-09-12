$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$source = Get-Content (Join-Path $repo 'src/game/characters.c') -Raw
$start = $source.IndexOf('static void play_character_sound_internal(')
$end = $source.IndexOf('void play_character_sound(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Character sound function not found' }
$function = $source.Substring($start, $end - $start)
$fixture = @'
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
typedef unsigned u32; typedef int s32; typedef float f32;
enum CharacterSound { CHAR_SOUND_PUNCH_YAH, CHAR_SOUND_PUNCH_WAH, CHAR_SOUND_PUNCH_HOO, OTHER_VOICE };
enum { ACT_FLAG_AIR=1, ACT_FLAG_SWIMMING=2, ACT_FLAG_ON_POLE=4, INPUT_A_PRESSED=1 };
enum { ACT_JUMP_KICK=0x101, MARIO_ACTION_SOUND_PLAYED=8 };
struct Character { float soundFreqScale; } character={1};
struct Object { struct { struct { float cameraToObject[3]; } gfx; } header; } object;
struct MarioState { int playerIndex; u32 action,flags,input; void* floor; float pos[3],floorHeight; struct Object* marioObj; };
static bool jumpPriority;
static bool vr_jump_gesture_has_priority(void){return jumpPriority;}
static bool active=true;
static int played;
static float gGlobalSoundSource[3];
static bool vr_is_active(void) { return active; }
static int get_character_sound(struct MarioState* m,enum CharacterSound s) { (void)m; (void)s; return 1; }
static struct Character* get_character(struct MarioState* m) { (void)m; return &character; }
static void play_sound_with_freq_scale(int s,float* p,float f) { (void)s; (void)p; (void)f; played++; }
'@
$fixture += "`n" + $function + @'
int main(void) {
 struct MarioState m={.floor=&object,.marioObj=&object};
 for(int s=0;s<3;s++) play_character_sound_internal(&m,s,0,0);
 assert(played==3);
 jumpPriority=true;play_character_sound_internal(&m,CHAR_SOUND_PUNCH_YAH,0,0);assert(played==3);
 jumpPriority=false;m.input=INPUT_A_PRESSED;
 play_character_sound_internal(&m,CHAR_SOUND_PUNCH_YAH,0,0);assert(played==3);m.input=0;
 m.action=ACT_FLAG_AIR;
 for(int s=0;s<3;s++) play_character_sound_internal(&m,s,0,8);
 assert(played==3 && (m.flags&8));
 play_character_sound_internal(&m,OTHER_VOICE,0,0);
 assert(played==4);
 m.action=0; m.pos[1]=50; m.flags=0;
 play_character_sound_internal(&m,CHAR_SOUND_PUNCH_YAH,0,0);
 assert(played==4);
 m.playerIndex=1;
 play_character_sound_internal(&m,CHAR_SOUND_PUNCH_YAH,0,0);
 assert(played==5);
 m.playerIndex=0; active=false;
 play_character_sound_internal(&m,CHAR_SOUND_PUNCH_YAH,0,0);
 assert(played==6);
 active=true; m.action=ACT_JUMP_KICK; m.flags=0; m.input=INPUT_A_PRESSED; jumpPriority=true;
 play_character_sound_internal(&m,CHAR_SOUND_PUNCH_HOO,0,MARIO_ACTION_SOUND_PLAYED);
 assert(played==7);
 play_character_sound_internal(&m,CHAR_SOUND_PUNCH_HOO,0,MARIO_ACTION_SOUND_PLAYED);
 assert(played==7); // Native kick cue is still once per action.
 play_character_sound_internal(&m,CHAR_SOUND_PUNCH_HOO,0,0);
 play_character_sound_internal(&m,CHAR_SOUND_PUNCH_YAH,0,0);
 assert(played==7); // Incidental airborne voices remain suppressed.
 puts("PASS: grounded voices, airborne suppression, one-shot jump-kick HOO, other voices, remote and flat-screen preservation");
}
'@
# Generated compilation fixture only; the production function above is verbatim.
$generated = Join-Path $repo 'build/test_vr_punch_voice.c'
[IO.File]::WriteAllText($generated, $fixture)
& 'C:\msys64\mingw64\bin\gcc.exe' -std=c99 -Wall -Wextra -Werror $generated -o (Join-Path $repo 'build/test_vr_punch_voice.exe')
if ($LASTEXITCODE -ne 0) { throw 'Voice fixture compilation failed' }
& (Join-Path $repo 'build/test_vr_punch_voice.exe')
if ($LASTEXITCODE -ne 0) { throw 'Voice fixture failed' }
