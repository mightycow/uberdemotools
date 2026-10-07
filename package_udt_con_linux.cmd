@echo off
setlocal enabledelayedexpansion

PATH=%PATH%;C:\Programs\7-zip

set "LIST_FILE=package_udt_con_bin_linux.lst"
set "TAR_FILE=udt_con_linux.tar"
set "GZ_FILE=udt_con_linux.tar.gz"

:: nuke old archives
if exist "%TAR_FILE%" del "%TAR_FILE%"
if exist "%GZ_FILE%" del "%GZ_FILE%"

:: add the files flat, need to loop because 7z doesn't have the -eq option
for /f "usebackq delims=" %%i in ("%LIST_FILE%") do (
	set "FULL_PATH=%%~fi"
	set "FILE_DIR=%%~dpi"
	set "FILE_NAME=%%~nxi"

	pushd "!FILE_DIR!"
	7za.exe a -ttar "%~dp0%TAR_FILE%" "!FILE_NAME!" >nul
	popd
)

:: .tar -> .tar.gz
7za.exe a -tgzip -mx=9 "%GZ_FILE%" "%TAR_FILE%"

:: nuke .tar
if exist "%TAR_FILE%" del "%TAR_FILE%"

pause
