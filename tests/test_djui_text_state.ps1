$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Get-Content (Join-Path $root 'src/pc/djui/djui_text.c') -Raw
$body=[regex]::Match($source,'(?s)static void djui_text_render_single_char\(.*?(?=\r?\nstatic void djui_text_render_char)').Value
if(!$body){throw 'Glyph renderer not found'}
[IO.File]::WriteAllText((Join-Path $root 'build/test_djui_text_state.inc'),$body)
& 'C:\msys64\mingw64\bin\gcc.exe' (Join-Path $root 'tests/test_djui_text_state.c') -o (Join-Path $root 'build/test_djui_text_state.exe')
if($LASTEXITCODE){throw 'Compile failed'}
& (Join-Path $root 'build/test_djui_text_state.exe')
if($LASTEXITCODE){throw 'Glyph test failed'}
$gfx=Get-Content (Join-Path $root 'src/pc/djui/djui_gfx.c') -Raw
foreach($name in @('djui_gfx_render_texture_font_begin','djui_gfx_render_texture_tile_font_begin')) {
 $function=[regex]::Match($gfx,"(?s)void $name\(\) \{.*?\n\}").Value
 foreach($flag in @('G_TEXTURE_GEN','G_TEXTURE_GEN_LINEAR','G_FOG','G_ZBUFFER','G_AC_NONE','G_TP_NONE','G_TT_NONE','G_TL_TILE')) {
  if(!$function.Contains($flag)){throw "Missing font state reset $name $flag"}
 }
}
Write-Output 'PASS: both font paths explicitly isolate 3D texture generation, fog and depth state.'
$native=Get-Content (Join-Path $root 'bin/segment2.c') -Raw
foreach($pattern in @('(?s)const Gfx dl_rgba16_text_begin\[\] = \{.*?\n\};','(?s)#elif defined\(VERSION_US\)\s+const Gfx dl_ia_text_begin\[\] = \{.*?\n\};')) {
 $function=[regex]::Match($native,$pattern).Value
 foreach($flag in @('G_TEXTURE_GEN','G_TEXTURE_GEN_LINEAR','G_FOG','G_ZBUFFER','G_CULL_BOTH','G_AC_NONE','G_TP_NONE','G_TT_NONE','G_TL_TILE','G_CYC_1CYCLE')) {
  if(!$function.Contains($flag)){throw "Native text state missing $flag"}
 }
}
Write-Output 'PASS: native text state reset covers foggy/environment-mapped scenes, alpha test, texture LUT/LOD and cycle type.'
$setter=[regex]::Match($source,'(?s)void djui_text_set_text\(.*?(?=\r?\nvoid djui_text_set_font\()').Value
if(!$setter){throw 'Text setter extraction failed'}
[IO.File]::WriteAllText((Join-Path $root 'build/test_djui_text_storage.inc'),$setter)
& 'C:\msys64\mingw64\bin\gcc.exe' (Join-Path $root 'tests/test_djui_text_storage.c') -o (Join-Path $root 'build/test_djui_text_storage.exe')
if($LASTEXITCODE){throw 'Storage compile failed'}
& (Join-Path $root 'build/test_djui_text_storage.exe')
if($LASTEXITCODE){throw 'Storage test failed'}
