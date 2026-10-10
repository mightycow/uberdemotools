@echo off

echo Which target?
echo 1. Release
echo 2. Debug
set /p choice=
if %choice%==1 (
	set gmake_target=release
) else if %choice%==2 (
	set gmake_target=debug
) else (
	echo Invalid choice
	exit
)

echo Clean?
echo y. Yes
echo *. No
set /p choice=
if %choice%==y (
	set gmake_clean=1
) else if %choice%==2 (
	set gmake_clean=0
) else (
	echo Invalid choice
	exit
)

set gmake_arch=x64
set gmake_config=%gmake_target%_%gmake_arch%

cd ..\.build\gmake_windows

@echo on
if %gmake_clean%==1 (
	mingw32-make.exe clean
)
mingw32-make.exe config=%gmake_config% UDT
mingw32-make.exe config=%gmake_config% UDT_captures
mingw32-make.exe config=%gmake_config% UDT_converter
mingw32-make.exe config=%gmake_config% UDT_cutter
mingw32-make.exe config=%gmake_config% UDT_json
mingw32-make.exe config=%gmake_config% UDT_merger
mingw32-make.exe config=%gmake_config% UDT_splitter
mingw32-make.exe config=%gmake_config% UDT_timeshifter
mingw32-make.exe config=%gmake_config% UDT_viewer
mingw32-make.exe config=%gmake_config% viewer_data_gen
mingw32-make.exe config=%gmake_config% tests
pause
