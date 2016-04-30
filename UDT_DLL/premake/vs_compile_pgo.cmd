@echo off
call "helpers/vs_version_selector.cmd"

set CONFIG=ReleaseInst

echo What action to perform?
echo 1. Build instrumented binaries
echo 2. Profile the binaries
echo 3. Build optimized binaries
set /p action=
if %action%==1 (
	del ..\.bin\%vs_generator%\x64\release\*.pgd
	del ..\.bin\%vs_generator%\x64\release\*.pgc
	call :Build
) else if %action%==2 (
	call :Profile
) else if %action%==3 (
	set CONFIG=ReleaseOpt
	call :Build
) else (
    echo Invalid choice
	exit
)

:Build

@echo on
call "helpers/vs_msbuild.cmd" UDT %CONFIG% x64
call "helpers/vs_msbuild.cmd" UDT_captures %CONFIG% x64
call "helpers/vs_msbuild.cmd" UDT_converter %CONFIG% x64
call "helpers/vs_msbuild.cmd" UDT_cutter %CONFIG% x64
call "helpers/vs_msbuild.cmd" UDT_json %CONFIG% x64

pause
exit

:Profile

@echo off
:: relative to the binaries
set DEMOPATH=..\..\..\..\..\demo_files
set MERGEPATH=%DEMOPATH%\dh_2011_summer_tdm_dignitas_vs_ffa_on_hf
PATH=%vs_path%..\..\VC\bin\amd64;%PATH%
cd ..\.bin\%vs_generator%\x64\release
copy ..\..\..\..\..\UDT_GUI\.bin\x64\Release\UDT_GUI.exe .
@echo on
UDT_captures.exe -r -q -o=captures.json "%DEMOPATH%"
UDT_converter.exe -r -q -p=68 -o=cut "%DEMOPATH%\dm3"
UDT_converter.exe -r -q -p=68 -o=cut "%DEMOPATH%\dm_48"
UDT_converter.exe -r -q -p=91 -o=cut "%DEMOPATH%\dm_73"
UDT_converter.exe -r -q -p=91 -o=cut "%DEMOPATH%\dm_90"
UDT_cutter.exe m -r -q -o=cut "%DEMOPATH%"
UDT_json.exe -r -q -o=cut "%DEMOPATH%"
UDT_GUI.exe "%DEMOPATH%"

pause
exit