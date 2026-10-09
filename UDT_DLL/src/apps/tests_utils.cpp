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
	char dirDoubleTerm[1024];
	sprintf(dirDoubleTerm, "%s\\*", dirPath);
	dirDoubleTerm[strlen(dirDoubleTerm) + 1] = '\0';

	SHFILEOPSTRUCTA fileOp = {};
	fileOp.wFunc = FO_DELETE;
	fileOp.pFrom = dirDoubleTerm;
	fileOp.pTo = NULL;
	fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	fileOp.fAnyOperationsAborted = FALSE,
	SHFileOperationA(&fileOp);
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
	// @TODO: validate
	char cmd[1024];
	sprintf(cmd, "rm -r %s/*", dirPath);
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
