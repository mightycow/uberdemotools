#include "uberdemotools.h"
#include "macros.hpp"
#include "string.hpp"
#include "stack_trace.hpp"
#if defined(UDT_WINDOWS)
#	include "thread_local_allocators.hpp"
#	include "scoped_stack_allocator.hpp"
#	include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>


static const char* LogLevels[4] =
{
	"",
	"Warning: ",
	"Error: ",
	"Fatal: "
};

static const char* ExecutableFileName = NULL;
static bool QuietMode = false;


extern int  udt_main(int argc, char** argv);
extern void PrintHelp();


static void CrashHandler(const char* message)
{
	fprintf(stderr, "\n%s\n", message);

#if !defined(UDT_WINDOWS)
	PrintStackTrace(stderr, 3, ExecutableFileName);
#endif

	exit(666);
}

static void ParseQuietOption(int argc, char** argv)
{
	for(int i = 1; i < argc; ++i)
	{
		if(udtString::Equals(udtString::NewConstRef(argv[i]), "-q"))
		{
			QuietMode = true;
			break;
		}
	}
}

void CallbackConsoleMessage(s32 logLevel, const char* message)
{
	if(logLevel < 0 || logLevel >= 3)
	{
		logLevel = 3;
	}

	const bool useStdOut = logLevel == 0 || logLevel == 1;
	if(QuietMode && useStdOut)
	{
		return;
	}

	fprintf(useStdOut ? stdout : stderr, "%s%s\n", LogLevels[logLevel], message);
}


#if defined(UDT_WINDOWS)

#define UDT_MAX_ARG_COUNT 16

void CallbackConsoleProgress(f32 progress, void*)
{
	char title[256];
	sprintf(title, "%.1f%% - %s", 100.0f * progress, ExecutableFileName);
	SetConsoleTitleA(title);
}

#if !defined(UDT_DONT_RESET_CD)
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
#endif

static void FindExecutableFileName(const char* exeFilePath)
{
	ExecutableFileName = exeFilePath;

	u32 slashIndex = 0;
	const udtString exeFilePathString = udtString::NewConstRef(exeFilePath);
	if(udtString::FindLastCharacterListMatch(slashIndex, exeFilePathString, udtString::NewConstRef("/\\")) &&
	   slashIndex + 1 < exeFilePathString.GetLength())
	{
		ExecutableFileName = exeFilePath + slashIndex + 1;
	}
}

#if defined(UDT_MINGWIN)
extern "C"
#endif
int wmain(int argc, wchar_t** argvWide)
{
	udtSetCrashHandler(&CrashHandler);
	udtInitLibrary();

	if(argc > UDT_MAX_ARG_COUNT)
	{
		argc = UDT_MAX_ARG_COUNT;
	}

	udtVMLinearAllocator stringAllocator("wmain::Strings");

	u32 argvOffsets[UDT_MAX_ARG_COUNT];
	for(int i = 0; i < argc; ++i)
	{
		argvOffsets[i] = udtString::NewFromUTF16(stringAllocator, argvWide[i]).GetOffset();
	}

	char* argv[UDT_MAX_ARG_COUNT];
	for(int i = 0; i < argc; ++i)
	{
		argv[i] = stringAllocator.GetWriteStringAt(argvOffsets[i]);
	}

	if(argc == 2)
	{
		if(udtString::Equals(udtString::NewConstRef(argv[1]), "/?") ||
		   udtString::Equals(udtString::NewConstRef(argv[1]), "--help"))
		{
			PrintHelp();
			return 0;
		}

		if(udtString::Equals(udtString::NewConstRef(argv[1]), "--version"))
		{
			printf("UDT library version: %s\n", udtGetVersionString());
			return 0;
		}
	}	   

#if !defined(UDT_DONT_RESET_CD)
	ResetCurrentDirectory(argv[0]);
#endif
	FindExecutableFileName(argv[0]);
	ParseQuietOption(argc, argv);

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
	   slashIndex + 1 < exeFilePathString.GetLength())
	{
		ExecutableFileName = exeFilePath + slashIndex + 1;
	}
}

int main(int argc, char** argv)
{
	if(argc == 2)
	{
		if(udtString::Equals(udtString::NewConstRef(argv[1]), "--help"))
		{
			PrintHelp();
			return 0;
		}

		if(udtString::Equals(udtString::NewConstRef(argv[1]), "--version"))
		{
			printf("UDT library version: %s\n", udtGetVersionString());
			return 0;
		}
	}

	FindExecutableFileName(argv[0]);
	udtSetCrashHandler(&CrashHandler);
	udtInitLibrary();
	ParseQuietOption(argc, argv);

	return udt_main(argc, argv);
}

#endif
