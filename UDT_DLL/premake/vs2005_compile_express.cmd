call "%VS80COMNTOOLS%vsvars32.bat"
"%VS80COMNTOOLS%..\IDE\VCExpress.exe" ..\build\vs2005\UDT.sln /Rebuild "Debug|Win32"
"%VS80COMNTOOLS%..\IDE\VCExpress.exe" ..\build\vs2005\UDT.sln /Rebuild "Release|Win32"
pause