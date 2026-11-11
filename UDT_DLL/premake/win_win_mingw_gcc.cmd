@echo off
setlocal

set toolchain=win_win_mingw_gcc

goto :PickAction

:Generate
premake5.exe gmake --preset=%toolchain%
goto :Exit

:Test
call "%~dp0helpers\run_tests.cmd" %toolchain% || goto :Exit
goto :Exit

:Compile

call "%~dp0helpers\pick_target.cmd" || goto :Exit
call "%~dp0helpers\pick_clean.cmd" || goto :Exit

@echo off

:: gmake_jobs = max(core_count - 2, 1)
set /a gmake_jobs=%NUMBER_OF_PROCESSORS% - 2
if %gmake_jobs% lss 1 set gmake_jobs=1

set gmake_arch=x64
set gmake_config=%gmake_target%_%gmake_arch%
set gmake_options=--jobs=%gmake_jobs% config=%gmake_config%

pushd ..\.build\%toolchain%
@echo on
if %clean%==1 (
	mingw32-make.exe clean %gmake_options%
)
mingw32-make.exe all %gmake_options%
popd
goto :Exit

:PickAction
call "%~dp0helpers\pick_action.cmd" || goto :Exit
goto :%action%

:Exit
pause
