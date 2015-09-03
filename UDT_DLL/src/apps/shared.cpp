#include "types.hpp"


#if defined(UDT_WINDOWS)
#	include "string.hpp"
#	include "thread_local_allocators.hpp"
#	include "scoped_stack_allocator.hpp"
#	include <Windows.h>
#endif

#include <stdio.h>


extern int udt_main(int argc, char** argv);
extern void CrashHandler(const char* message);


#if defined(UDT_WINDOWS)

#define UDT_MAX_ARG_COUNT 16

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
	printf("UDT library version: %s\n", udtGetVersionString());
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

int main(int argc, char** argv)
{
	printf("UDT library version: %s\n", udtGetVersionString());
	udtSetCrashHandler(&CrashHandler);
	udtInitLibrary();

	return udt_main(argc, argv);
}

#endif
