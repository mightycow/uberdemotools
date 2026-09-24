echo What Visual Studio version?
echo 1. Visual Studio 2010
echo 2. Visual Studio 2012
echo 3. Visual Studio 2013
echo 4. Visual Studio 2015
echo 5. Visual Studio 2017
echo 6. Visual Studio 2019
echo 7. Visual Studio 2022
echo 8. Visual Studio 2026

set /p choice=
if %choice%==1 (
	set vs_generator=vs2010
	set vs_version=10.0
) else if %choice%==2 (
	set vs_generator=vs2012
	set vs_version=11.0
) else if %choice%==3 (
	set vs_generator=vs2013
	set vs_version=12.0
) else if %choice%==4 (
	set vs_generator=vs2015
	set vs_version=14.0
) else if %choice%==5 (
	set vs_generator=vs2017
	set vs_version=15.0
) else if %choice%==6 (
	set vs_generator=vs2019
	set vs_version=16.0
) else if %choice%==7 (
	set vs_generator=vs2022
	set vs_version=17.0
) else if %choice%==8 (
	set vs_generator=vs2026
	set vs_version=18.0
) else (
	echo Invalid choice
	exit
)