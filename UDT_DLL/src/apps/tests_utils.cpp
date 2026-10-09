#include "tests.hpp"
#include "common.hpp"
#include <stdio.h>

#if defined(UDT_WINDOWS)

#include <Windows.h>
#include <Shlwapi.h>

static void CreateTempDir(char* dir, const char* name)
{
	char tempDir[1024];
	GetTempPathA((DWORD)sizeof(tempDir), tempDir);
	PathCombineA(dir, tempDir, name);
	CreateDirectoryA(dir, NULL);
}

void MakeDirectoryEmpty(const char* dirPath)
{
	// @TODO:
}

#else

#include <stdlib.h>

static void CreateTempDir(char* dir, const char* name)
{
	const char* tempDir = getenv("TMPDIR");
	if(tempDir == nullptr)
	{
		tempDir = "/tmp";
	}

	sprintf(dir, "%s/%s_XXXXXX", tempDir, name);
	mkdtemp(dir)
}

void MakeDirectoryEmpty(const char* dirPath)
{
	// @TODO:
}

#endif

void Pause()
{
	printf("Press any key to continue . . .\n");
	(void)getchar();
}

void InitContext(const char* repoPath)
{
	Q_strncpyz(g_testContext.RepoDir, repoPath, (s32)sizeof(g_testContext.RepoDir));
	CreateTempDir(g_testContext.InTempDir, "udt_in");
	MakeDirectoryEmpty(g_testContext.InTempDir);
	CreateTempDir(g_testContext.OutTempDir, "udt_out");
	MakeDirectoryEmpty(g_testContext.OutTempDir);
}
