echo Clean?
echo Y: Yes
echo N: No
set /p choice=
if /i %choice%==Y (
	set clean=1
) else if /i %choice%==N (
	set clean=0
) else (
	echo Invalid choice
	exit /b 1
)
