@echo off
setlocal
set "UEEDITOR=D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%UEEDITOR%" (
  echo UE 5.8 was not found. Right-click IndigoWhitebox.uproject and select your Unreal Engine 5.8 installation.
  pause
  exit /b 1
)
start "" "%UEEDITOR%" "%~dp0IndigoWhitebox.uproject"
