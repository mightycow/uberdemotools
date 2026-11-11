echo Verbose?
echo Y: Yes
echo N: No
set /p choice=
if /i %choice%==Y (
	set "verbose=1"
	set "ninja_verbose=--verbose"
) else if /i %choice%==N (
	set "verbose=0"
	set "ninja_verbose="
) else (
	echo Invalid choice
	exit /b 1
)
