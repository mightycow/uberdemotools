call "%VS120COMNTOOLS%vsvars32.bat"
"%VS120COMNTOOLS%..\IDE\devenv.exe" UDT.sln /Rebuild "Release|x86"
"%VS120COMNTOOLS%..\IDE\devenv.exe" UDT.sln /Rebuild "Release|x64"
pause