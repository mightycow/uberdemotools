echo What Visual Studio version?
echo 1. Visual Studio 2017
echo 2. Visual Studio 2019
echo 3. Visual Studio 2022
echo 4. Visual Studio 2026

set /p choice=
if %choice%==1 (
	set vs_generator=vs2017
	set vs_version=15.0
) else if %choice%==2 (
	set vs_generator=vs2019
	set vs_version=16.0
) else if %choice%==3 (
	set vs_generator=vs2022
	set vs_version=17.0
) else if %choice%==4 (
	set vs_generator=vs2026
	set vs_version=18.0
) else (
	echo Invalid choice
	exit
)