@echo off
setlocal

goto :PickAction

:Generate
call "%~dp0helpers\pick_vs_version.cmd" || goto :Exit
premake5.exe %vs_generator% --preset=win_win_vs
goto :Exit

:Test
call "%~dp0helpers\pick_vs_version.cmd" || goto :Exit
call "%~dp0helpers\run_tests.cmd" win_win_%vs_generator% || goto :Exit
goto :Exit

:Compile

setlocal EnableDelayedExpansion

call "%~dp0helpers\pick_vs_version.cmd" || goto :Exit
call "%~dp0helpers\pick_target.cmd" || goto :Exit
call "%~dp0helpers\pick_clean.cmd" || goto :Exit

set "vs_where_path=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

:: check if vswhere exists in the standard location
if not exist "%vs_where_path%" (
	echo Error: vswhere.exe not found at standard location.
	goto :Exit
)

:: that comma is from hell
set "vs_where_range=[!vs_version!,)"

:: find latest C++ build tools that can handle the VS solution version
for /f "usebackq tokens=*" %%i in (`"%vs_where_path%" -version !vs_where_range! -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
	set "vs_install_dir=%%i"
)

:: run the environment initialization script
if exist "%vs_install_dir%\Common7\Tools\vsdevcmd.bat" (
	call "%vs_install_dir%\Common7\Tools\vsdevcmd.bat" || goto :Exit
) else (
	echo Error: vsdevcmd.bat not found in the resolved installation directory.
	goto :Exit
)

:: compile..
if %clean%==1 (
	"%vs_install_dir%\Common7\IDE\devenv.com" ..\.build\win_win_%vs_generator%\UDT.sln /Clean "%target%|x64"
)
"%vs_install_dir%\Common7\IDE\devenv.com" ..\.build\win_win_%vs_generator%\UDT.sln /Build "%target%|x64"

goto :Exit

:PickAction
call "%~dp0helpers\pick_action.cmd" || goto :Exit
goto :%action%

:Exit
pause
