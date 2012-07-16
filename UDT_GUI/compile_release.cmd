call "%VS100COMNTOOLS%vsvars32.bat"
"%VS100COMNTOOLS%..\IDE\devenv.exe" UDT.sln /Rebuild "Release"
pause