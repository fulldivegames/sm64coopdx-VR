@echo off
setlocal
cd /d "%~dp0"
if not exist "logs" mkdir "logs"
:picklog
set "vrDiagnosticLog=logs\vr-%RANDOM%-%RANDOM%.log"
if exist "%vrDiagnosticLog%" goto picklog
echo Recording diagnostics to %vrDiagnosticLog%
echo Play normally, then close the game to finish the log.
"SM64-Co-Op-DX-VR.exe" --console > "%vrDiagnosticLog%" 2>&1
echo Game exited. Diagnostics saved to %vrDiagnosticLog%
pause
