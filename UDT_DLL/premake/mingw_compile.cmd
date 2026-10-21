@echo off

echo Which target?
echo R: Release
echo D: Debug
set /p choice=
if %choice%==r (
	set gmake_target=release
) else if %choice%==d (
	set gmake_target=debug
) else (
	echo Invalid choice
	exit /b 1
)

echo Clean?
echo Y: Yes
echo N: No
set /p choice=
if %choice%==y (
	set gmake_clean=1
) else if %choice%==n (
	set gmake_clean=0
) else (
	echo Invalid choice
	exit /b 1
)

set gmake_arch=x64
set gmake_config=%gmake_target%_%gmake_arch%
set gmake_options=--jobs=8 config=%gmake_config%

pushd ..\.build\win_mingw
@echo on
if %gmake_clean%==1 (
	mingw32-make.exe %gmake_options% clean
)
mingw32-make.exe %gmake_options% UDT
mingw32-make.exe %gmake_options% UDT_captures
mingw32-make.exe %gmake_options% UDT_converter
mingw32-make.exe %gmake_options% UDT_cutter
mingw32-make.exe %gmake_options% UDT_json
mingw32-make.exe %gmake_options% UDT_merger
mingw32-make.exe %gmake_options% UDT_splitter
mingw32-make.exe %gmake_options% UDT_timeshifter
mingw32-make.exe %gmake_options% UDT_viewer
mingw32-make.exe %gmake_options% viewer_data_gen
mingw32-make.exe %gmake_options% tests
popd
pause
