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

set gmake_arch=x64

set gmake_config=%gmake_target%_%gmake_arch%
cd ..\.build\gmake

::set zigcc=CC="zig cc" CXX="zig c++"
::set zigcc=SHELL=cmd.exe CC="zig cc" CXX="zig c++"
::set zigcc=SHELL=cmd.exe CC="zig cc -target x86_64-linux-gnu" CXX="zig c++ -target x86_64-linux-gnu"
set zigcc=SHELL=cmd.exe CC="zig cc -target x86_64-linux-gnu" CXX="zig c++ -target x86_64-linux-gnu" LD="zig c++ -target x86_64-linux-gnu" AR="zig ar"
::set zigcc=SHELL=cmd.exe CC="zig cc -target x86_64-linux-gnu" CXX="zig c++ -target x86_64-linux-gnu" LD="zig c++ -target x86_64-linux-gnu" AR="zig ar" LINKCMD="zig c++ -target x86_64-linux-gnu -shared"

@echo on
mingw32-make.exe %zigcc% clean
mingw32-make.exe %zigcc% config=%gmake_config% UDT
mingw32-make.exe %zigcc% config=%gmake_config% UDT_captures
mingw32-make.exe %zigcc% config=%gmake_config% UDT_converter
mingw32-make.exe %zigcc% config=%gmake_config% UDT_cutter
mingw32-make.exe %zigcc% config=%gmake_config% UDT_json
mingw32-make.exe %zigcc% config=%gmake_config% UDT_merger
mingw32-make.exe %zigcc% config=%gmake_config% UDT_splitter
mingw32-make.exe %zigcc% config=%gmake_config% UDT_timeshifter
pause
