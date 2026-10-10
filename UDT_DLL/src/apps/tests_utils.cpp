#include "tests.hpp"
#include "common.hpp"
#include <stdio.h>
#include <stdarg.h>

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

const char* RunAndCaptureOutput(const char* format, ...)
{
	char cmd[4096];
	va_list argList;
	va_start(argList, format);
	vsprintf(cmd, format, argList);
	va_end(argList);

	FILE* const pipe = _popen(cmd, "r");
	if(pipe == nullptr)
	{
		return NULL;
	}

	// @NOTE: we don't use alloc.AllocateAndGetAddress
	// because the allocator forces some alignment constraints that would break up the string.
	udtVMLinearAllocator& alloc = ctx.TempAllocator;
	const uptr startOffset = alloc.GetCurrentByteCount();
	uptr writeOffset = startOffset;
	while(fgets(cmd, sizeof(cmd), pipe) != nullptr)
	{
		const uptr size = (uptr)strlen(cmd);
		alloc.Allocate(size);
		u8* const dest = alloc.GetAddressAt(writeOffset);
		memcpy(dest, cmd, (size_t)size);
		writeOffset += size;
	}
	_pclose(pipe);
	alloc.Allocate(1);
	alloc.GetAddressAt(writeOffset)[0] = '\0';
	const char* const stdOut = (const char*)(alloc.GetStartAddress() + startOffset);

	return stdOut;
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

bool RunAndCaptureOutput(const char* format, ...)
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
	ctx.TempAllocator.Init(1 << 20);
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
