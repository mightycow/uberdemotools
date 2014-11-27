echo What Visual Studio version?
echo 1. Visual Studio 2013
echo 2. Visual Studio 2012
echo 3. Visual Studio 2010
echo 4. Visual Studio 2008
echo 5. Visual Studio 2005
set /p choice=
if %choice%==1 (
	set vs_generator=vs2013
	set "vs_path=%VS120COMNTOOLS%"
) else if %choice%==2 (
	set vs_generator=vs2012
	set "vs_path=%VS110COMNTOOLS%"
) else if %choice%==3 (
	set vs_generator=vs2010
	set "vs_path=%VS100COMNTOOLS%"
) else if %choice%==4 (
	set vs_generator=vs2008
	set "vs_path=%VS90COMNTOOLS%"
) else if %choice%==5 (
	set vs_generator=vs2005
	set "vs_path=%VS80COMNTOOLS%"
) else (
    echo Invalid choice
	exit
)