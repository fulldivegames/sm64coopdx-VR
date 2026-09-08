param([string]$OutputDirectory = 'build/us_pc/speech')
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$output = [IO.Path]::GetFullPath((Join-Path $repo $OutputDirectory))
if (-not $output.StartsWith($repo + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must stay inside the workspace' }
$cache = Join-Path $repo 'build/dictation-deps'
New-Item -ItemType Directory -Force $cache,$output | Out-Null
function Fetch-Checked($url, $path, $hash) {
    if (-not (Test-Path -LiteralPath $path)) { Invoke-WebRequest $url -OutFile $path }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $hash) { throw "Checksum mismatch: $path" }
}
$archive = Join-Path $cache 'source-b4938.zip'
Fetch-Checked 'https://codeload.github.com/ggml-org/whisper.cpp/zip/refs/tags/b4938' $archive '1c1471b9b33cc9c7018be80c623931167a9bf4e40313c1b2307a57637455129e'
$model = Join-Path $cache 'ggml-small-q5_1.bin'
Fetch-Checked 'https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small-q5_1.bin' $model 'ae85e4a935d7a567bd102fe55afc16bb595bdb618e11b2fc7591bc08120411bb'
$source = Join-Path $cache 'source/whisper.cpp-b4938'
if (-not (Test-Path -LiteralPath $source)) { Expand-Archive -LiteralPath $archive -DestinationPath (Join-Path $cache 'source') }
$cmake = (Get-Command cmake -ErrorAction Stop).Source
$ninja = (Get-Command ninja -ErrorAction Stop).Source
$previousPath = $env:PATH
try {
    $env:PATH = 'C:\msys64\mingw64\bin;' + $env:PATH
    foreach ($variant in @('generic','avx2')) {
        $build = Join-Path $cache $(if ($variant -eq 'generic') { 'mingw-b4938' } else { 'mingw-avx2-b4938' })
        $simd = if ($variant -eq 'avx2') { 'ON' } else { 'OFF' }
        & $cmake -S $source -B $build -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" '-DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe' '-DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe' '-DCMAKE_BUILD_TYPE=Release' '-DBUILD_SHARED_LIBS=OFF' '-DGGML_NATIVE=OFF' '-DGGML_OPENMP=OFF' "-DGGML_AVX=$simd" "-DGGML_AVX2=$simd" "-DGGML_FMA=$simd" "-DGGML_F16C=$simd" '-DGGML_BMI2=OFF' '-DGGML_SSE42=OFF' '-DWHISPER_BUILD_TESTS=OFF' '-DWHISPER_BUILD_SERVER=OFF' '-DWHISPER_SDL2=OFF' '-DCMAKE_EXE_LINKER_FLAGS=-static' '-DCMAKE_POLICY_VERSION_MINIMUM=3.5'
        if ($LASTEXITCODE -ne 0) { throw 'Speech runtime configure failed' }
        & $cmake --build $build --target whisper-cli -j 8
        if ($LASTEXITCODE -ne 0) { throw 'Speech runtime build failed' }
        $name = if ($variant -eq 'generic') { 'whisper-cli.exe' } else { 'whisper-cli-avx2.exe' }
        Copy-Item -LiteralPath (Join-Path $build 'bin/whisper-cli.exe') -Destination (Join-Path $output $name)
    }
} finally {
    $env:PATH = $previousPath
}
Copy-Item -LiteralPath $model -Destination $output
foreach ($license in @('whisper-LICENSE.txt','model-LICENSE.txt')) {
    $path = Join-Path $cache $license
    if (-not (Test-Path -LiteralPath $path)) {
        $url = if ($license -eq 'whisper-LICENSE.txt') { 'https://raw.githubusercontent.com/ggml-org/whisper.cpp/b4938/LICENSE' } else { 'https://raw.githubusercontent.com/openai/whisper/main/LICENSE' }
        Invoke-WebRequest $url -OutFile $path
    }
    Copy-Item -LiteralPath $path -Destination $output
}
Copy-Item -LiteralPath (Join-Path $repo 'docs/PC-DICTATION.txt') -Destination $output
Write-Output "Verified offline dictation runtime and model staged in $output"
