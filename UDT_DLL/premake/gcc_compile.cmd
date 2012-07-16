cd ..\build\gmake
mingw32-make.exe clean
mingw32-make.exe config=debug32 UDT
mingw32-make.exe config=release32 UDT
pause