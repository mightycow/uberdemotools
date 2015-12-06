echo What Visual Studio version?
echo 1. Visual Studio 2005
echo 2. Visual Studio 2008
echo 3. Visual Studio 2010
echo 4. Visual Studio 2012
echo 5. Visual Studio 2013
echo 6. Visual Studio 2015
set /p choice=
if %choice%==1 (
	set vs_generator=vs2005
	set "vs_path=%VS80COMNTOOLS%"
	set vs_version=8.0
) else if %choice%==2 (
	set vs_generator=vs2008
	set "vs_path=%VS90COMNTOOLS%"
	set vs_version=9.0
) else if %choice%==3 (
	set vs_generator=vs2010
	set "vs_path=%VS100COMNTOOLS%"
	set vs_version=10.0
) else if %choice%==4 (
	set vs_generator=vs2012
	set "vs_path=%VS110COMNTOOLS%"
	set vs_version=11.0
) else if %choice%==5 (
	set vs_generator=vs2013
	set "vs_path=%VS120COMNTOOLS%"
	set vs_version=12.0
) else if %choice%==6 (
	set vs_generator=vs2015
	set "vs_path=%VS140COMNTOOLS%"
	set vs_version=14.0
) else (
    echo Invalid choice
	exit
)