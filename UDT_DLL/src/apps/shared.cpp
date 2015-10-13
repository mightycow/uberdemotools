#include "api.h"
#include "string.hpp"
#if defined(UDT_WINDOWS)
#	include "thread_local_allocators.hpp"
#	include "scoped_stack_allocator.hpp"
#	include <Windows.h>
#endif

#include <stdio.h>


static const char* LogLevels[4] =
{
	"",
	"Warning: ",
	"Error: ",
	"Fatal: "
};

#if !defined(UDT_WINDOWS)
static const char* ExecutableFileName = NULL;
#endif


extern int udt_main(int argc, char** argv);


static void CrashHandler(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, message);
	fprintf(stderr, "\n");

#if !defined(UDT_WINDOWS)
	PrintStackTrace(stderr, 3, ExecutableFileName);
#endif

	exit(666);
}

void CallbackConsoleMessage(s32 logLevel, const char* message)
{
	if(logLevel < 0 || logLevel >= 3)
	{
		logLevel = 3;
	}

	FILE* const file = (logLevel == 0 || logLevel == 1) ? stdout : stderr;
	fprintf(file, LogLevels[logLevel]);
	fprintf(file, message);
	fprintf(file, "\n");
}


#if defined(UDT_WINDOWS)

#define UDT_MAX_ARG_COUNT 16

void CallbackConsoleProgress(f32 progress, void*)
{
	char title[256];
	sprintf(title, "%.1f%% - UDT", 100.0f * progress);
	SetConsoleTitleA(title);
}

static void ResetCurrentDirectory(const char* exeFilePath)
{
	const char* const match = strrchr(exeFilePath, '\\');
	if(match == NULL)
	{
		return;
	}

	udtVMLinearAllocator& allocator = udtThreadLocalAllocators::GetTempAllocator();
	udtVMScopedStackAllocator allocatorScope(allocator);

	const udtString filePath = udtString::NewConstRef(exeFilePath);
	const udtString directoryPath = udtString::NewSubstringClone(allocator, filePath, 0, (u32)(match - exeFilePath));
	wchar_t* const wideDirectoryPath = udtString::ConvertToUTF16(allocator, directoryPath);
	SetCurrentDirectoryW(wideDirectoryPath);
}

int wmain(int argc, wchar_t** argvWide)
{
	udtSetCrashHandler(&CrashHandler);
	udtInitLibrary();

	if(argc > UDT_MAX_ARG_COUNT)
	{
		argc = UDT_MAX_ARG_COUNT;
	}

	udtVMLinearAllocator& allocator = udtThreadLocalAllocators::GetTempAllocator();
	udtVMScopedStackAllocator allocatorScope(allocator);

	char* argv[UDT_MAX_ARG_COUNT];
	for(int i = 0; i < argc; ++i)
	{
		argv[i] = udtString::NewFromUTF16(allocator, argvWide[i]).String;
	}

	ResetCurrentDirectory(argv[0]);

	return udt_main(argc, argv);
}

#else

void CallbackConsoleProgress(f32, void*)
{
}

static void FindExecutableFileName(const char* exeFilePath)
{
	ExecutableFileName = exeFilePath;

	u32 slashIndex = 0;
	const udtString exeFilePathString = udtString::NewConstRef(exeFilePath);
	if(udtString::FindLastCharacterMatch(slashIndex, exeFilePathString, '/') &&
	   slashIndex + 1 < exeFilePathString.Length)
	{
		ExecutableFileName = exeFilePathString.String + slashIndex + 1;
	}
}

int main(int argc, char** argv)
{
	FindExecutableFileName(argv[0]);
	udtSetCrashHandler(&CrashHandler);
	udtInitLibrary();

	return udt_main(argc, argv);
}

#endif
