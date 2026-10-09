#include "tests.hpp"
#include "string.hpp"
#include <stdio.h>
#include <Windows.h>

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

TestContext g_testContext;
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

	printf("%d test%s total\n", s_tests.testCount, s_tests.testCount > 1 ? "s" : "");

	qsort(&s_tests.tests[0], (size_t)s_tests.testCount, sizeof(s_tests.tests[0]), &CompareTests);

	for(int i = 0; i < s_tests.testCount; i++)
	{
		Test& test = s_tests.tests[i];
		printf("%03d. %s -> %s", i + 1, test.section, test.name);
		const bool passed = (*test.function)();
		if(passed)
		{
			printf(" -> OK\n");
			s_tests.passCount++;
		}
		else
		{
			printf(" -> failed\n");
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
				printf("%03d. %s -> %s\n", i + 1, test.section, test.name);
			}
		}
	}

	printf("\n");
	Pause();

	return 0;
}
