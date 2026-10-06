@echo off
setlocal enabledelayedexpansion

call "helpers/vs_version_selector.cmd"
call "helpers/vs_build_selector.cmd"

set "vs_where_path=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

:: check if vswhere exists in the standard location
if not exist "%vs_where_path%" (
	echo Error: vswhere.exe not found at standard location.
	exit /b 1
)

:: that comma is from hell
set "vs_where_range=[!vs_version!,)"

for /f "usebackq tokens=*" %%i in (`"%vs_where_path%" -version !vs_where_range! -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
	set "InstallDir=%%i"
)

:: run the environment initialization script
if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (
	call "%InstallDir%\Common7\Tools\vsdevcmd.bat"
) else (
	echo Error: vsdevcmd.bat not found in the resolved installation directory.
	exit /b 1
)

:: compile..
"%InstallDir%\Common7\IDE\devenv.com" ..\.build\%vs_generator%\UDT.sln /Rebuild "%vs_target%|%vs_arch%"

pause