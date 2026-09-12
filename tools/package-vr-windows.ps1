[CmdletBinding()]
param(
    [string]$BuildDirectory = "build/us_pc",
    [string]$OutputDirectory = "dist",
    [string]$Version = "dev"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $BuildDirectory))
$outputPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDirectory))
$safeVersion = $Version -replace '[^A-Za-z0-9._-]', '-'
$packageName = "SM64-Co-Op-DX-VR-Windows-$safeVersion"
$stagePath = Join-Path $outputPath $packageName
$zipPath = Join-Path $outputPath "$packageName.zip"

$requiredFiles = @(
    "sm64coopdx.exe",
    "discord_game_sdk.dll",
    "libopenxr_loader.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
    "openxr-sdk-LICENSE",
    "gcc-COPYING.LIB",
    "gcc-COPYING.RUNTIME",
    "gcc-COPYING3",
    "libwinpthread-COPYING",
    "normal_maps.bin"
)

$requiredDirectories = @(
    "dynos",
    "lang",
    "mods",
    "palettes",
    "sonic_shoes",
    "speech"
)

foreach ($file in $requiredFiles) {
    $candidate = Join-Path $buildPath $file
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "Missing required build file: $candidate. Build the Windows game before packaging."
    }
}
$speechFiles = @('whisper-cli.exe','whisper-cli-avx2.exe','ggml-small-q5_1.bin','whisper-LICENSE.txt','model-LICENSE.txt','PC-DICTATION.txt')
foreach ($file in $speechFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $buildPath "speech/$file") -PathType Leaf)) {
        throw "Missing speech/$file. Run tools/prepare-pc-dictation.ps1 before packaging."
    }
}

foreach ($directory in $requiredDirectories) {
    $candidate = Join-Path $buildPath $directory
    if (-not (Test-Path -LiteralPath $candidate -PathType Container)) {
        throw "Missing required build directory: $candidate. Build the Windows game before packaging."
    }
}

New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
if (Test-Path -LiteralPath $stagePath) {
    Remove-Item -LiteralPath $stagePath -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
New-Item -ItemType Directory -Path $stagePath | Out-Null

Copy-Item -LiteralPath (Join-Path $buildPath "sm64coopdx.exe") -Destination (Join-Path $stagePath "SM64-Co-Op-DX-VR.exe")

Copy-Item -LiteralPath (Join-Path $buildPath "discord_game_sdk.dll") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $buildPath "libopenxr_loader.dll") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $buildPath "libgcc_s_seh-1.dll") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $buildPath "libstdc++-6.dll") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $buildPath "libwinpthread-1.dll") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $buildPath "normal_maps.bin") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $repoRoot "docs/PC-VR-PLAYER-GUIDE.txt") -Destination (Join-Path $stagePath "README.txt")
Copy-Item -LiteralPath (Join-Path $repoRoot "release_notes.txt") -Destination (Join-Path $stagePath "release_notes.txt")
Copy-Item -LiteralPath (Join-Path $repoRoot "tools/Launch-VR-Diagnostics.cmd") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $repoRoot "docs/VR-SPEEDRUN.txt") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $repoRoot "docs/PROPELLER-MUSHROOM.txt") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $repoRoot "docs/PHYSICAL-JUMPING.txt") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $repoRoot "docs/PHYSICAL-SWIMMING.txt") -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $repoRoot "docs/POWER-UP-SPAWN-WEIGHTS.txt") -Destination $stagePath

$licensesPath = Join-Path $stagePath "licenses"
New-Item -ItemType Directory -Path $licensesPath | Out-Null
Copy-Item -LiteralPath (Join-Path $buildPath "openxr-sdk-LICENSE") -Destination (Join-Path $licensesPath "OpenXR-SDK-LICENSE.txt")
Copy-Item -LiteralPath (Join-Path $buildPath "gcc-COPYING.LIB") -Destination (Join-Path $licensesPath "GCC-COPYING.LIB.txt")
Copy-Item -LiteralPath (Join-Path $buildPath "gcc-COPYING.RUNTIME") -Destination (Join-Path $licensesPath "GCC-Runtime-Library-Exception.txt")
Copy-Item -LiteralPath (Join-Path $buildPath "gcc-COPYING3") -Destination (Join-Path $licensesPath "GCC-GPL-3.0.txt")
Copy-Item -LiteralPath (Join-Path $buildPath "libwinpthread-COPYING") -Destination (Join-Path $licensesPath "libwinpthread-COPYING.txt")

foreach ($directory in $requiredDirectories) {
    if ($directory -eq 'speech') {
        $speechStage = Join-Path $stagePath 'speech'
        New-Item -ItemType Directory -Path $speechStage | Out-Null
        foreach ($file in $speechFiles) {
            Copy-Item -LiteralPath (Join-Path $buildPath "speech/$file") -Destination $speechStage
        }
        continue
    }
    Copy-Item -LiteralPath (Join-Path $buildPath $directory) -Destination $stagePath -Recurse
}

$romExtensions = @(".z64", ".n64", ".v64")
$romFiles = @(
    Get-ChildItem -LiteralPath $stagePath -Recurse -File |
        Where-Object { $_.Extension.ToLowerInvariant() -in $romExtensions }
)
if ($romFiles.Count -ne 0) {
    throw "Packaging stopped because a ROM file was found in the staging directory."
}

Compress-Archive -LiteralPath $stagePath -DestinationPath $zipPath -CompressionLevel Optimal

# A complete game update excluding only the immutable, checksum-identified model.
# Keep the full fresh-install archive intact; stream entries into a second ZIP.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$updateName = "$packageName-update.zip"
$updatePath = Join-Path $outputPath $updateName
$manifestPath = Join-Path $outputPath "$packageName-update.json"
if ((Test-Path -LiteralPath $updatePath) -or (Test-Path -LiteralPath $manifestPath)) {
    throw 'Update outputs already exist; refusing to overwrite them.'
}
$modelRelative = 'speech/ggml-small-q5_1.bin'
$modelHash = (Get-FileHash -LiteralPath (Join-Path $stagePath $modelRelative) -Algorithm SHA256).Hash.ToLowerInvariant()
$sourceZip = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $updateZip = [IO.Compression.ZipFile]::Open($updatePath, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($entry in $sourceZip.Entries) {
            if ($entry.FullName -eq "$packageName/$modelRelative") { continue }
            $copy = $updateZip.CreateEntry($entry.FullName, [IO.Compression.CompressionLevel]::Optimal)
            $inputStream = $entry.Open(); $outputStream = $copy.Open()
            try { $inputStream.CopyTo($outputStream) } finally { $inputStream.Dispose(); $outputStream.Dispose() }
        }
    } finally { $updateZip.Dispose() }
} finally { $sourceZip.Dispose() }
$manifest = @{
    schema=1; version=$Version; model_sha256=$modelHash
    full_sha256=(Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    update_sha256=(Get-FileHash -LiteralPath $updatePath -Algorithm SHA256).Hash.ToLowerInvariant()
} | ConvertTo-Json
[IO.File]::WriteAllText($manifestPath, $manifest, [Text.UTF8Encoding]::new($false))
Write-Host "Also created $updatePath and $manifestPath. Publish all three PC assets together."

Write-Host "Created player package: $zipPath"
Write-Host "The package contains no ROM, map, debug database, or backup executable."
