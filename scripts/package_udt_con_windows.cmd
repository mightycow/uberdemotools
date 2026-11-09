@echo off
setlocal enabledelayedexpansion

PATH=%PATH%;C:\Programs\7-zip

set "LIST_FILE=package_udt_con_windows.lst"
set "ZIP_FILE=udt_con.zip"

:: nuke old archive
if exist "%ZIP_FILE%" del "%ZIP_FILE%"

:: add the files flat, need to loop because 7z doesn't have the -eq option
for /f "usebackq delims=" %%i in ("%LIST_FILE%") do (
	set "FULL_PATH=%%~fi"
	set "FILE_DIR=%%~dpi"
	set "FILE_NAME=%%~nxi"

	pushd "!FILE_DIR!"
	echo %~dp0%ZIP_FILE% ^<- !FILE_NAME!
	7za.exe a -tzip -mx9 "%~dp0%ZIP_FILE%" "!FILE_NAME!" >nul
	popd
)

pause
