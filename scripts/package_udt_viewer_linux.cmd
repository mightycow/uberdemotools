PATH=%PATH%;C:\Programs\7-zip

set "TAR_FILE=udt_viewer_linux.tar"
set "GZ_FILE=udt_viewer_linux.tar.gz"

if exist "%TAR_FILE%" del "%TAR_FILE%"
if exist "%GZ_FILE%" del "%GZ_FILE%"

del /Q __temp\viewer_data\*.*
rd /S /Q __temp
mkdir __temp
mkdir __temp\viewer_data

:: populate temp folder with everything
:: @TODO: use the Linux build
..\UDT_DLL\.bin\vs2022\x64\release\viewer_data_gen.exe -o=__temp\viewer_data ..\viewer_data
copy /Y ..\UDT_DLL\.bin\gmake_linux\x64\release\UDT_viewer __temp
copy /Y ..\changelog_viewer.txt __temp
copy /Y ..\viewer_data\map_aliases.txt __temp\viewer_data
copy /Y ..\viewer_data\deja_vu_sans.ttf __temp\viewer_data
copy /Y ..\viewer_data\blender_icons.png __temp\viewer_data
copy /Y ..\viewer_data\maps\*.png __temp\viewer_data

:: temp -> .tar
pushd __temp
7za.exe a -ttar "%~dp0%TAR_FILE%" . >nul
popd

:: .tar -> .tar.gz
7za.exe a -tgzip -mx=9 "%GZ_FILE%" "%TAR_FILE%"

:: nuke .tar
if exist "%TAR_FILE%" del "%TAR_FILE%"

rd /S /Q __temp

pause
