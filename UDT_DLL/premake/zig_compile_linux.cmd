@echo off

echo Which target?
echo R: Release
echo D: Debug
set /p choice=
if %choice%==r (
	set ninja_target=Release_x64
) else if %choice%==d (
	set ninja_target=Debug_x64
) else (
	echo Invalid choice
	exit /b 1
)

echo Clean?
echo Y: Yes
echo N: No
set /p choice=
if %choice%==y (
	set ninja_clean=1
) else if %choice%==n (
	set ninja_clean=0
) else (
	echo Invalid choice
	exit /b 1
)

echo Verbose?
echo Y: Yes
echo N: No
set /p choice=
if %choice%==y (
	set "ninja_verbose=--verbose"
) else if %choice%==n (
	set "ninja_verbose="
) else (
	echo Invalid choice
	exit /b 1
)

set "ninja=ninja %ninja_verbose%"

PATH=%PATH%;C:\Programs\ninja

pushd ..\.build\win2lin_zig
@echo on
if %ninja_clean%==1 (
	:: @NOTE: clean is not a target but a command
	ninja -t clean
)
%ninja% UDT_%ninja_target%
%ninja% UDT_captures_%ninja_target%
%ninja% UDT_converter_%ninja_target%
%ninja% UDT_cutter_%ninja_target%
%ninja% UDT_json_%ninja_target%
%ninja% UDT_merger_%ninja_target%
%ninja% UDT_splitter_%ninja_target%
%ninja% UDT_timeshifter_%ninja_target%
%ninja% UDT_viewer_%ninja_target%
%ninja% viewer_data_gen_%ninja_target%
%ninja% tests_%ninja_target%
popd

pause
