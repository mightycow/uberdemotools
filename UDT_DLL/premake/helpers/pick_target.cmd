echo Which target?
echo R: Release
echo D: Debug
set /p choice=
if /i %choice%==R (
	set target=Release
	set ninja_target=Release_x64
	set gmake_target=release
) else if /i %choice%==D (
	set target=Debug
	set ninja_target=Debug_x64
	set gmake_target=debug
) else (
	echo Invalid choice
	exit /b 1
)
