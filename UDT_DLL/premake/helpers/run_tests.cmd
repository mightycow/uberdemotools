:: argument 1 is the toolchain's folder name
@echo off
call "%~dp0pick_target.cmd"
pushd ..\.bin\%1\x64\%target%
@echo on
tests.exe
@echo off
popd
