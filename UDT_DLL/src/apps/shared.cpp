#include "shared.hpp"

#if defined(_WIN32)
#	include <Windows.h>
#endif


#if defined(_WIN32)

void ResetCurrentDirectory(const char* exeFilePath)
{
	const char* const match = strrchr(exeFilePath, '\\');
	if(match == NULL)
	{
		return;
	}

	const size_t cdLength = match - exeFilePath;
	char directoryPath[MAX_PATH];
	strncpy(directoryPath, exeFilePath, cdLength);
	directoryPath[cdLength] = '\0';

	SetCurrentDirectoryA(directoryPath);
}

void PauseConsoleApp()
{
	system("pause");
}

#else

void ResetCurrentDirectory(const char*)
{
}

void PauseConsoleApp()
{
}

#endif
