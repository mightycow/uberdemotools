PATH=%PATH%;C:\Program Files\WinRAR
del /Q __temp\viewer_data\*.*
mkdir __temp\viewer_data
..\UDT_DLL\.bin\vs2022\x64\release\viewer_data_gen.exe -o=__temp\viewer_data ..\viewer_data
call :Archive x64
rd /S /Q __temp
pause
exit
:Archive
del udt_viewer.zip
WinRAR.exe a udt_viewer.zip -ep1 __temp\viewer_data
WinRAR.exe a udt_viewer.zip ..\changelog_viewer.txt
WinRAR.exe a udt_viewer.zip ..\viewer_data\map_aliases.txt
WinRAR.exe a udt_viewer.zip ..\viewer_data\deja_vu_sans.ttf
WinRAR.exe a udt_viewer.zip ..\viewer_data\blender_icons.png
WinRAR.exe a udt_viewer.zip -apviewer_data -ep1 ..\viewer_data\maps\*.png
WinRAR.exe a udt_viewer.zip -ep ..\UDT_DLL\.bin\vs2022\%1\release\UDT_viewer.exe
goto:eof