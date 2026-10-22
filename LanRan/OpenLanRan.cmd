@echo off
setlocal
set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
if exist "D:\UE-devlop\engine\UE_5.8\Engine\Build\BatchFiles\Build.bat" (
  set "UE_ROOT=D:\UE-devlop\engine\UE_5.8"
)
if not "%~1"=="" set "UE_ROOT=%~1"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
  echo Unreal Engine was not found at "%UE_ROOT%".
  echo Usage: OpenLanRan.cmd "Your Unreal Engine root directory"
  pause
  exit /b 1
)
if not exist "%~dp0Saved\Logs" mkdir "%~dp0Saved\Logs"
echo Building LanRanEditor with "%UE_ROOT%"...
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" LanRanEditor Win64 Development "-Project=%~dp0LanRan.uproject" -WaitMutex -NoHotReloadFromIDE > "%~dp0Saved\Logs\ManualEditorBuild.log" 2>&1
if errorlevel 1 (
  type "%~dp0Saved\Logs\ManualEditorBuild.log"
  echo Build failed. Log: "%~dp0Saved\Logs\ManualEditorBuild.log"
  pause
  exit /b 1
)
echo Build succeeded. Opening LanRan...
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0LanRan.uproject"
