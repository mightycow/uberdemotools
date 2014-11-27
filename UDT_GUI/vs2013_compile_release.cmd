call "%VS120COMNTOOLS%vsvars32.bat"
"%VS120COMNTOOLS%..\IDE\devenv.exe" UDT.sln /Rebuild "Release"
pause