call "%VS90COMNTOOLS%vsvars32.bat"
"%VS90COMNTOOLS%..\IDE\devenv.exe" ..\build\vs2008\U2DDV.sln /Rebuild "Debug|Win32"
"%VS90COMNTOOLS%..\IDE\devenv.exe" ..\build\vs2008\U2DDV.sln /Rebuild "Release|Win32"
pause