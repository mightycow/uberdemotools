call "%VS100COMNTOOLS%vsvars32.bat"
"%VS100COMNTOOLS%..\IDE\devenv.exe" ..\build\vs2010\UDT.sln /Rebuild "Debug|Win32"
"%VS100COMNTOOLS%..\IDE\devenv.exe" ..\build\vs2010\UDT.sln /Rebuild "Release|Win32"
pause