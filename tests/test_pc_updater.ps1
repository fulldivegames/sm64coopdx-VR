$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$testRoot=Join-Path $root ('build/updater-test-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $testRoot | Out-Null
$source=Get-Content (Join-Path $root 'src/pc/update_checker.c') -Raw
$block=($source -split 'const int scriptResult = fprintf\(script,')[1] -split 'escapedExeDir, escapedExePath' | Select-Object -First 1
$template=([regex]::Matches($block,'(?m)^\s*"((?:\\.|[^"\\])*)"') | ForEach-Object { [regex]::Unescape($_.Groups[1].Value) }) -join ''
if (!$template.Contains('DownloadFile')) {throw 'Failed to extract updater'}
Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach ($case in @('matching','missing','corrupt','changed','legacy','bad_download')) {
    $caseRoot=Join-Path $testRoot $case
    $install=Join-Path $caseRoot 'install'; $full=Join-Path $caseRoot 'full/payload'; $small=Join-Path $caseRoot 'small/payload'
    foreach ($dir in @($install,$full,$small)) {New-Item -ItemType Directory -Force (Join-Path $dir 'speech') | Out-Null}
    foreach ($dir in @($full,$small)) {[IO.File]::WriteAllText((Join-Path $dir 'SM64-Co-Op-DX-VR.exe'),'current-game')}
    $model=Join-Path $full 'speech/ggml-small-q5_1.bin'
    [IO.File]::WriteAllText($model,'same-model')
    $localModel=Join-Path $install 'speech/ggml-small-q5_1.bin'
    if ($case -ne 'missing') {[IO.File]::WriteAllText($localModel,$(if($case -in @('corrupt','changed')) {'different-model'} else {'same-model'}))}
    [IO.File]::WriteAllText((Join-Path $install 'SM64-Co-Op-DX-VR.exe'),'old-game')
    foreach ($name in @('save.bin','palette.json','mod.lua','settings.txt')) {[IO.File]::WriteAllText((Join-Path $install $name),'user-content')}
    $fullZip=Join-Path $caseRoot 'full.zip'; $smallZip=Join-Path $caseRoot 'small.zip'
    [IO.Compression.ZipFile]::CreateFromDirectory((Split-Path $full),$fullZip)
    [IO.Compression.ZipFile]::CreateFromDirectory((Split-Path $small),$smallZip)
    $script:mockManifest=@{schema=1;version='v99.0';model_sha256=(Get-FileHash $model).Hash;full_sha256=(Get-FileHash $fullZip).Hash;update_sha256=(Get-FileHash $smallZip).Hash}
    if ($case -eq 'bad_download') {$script:mockManifest.update_sha256=('0'*64)}
    $assets=@(
      @{name='SM64-Co-Op-DX-VR-Windows-v99.0.zip';browser_download_url=([Uri]$fullZip).AbsoluteUri;size=(Get-Item $fullZip).Length},
      @{name='SM64-Co-Op-DX-VR-Windows-v99.0-update.zip';browser_download_url=([Uri]$smallZip).AbsoluteUri;size=(Get-Item $smallZip).Length}
    )
    if ($case -ne 'legacy') {$assets+=@{name='SM64-Co-Op-DX-VR-Windows-v99.0-update.json';browser_download_url='mock-manifest'}}
    $script:mockRelease=@{tag_name='v99.0';draft=$false;prerelease=$false;assets=$assets}
    function Invoke-RestMethod {param($Headers,$Uri) if($Uri -eq 'mock-manifest') {$script:mockManifest} else {$script:mockRelease}}
    function Start-Sleep {param($Seconds)}
    function Start-Process {param($FilePath) $script:launched=$true}
    # Retain test fixtures; never execute the production cleanup commands here.
    function Remove-Item {param($LiteralPath,[switch]$Force,[switch]$Recurse,$ErrorAction)}
    $script:launched=$false
    $scriptText=$template
    foreach($value in @($install,(Join-Path $install 'SM64-Co-Op-DX-VR.exe'),(Join-Path $caseRoot 'download.zip'),(Join-Path $caseRoot 'stage'),(Join-Path $caseRoot 'unused.ps1'))) {
      $replacement=$value.Replace("'","''")
      $scriptText=([regex]'%s').Replace($scriptText,[Text.RegularExpressions.MatchEvaluator]{param($m) $replacement},1)
    }
    $failed=$false
    try { & ([scriptblock]::Create($scriptText)) } catch { if($case -ne 'bad_download') {throw}; $failed=$true }
    $actual=Get-Content (Join-Path $install 'SM64-Co-Op-DX-VR.exe') -Raw
    if($case -eq 'bad_download') {if(!$failed -or $script:launched -or $actual -ne 'old-game') {throw 'Corrupt download was not blocked'}}
    else {
      if(!$script:launched -or $actual -ne 'current-game') {throw 'Game not updated'}
      if((Get-FileHash $localModel).Hash -ne $script:mockManifest.model_sha256) {throw 'Model missing after update'}
      $expectedZip=if($case -eq 'matching') {$smallZip} else {$fullZip}
      if((Get-FileHash (Join-Path $caseRoot 'download.zip')).Hash -ne (Get-FileHash $expectedZip).Hash) {throw 'Wrong package selected'}
    }
    foreach($name in @('save.bin','palette.json','mod.lua','settings.txt')) {if((Get-Content (Join-Path $install $name) -Raw) -ne 'user-content') {throw 'User data changed'}}
    Write-Host "PASS: $case; preserved user files"
}
