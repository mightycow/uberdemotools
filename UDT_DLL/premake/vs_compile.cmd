@echo off
call "helpers/vs_version_selector.cmd"
call "helpers/vs_express_selector.cmd"
call "helpers/vs_build_selector.cmd"

set "VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

:: Check if vswhere exists in the standard location
if not exist "%VSWHERE_EXE%" (
	echo Error: vswhere.exe not found at standard location.
	exit /b 1
)

:: Query the latest VS installation that specifically has C++ tools installed
for /f "usebackq tokens=*" %%i in (`"%VSWHERE_EXE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
	set "InstallDir=%%i"
)

:: Execute the environment initialization script safely
if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (
	call "%InstallDir%\Common7\Tools\vsdevcmd.bat"
) else (
	echo Error: vsdevcmd.bat not found in the resolved installation directory.
	exit /b 1
)

%InstallDir%\Common7\IDE\devenv.exe..\.build\%vs_generator%\UDT.sln /Rebuild "%vs_target%|%vs_arch%"

pause