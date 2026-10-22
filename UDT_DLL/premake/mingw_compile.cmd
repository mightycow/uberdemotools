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
	mingw32-make.exe clean %gmake_options%
)
mingw32-make.exe all %gmake_options%
popd
pause
