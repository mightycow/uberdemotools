@echo off
call "helpers/vs_version_selector.cmd"
call "helpers/vs_express_selector.cmd"
call "helpers/vs_build_selector.cmd"

@echo on
call "%vs_path%vsvars32.bat"
"%vs_path%..\IDE\%vs_exe%" ..\.build\%vs_generator%\UDT.sln /Rebuild "%vs_target%|%vs_arch%"

pause