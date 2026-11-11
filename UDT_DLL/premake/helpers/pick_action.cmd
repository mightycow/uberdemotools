echo Which action?
echo G: Generate makefiles/projects
echo C: Compile
echo T: Run tests
set /p choice=
if /i %choice%==G (
	set action=Generate
) else if /i %choice%==C (
	set action=Compile
) else if /i %choice%==T (
	set action=Test
) else (
	echo Invalid choice
	exit /b 1
)
