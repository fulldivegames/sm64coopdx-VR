$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Get-Content (Join-Path $root 'src/pc/configfile.c') -Raw
$body=[regex]::Match($source,'(?s)static bool configfile_migrate_vr_physical_actions\(void\) \{.*?(?=\r?\nvoid configfile_load\()').Value
if(!$body){throw 'Migration extraction failed'}
if($source -notmatch '\.uintValue = &sConfigVrPhysicalActionsVersion'){throw 'Missing persisted migration marker'}
[IO.File]::WriteAllText((Join-Path $root 'build/test_vr_physical_migration.inc'),$body)
& 'C:\msys64\mingw64\bin\gcc.exe' (Join-Path $root 'tests/test_vr_physical_migration.c') -o (Join-Path $root 'build/test_vr_physical_migration.exe')
if($LASTEXITCODE){throw 'Compile failed'}
& (Join-Path $root 'build/test_vr_physical_migration.exe')
if($LASTEXITCODE){throw 'Migration failed'}
