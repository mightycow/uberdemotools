call "%VS90COMNTOOLS%vsvars32.bat"
"%VS90COMNTOOLS%..\IDE\VCExpress.exe" ..\build\vs2008\UDT.sln /Rebuild "Debug|Win32"
"%VS90COMNTOOLS%..\IDE\VCExpress.exe" ..\build\vs2008\UDT.sln /Rebuild "Release|Win32"
pause