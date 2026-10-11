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
set zigcc=SHELL=cmd.exe CC="zig cc -target x86_64-linux-gnu" CXX="zig c++ -target x86_64-linux-gnu" LD="zig c++ -target x86_64-linux-gnu" AR="zig ar"
set gmake_options=--jobs=8 %zigcc% config=%gmake_config%

cd ..\.build\gmake_linux

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
pause
