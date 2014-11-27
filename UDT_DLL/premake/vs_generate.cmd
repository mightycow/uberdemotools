@echo off

call "helpers/vs_version_selector.cmd"
premake5.exe %vs_generator%

pause