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

static void GetAbsoluteDirPath(char* dir, const char* relPath)
{
	GetFullPathNameA(relPath, 1024, dir, NULL);
}

void SetCD(const char* dirPath)
{
	SetCurrentDirectoryA(dirPath);
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
#include <unistd.h>

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

static void GetAbsoluteDirPath(char* dir, const char* relPath)
{
	realpath(relPath, dir);
}

void SetCD(const char* dirPath)
{
	chdir(dirPath);
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
	if(repoPath == NULL)
	{
		repoPath = "../../../../..";
	}
	char repoDir[1024];
	GetAbsoluteDirPath(repoDir, repoPath);
	sprintf(ctx.DemoDir, "%s/demo_files", repoDir);

	CreateTempDir(ctx.InTempDir.Path, "udt_in");
	MakeDirectoryEmpty(ctx.InTempDir.Path);
	CreateTempDir(ctx.OutTempDir.Path, "udt_out");
	MakeDirectoryEmpty(ctx.OutTempDir.Path);
	ctx.InTempDir.Init();
	ctx.OutTempDir.Init();
}

void Directory::Init()
{
	WriteIndex = 0;
	QueriesValid[0] = false;
	QueriesValid[1] = false;
	for(size_t i = 0; i < UDT_COUNT_OF(Queries); ++i)
	{
		Queries[i].FileFilter = NULL;
		Queries[i].UserData = nullptr;
		Queries[i].Recursive = false;
		Queries[i].FolderPath = udtString::NewConstRef(Path);
	}
}

void Directory::ListFiles()
{
	GetDirectoryFileList(Queries[WriteIndex]);
	QueriesValid[WriteIndex] = true;
	WriteIndex ^= 1;

	NewFiles.Clear();
	if(QueriesValid[0] && QueriesValid[1])
	{
		const udtVMArray<udtFileInfo>& oldList = Queries[WriteIndex].Files;
		const udtVMArray<udtFileInfo>& newList = Queries[WriteIndex ^ 1].Files;
		for(int n = 0, nc = newList.GetSize(); n < nc; ++n)
		{
			const udtFileInfo& newFile = newList[n];

			bool oldFound = false;
			for(int o = 0, oc = oldList.GetSize(); o < oc; ++o)
			{
				const udtFileInfo& oldFile = oldList[n];
				if(udtString::Equals(newFile.Name, oldFile.Name))
				{
					oldFound = true;
					break;
				}
			}

			if(!oldFound)
			{
				NewFiles.Add(newFile);
			}
		}
	}
}

udtVMArray<udtFileInfo>& Directory::GetFileList()
{
	return Queries[WriteIndex ^ 1].Files;
}

udtVMArray<udtFileInfo>& Directory::GetNewFiles()
{
	return NewFiles;
}
