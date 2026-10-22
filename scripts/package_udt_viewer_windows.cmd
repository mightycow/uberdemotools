@echo off

call ..\UDT_DLL\premake\helpers\vs_version_selector.cmd
set bin_path=..\UDT_DLL\.bin\%vs_generator%\x64\release

PATH=%PATH%;C:\Programs\7-zip

set "ZIP_FILE=udt_viewer.zip"

@echo on

if exist "%ZIP_FILE%" del "%ZIP_FILE%"

del /Q __temp\viewer_data\*.*
rd /S /Q __temp
mkdir __temp
mkdir __temp\viewer_data

:: populate temp folder with everything
%bin_path%\viewer_data_gen.exe -o=__temp\viewer_data ..\viewer_data
copy /Y %bin_path%\UDT_viewer.exe __temp
copy /Y ..\changelog_viewer.txt __temp\changelog.txt
copy /Y ..\viewer_data\map_aliases.txt __temp\viewer_data
copy /Y ..\viewer_data\deja_vu_sans.ttf __temp\viewer_data
copy /Y ..\viewer_data\blender_icons.png __temp\viewer_data
copy /Y ..\viewer_data\maps\*.png __temp\viewer_data

:: temp -> .zip
pushd __temp
7za.exe a -tzip -mx9 "%~dp0%ZIP_FILE%" . >nul
popd

rd /S /Q __temp

pause
