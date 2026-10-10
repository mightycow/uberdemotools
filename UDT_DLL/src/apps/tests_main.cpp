#include "tests.hpp"
#include "string.hpp"
#include "file_system.hpp"
#include <stdio.h>
#include <exception>
#if defined(UDT_WINDOWS)
#include <Windows.h>
#endif

struct Test
{
	udtTestFunction function;
	const char* section;
	const char* name;
	bool passed;
};

struct TestGlobals
{
	Test tests[1024];
	int testCount;
	int passCount;
};

TestContext ctx;
static TestGlobals s_tests;

void RegisterTest(udtTestFunction function, const char* section, const char* name)
{
	assert((size_t)s_tests.testCount < UDT_COUNT_OF(s_tests.tests));
	Test& test = s_tests.tests[s_tests.testCount++];
	test.function = function;
	test.section = section;
	test.name = name;
	test.passed = false;
}

static int CompareTests(const void* aPtr, const void* bPtr)
{
	const Test* const a = (const Test*)aPtr;
	const Test* const b = (const Test*)bPtr;

	const int s = strcmp(a->section, b->section);
	if(s != 0)
	{
		return s;
	}

	const int n = strcmp(a->name, b->name);

	return n;
}

#if defined(UDT_WINDOWS)
#if defined(UDT_MINGWIN)
extern "C"
#endif
int wmain(int argc, wchar_t** argvWide)
#else
int main(int argc, char** argv)
#endif
{
	if(argc == 2)
	{
#if defined(UDT_WINDOWS)
		char repoPath[1024];
		char* utf8String = repoPath;
		wchar_t* const utf16String = argvWide[1];
		if(!WideCharToMultiByte(CP_UTF8, 0, utf16String, -1, utf8String, (int)UDT_COUNT_OF(repoPath) - 1, nullptr, nullptr))
		{
			utf8String = NULL;
		}
		InitContext(utf8String);
#else
		InitContext(argv[1]);
#endif
	}
	else
	{
		InitContext(NULL);
	}

	udtInitLibrary();
	if(!IsValidDirectory(ctx.InTempDir.Path))
	{
		return 666;
	}
	if(!IsValidDirectory(ctx.OutTempDir.Path))
	{
		return 666;
	}
	udtShutDownLibrary();

	printf("%d test%s total\n", s_tests.testCount, s_tests.testCount > 1 ? "s" : "");

	qsort(&s_tests.tests[0], (size_t)s_tests.testCount, sizeof(s_tests.tests[0]), &CompareTests);

	for(int i = 0; i < s_tests.testCount; i++)
	{
		MakeDirectoryEmpty(ctx.OutTempDir.Path);
		MakeDirectoryEmpty(ctx.InTempDir.Path);
		ctx.OutTempDir.ListFiles();
		ctx.InTempDir.ListFiles();
		ctx.TempAllocator.Clear();

		Test& test = s_tests.tests[i];
		printf("%03d. %s/%s\n", i + 1, test.section, test.name);
		bool passed = false;
		try
		{
			passed = (*test.function)();
		}
		catch(std::exception& e)
		{
			printf("%03d. Crash: %s\n", i + 1, e.what());
		}
		catch(...)
		{
			printf("%03d. Crash: unknown\n", i + 1);
		}
		if(passed)
		{
			printf("%03d. OK\n", i + 1);
			s_tests.passCount++;
		}
		else
		{
			printf("%03d. failed\n", i + 1);
		}
		test.passed = passed;
	}

	printf("\n");
	if(s_tests.passCount == s_tests.testCount)
	{
		printf("Flawless victory!\n");
	}
	else
	{
		const int failCount = s_tests.testCount - s_tests.passCount;
		printf("%d test%s failed:\n", failCount, failCount > 1 ? "s" : "");
		for(int i = 0; i < s_tests.testCount; i++)
		{
			const Test& test = s_tests.tests[i];
			if(!test.passed)
			{
				printf("%03d. %s/%s\n", i + 1, test.section, test.name);
			}
		}
	}

	MakeDirectoryEmpty(ctx.OutTempDir.Path);
	MakeDirectoryEmpty(ctx.InTempDir.Path);

	printf("\n");
	Pause();

	return 0;
}
