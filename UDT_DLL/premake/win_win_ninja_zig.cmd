@echo off
setlocal

set toolchain=win_win_ninja_zig

goto :PickAction

:Generate
premake5.exe ninja --preset=%toolchain%
goto :Exit

:Test
call "%~dp0helpers\run_tests.cmd" %toolchain% || goto :Exit
goto :Exit

:Compile

call "%~dp0helpers\pick_target.cmd" || goto :Exit
call "%~dp0helpers\pick_clean.cmd" || goto :Exit
call "%~dp0helpers\pick_verbose.cmd" || goto :Exit

@echo off

set "ninja_args="
if %verbose%==1 set "ninja_args=%ninja_args% --verbose"

PATH=%PATH%;C:\Programs\ninja

pushd ..\.build\%toolchain%

:: clean is not a target but a command
:: infortunately we have to either manually clean everything separately
:: or clean everything in both debug and release
if %clean%==1 (
    ninja -t clean
)
:: just the DLL: %ninja% UDT_%ninja_target%
:: note the underscore, the DLL targets are "UDT_Release_x64" and "UDT_Debug_x64"
@echo on
ninja %ninja_args% %ninja_target%
@echo off
popd
goto :Exit

:PickAction
call "%~dp0helpers\pick_action.cmd" || goto :Exit
goto :%action%

:Exit
pause
