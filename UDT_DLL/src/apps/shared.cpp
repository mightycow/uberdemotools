#include "types.hpp"


#if defined(UDT_WINDOWS)
#	include "string.hpp"
#	include <Windows.h>
#endif


extern int udt_main(int argc, char** argv);


#if defined(UDT_WINDOWS)

#define UDT_MAX_ARG_COUNT 16

int wmain(int argc, wchar_t** argvWide)
{
	if(argc > UDT_MAX_ARG_COUNT)
	{
		argc = UDT_MAX_ARG_COUNT;
	}

	// @FIXME: This is a job for a thread-local allocator.
	udtVMLinearAllocator allocator;
	allocator.Init(UDT_MEMORY_PAGE_SIZE);

	char* argv[UDT_MAX_ARG_COUNT];
	for(int i = 0; i < argc; ++i)
	{
		argv[i] = udtString::NewFromUTF16(allocator, argvWide[i]).String;
	}

	return udt_main(argc, argv);
}

void ResetCurrentDirectory(const char* exeFilePath)
{
	const char* const match = strrchr(exeFilePath, '\\');
	if(match == NULL)
	{
		return;
	}

	// @FIXME: This is a job for a thread-local allocator.
	udtVMLinearAllocator allocator;
	allocator.Init(UDT_MEMORY_PAGE_SIZE);

	const udtString filePath = udtString::NewConstRef(exeFilePath);
	const udtString directoryPath = udtString::NewSubstringClone(allocator, filePath, 0, (u32)(match - exeFilePath));
	wchar_t* const wideDirectoryPath = udtString::ConvertToUTF16(allocator, directoryPath);
	SetCurrentDirectoryW(wideDirectoryPath);
}

#else

int main(int argc, char** argv)
{
	return udt_main(argc, argv);
}

void ResetCurrentDirectory(const char*)
{
}

#endif
