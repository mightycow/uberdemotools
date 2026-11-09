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

pushd ..\.build\win_zig
@echo on
if %ninja_clean%==1 (
	:: clean is not a target but a command
	:: infortunately we have to either manually clean everything separately
	:: or clean everything in both debug and release
	ninja -t clean
)
:: just the DLL: %ninja% UDT_%ninja_target
:: note the underscore, the DLL targets are "UDT_Release_x64" and "UDT_Debug_x64"
%ninja% %ninja_target%
popd

pause
