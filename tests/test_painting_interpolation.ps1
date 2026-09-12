param([string]$Compiler='C:\msys64\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
 $s=Get-Content src/game/paintings.c -Raw
 $a=$s.IndexOf('void patch_paintings_interpolated(');$b=$s.IndexOf('void add_to_rippling_list(', $a)
 if($a -lt 0 -or $b -le $a){throw 'Extraction failed'}
 [IO.File]::WriteAllText((Join-Path $root 'build/test_painting_interpolation.inc'),$s.Substring($a,$b-$a))
 & $Compiler -std=c11 -O2 tests/test_painting_interpolation.c -o build/test_painting_interpolation.exe
 if($LASTEXITCODE){throw 'Compile failed'}
 & ./build/test_painting_interpolation.exe
 if($LASTEXITCODE){throw 'Test failed'}
} finally {Pop-Location}
