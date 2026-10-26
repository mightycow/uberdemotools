@echo off
setlocal enabledelayedexpansion

set "vs_where_path=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

:: check if vswhere exists in the standard location
if not exist "%vs_where_path%" (
	echo Error: vswhere.exe not found at standard location.
	exit /b 1
)

:: that comma is from hell
set "vs_where_range=[17.0^,)"

:: find C# build tools for VS 2022 or later
for /f "usebackq tokens=*" %%i in (`"%vs_where_path%" -version !vs_where_range! -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
	set "vs_install_dir=%%i"
)

:: run the environment initialization script
if exist "%vs_install_dir%\Common7\Tools\vsdevcmd.bat" (
	call "%vs_install_dir%\Common7\Tools\vsdevcmd.bat"
) else (
	echo Error: vsdevcmd.bat not found in the resolved installation directory.
	exit /b 1
)

:: compile..
"%vs_install_dir%\Common7\IDE\devenv.com" UDT.sln /Rebuild "Release|x64"

pause