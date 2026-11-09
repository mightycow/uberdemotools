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
	set "ninja_verbose= --verbose"
) else if %choice%==n (
	set "ninja_verbose="
) else (
	echo Invalid choice
	exit /b 1
)

set "ninja=ninja%ninja_verbose%"

PATH=%PATH%;C:\Programs\ninja

pushd ..\.build\win2lin_zig
@echo on
if %ninja_clean%==1 (
	:: @NOTE: clean is not a target but a command
	ninja -t clean
)
%ninja% %ninja_target%
popd

pause
