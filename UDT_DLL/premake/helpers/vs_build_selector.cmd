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

echo Which architecture?
echo 1. x86
echo 2. x64
set /p choice=
if %choice%==1 (
	set vs_arch=Win32
) else if %choice%==2 (
	set vs_arch=x64
) else (
    echo Invalid choice
	exit
)