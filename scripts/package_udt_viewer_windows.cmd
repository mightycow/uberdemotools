PATH=%PATH%;C:\Programs\7-zip

set "ZIP_FILE=udt_viewer.zip"

if exist "%ZIP_FILE%" del "%ZIP_FILE%"

del /Q __temp\viewer_data\*.*
rd /S /Q __temp
mkdir __temp
mkdir __temp\viewer_data

:: populate temp folder with everything
..\UDT_DLL\.bin\vs2022\x64\release\viewer_data_gen.exe -o=__temp\viewer_data ..\viewer_data
copy /Y ..\UDT_DLL\.bin\vs2022\x64\release\UDT_viewer.exe __temp
copy /Y ..\changelog_viewer.txt __temp
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
