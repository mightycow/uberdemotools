PATH=%PATH%;C:\Program Files\WinRAR
del /Q __temp\viewer_data\*.*
rd /S /Q __temp
mkdir __temp\viewer_data
..\UDT_DLL\.bin\gmake_linux\x64\release\viewer_data_gen.exe -o=__temp\viewer_data ..\viewer_data
del udt_viewer_linux.zip
WinRAR.exe a udt_viewer_linux.zip -ep1 __temp\viewer_data
WinRAR.exe a udt_viewer_linux.zip ..\changelog_viewer.txt
WinRAR.exe a udt_viewer_linux.zip ..\viewer_data\map_aliases.txt
WinRAR.exe a udt_viewer_linux.zip ..\viewer_data\deja_vu_sans.ttf
WinRAR.exe a udt_viewer_linux.zip ..\viewer_data\blender_icons.png
WinRAR.exe a udt_viewer_linux.zip -apviewer_data -ep1 ..\viewer_data\maps\*.png
WinRAR.exe a udt_viewer_linux.zip -ep ..\UDT_DLL\.bin\gmake_linux\x64\release\UDT_viewer
rd /S /Q __temp
pause
