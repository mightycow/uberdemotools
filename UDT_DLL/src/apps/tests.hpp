#pragma once

#include "uberdemotools.h"
#include "array.hpp"

struct TestContext
{
	char RepoDir[512];
	char InTempDir[512];
	char OutTempDir[512];
};

extern TestContext g_testContext;

typedef bool (*udtTestFunction)();

void RegisterTest(udtTestFunction test, const char* section, const char* name);

struct TestRegisterer
{
	TestRegisterer(udtTestFunction test, const char* section, const char* name)
	{
		RegisterTest(test, section, name);
	}
};

// #define A 42
// #define B 69
// UDT_CONCAT_NE(A, B) -> AB
// UDT_CONCAT(A, B)    -> 4269
#define UDT_CONCAT_NE(x, y)    x ## y                 // no   expansion
#define UDT_CONCAT(x, y)       UDT_CONCAT_NE(x, y)    // with expansion

#define UDT_ENSURE(Condition) \
	if(!(Condition)) \
	{ \
		fprintf(stderr, "    false: '%s' at line %d\n", #Condition, __LINE__); \
		return false; \
	}

#define UDT_TEST_FILE(FileTitle) \
	static const char* s_fileTitle = FileTitle

#define UDT_TEST(TestTitle) \
	static bool UDT_CONCAT(TestFunction_, __LINE__)(); \
	static TestRegisterer UDT_CONCAT(g_registerer_, __LINE__)(UDT_CONCAT(&TestFunction_, __LINE__), s_fileTitle, TestTitle); \
	static bool UDT_CONCAT(TestFunction_, __LINE__)()

void Pause();
void InitContext(const char* repoPath);
void MakeDirectoryEmpty(const char* dirPath);
