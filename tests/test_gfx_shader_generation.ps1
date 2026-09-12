param([string]$Compiler = 'C:\msys64\mingw64\bin\gcc.exe', [switch]$Gles)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'build/shader-validation'
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source, [string]$start, [string]$end) {
    $a = $source.IndexOf($start)
    $b = $source.IndexOf($end, $a)
    if ($a -lt 0 -or $b -lt 0) { throw "Shader test extraction marker missing: $start" }
    return $source.Substring($a, $b - $a)
}
$gl = Get-Content -Raw (Join-Path $root 'src/pc/gfx/gfx_opengl.c')
$cc = Get-Content -Raw (Join-Path $root 'src/pc/gfx/gfx_cc.c')
$pc = Get-Content -Raw (Join-Path $root 'src/pc/gfx/gfx_pc.c')
$sdl = Get-Content -Raw (Join-Path $root 'src/pc/gfx/gfx_sdl.c')
$helpers = Section $gl 'static void append_str(' 'static struct ShaderProgram *gfx_opengl_create_and_load_new_shader('
$generate = Section $gl '    struct CCFeatures ccf = { 0 };' "    vs_buf[vs_len] = '\0';"
# Use larger guarded test storage to report the exact production capacity violation,
# rather than letting a production overflow corrupt the test process first.
$vsCapacity = [regex]::Match($generate, 'char vs_buf\[(\d+)\]').Groups[1].Value
$fsCapacity = [regex]::Match($generate, 'char fs_buf\[(\d+)\]').Groups[1].Value
if (!$vsCapacity -or !$fsCapacity) { throw 'Shader capacity markers missing' }
$generate = [regex]::Replace($generate, 'char ([vf]s_buf)\[\d+\]', 'char $1[65536]')
$features = Section $cc 'void gfx_cc_get_features(' 'void gfx_cc_print('
$mapping = (Section $pc 'static void gfx_generate_cc(' '    color_combiner_update_hash(cc);') + "}`n"
$cases = Section $cc 'void gfx_cc_precomp(void)' 'static uint8_t color_comb_component_a('
$probe = Section $sdl 'bool gfx_sdl_check_opengl_compatibility(void)' 'static void gfx_sdl_main_loop('
$capacity = [regex]::Match($gl,'(?m)^#define OPENGL_MAX_SHADER_ATTRIBUTES .+$').Value
if (!$capacity) { throw 'Attribute capacity definition missing' }
foreach ($array in @('attrib_locations','attrib_sizes','opengl_attrib_layout_locations','opengl_attrib_layout_sizes')) {
    if ($gl -notmatch ($array+'\[OPENGL_MAX_SHADER_ATTRIBUTES\]')) { throw "Attribute capacity regressed: $array" }
}
$menu = Get-Content -Raw (Join-Path $root 'src/pc/gfx/gfx_menu_target.h')
$cell = Get-Content -Raw (Join-Path $root 'src/pc/gfx/gfx_cell_shaded.h')
$menuSource = Section $menu '#if defined(USE_GLES) || defined(__ANDROID__)' '    GLuint vs = menu_target_shader('
$cellSource = Section $cell '#ifdef __ANDROID__' '    GLuint vs=menu_target_shader('
# Exercise the Android shader text in the ES context without defining Android
# for the Windows test harness itself.
if ($Gles) { $cellSource = $cellSource.Replace('#ifdef __ANDROID__','#if 1') }
$post = "static void validate_post_effects(void) { struct ColorCombiner cc={0}; {`n" +
    $menuSource + "validate(&cc,vertex,fragment,strlen(vertex),strlen(fragment)); } {`n" +
    $cellSource + "validate(&cc,vertex,fragment,strlen(vertex),strlen(fragment)); } }`n"
$generated = "$capacity`n#define VS_CAPACITY $vsCapacity`n#define FS_CAPACITY $fsCapacity`n" + $features + $helpers + $mapping +
    "static void generate(struct ColorCombiner *cc) {`n" + $generate +
    "vs_buf[vs_len]=0; fs_buf[fs_len]=0; validate(cc,vs_buf,fs_buf,vs_len,fs_len); (void)num_floats; }`n" + $cases + $probe + $post
# This is a generated test translation unit, never game source.
[IO.File]::WriteAllText((Join-Path $out 'generated.h'), $generated)
$env:PATH = (Split-Path $Compiler) + ';' + $env:PATH
$defines = @()
if ($Gles) { $defines += '-DUSE_GLES' }
& $Compiler -std=c99 -Wall -Wextra -Werror @defines (Join-Path $PSScriptRoot 'test_gfx_shader_generation.c') "-I$out" -o (Join-Path $out 'test.exe') -lglew32 -lSDL2 -lopengl32
if ($LASTEXITCODE) { throw 'Shader test compilation failed' }
& (Join-Path $out 'test.exe')
if ($LASTEXITCODE) { throw 'Shader validation failed' }
