echo Which target?
echo 1. Release
echo 2. Debug
set /p choice=
if %choice%==1 (
	set vs_target=Release
) else if %choice%==2 (
	set vs_target=Debug
) else (
	echo Invalid choice
	exit
)

set vs_arch=x64