#include "tests.hpp"
#include "common.hpp"
#include <stdio.h>
#include <stdarg.h>

#if defined(UDT_WINDOWS)
#define udt_popen _popen
#define udt_pclose _pclose
#else
#define udt_popen popen
#define udt_pclose pclose
#endif

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

bool IsDebuggerAttached()
{
	return IsDebuggerPresent();
}

#else

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

static void CreateTempDir(char* dir, const char* name)
{
	const char* tempDir = getenv("TMPDIR");
	if(tempDir == NULL)
	{
		tempDir = "/tmp";
	}

	sprintf(dir, "%s/%s", tempDir, name);
	mkdir(dir, S_IRWXU);
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
	char cmd[1024];
	sprintf(cmd, "rm -rf %s/*", dirPath);
	system(cmd);
}

#endif

const char* fmt(const char* msg, ...)
{
	const int MaxBuffers = 16;
	const int BufferSize = 1024;
	struct Buffer { char buffer[BufferSize]; };
	static thread_local Buffer g_fmtBuffers[MaxBuffers];
	static thread_local int g_fmtBufferIndex = 0;

	g_fmtBufferIndex = (g_fmtBufferIndex + 1) % MaxBuffers;
	Buffer& buffer = g_fmtBuffers[g_fmtBufferIndex];

	va_list argList;
	va_start(argList, msg);
	vsnprintf(buffer.buffer, BufferSize - 1, msg, argList);
	va_end(argList);
	buffer.buffer[BufferSize - 1] = '\0';

	return buffer.buffer;
}

void Pause()
{
	printf("Press any key to continue . . .\n");
	(void)getchar();
}

const char* RunAndCaptureOutput(const char* exeName, const char* argsFormat, ...)
{
	char args[4096];
	char cmd[4096];
	va_list argList;
	va_start(argList, argsFormat);
	vsprintf(args, argsFormat, argList);
	va_end(argList);
#if defined(_WIN32)
	sprintf(cmd, "%s.exe %s", exeName, args);
#else
	sprintf(cmd, "./%s %s", exeName, args);
#endif

	FILE* const pipe = udt_popen(cmd, "r");
	if(pipe == NULL)
	{
		return NULL;
	}

	// @NOTE: we don't use alloc.AllocateAndGetAddress
	// because the allocator forces some alignment constraints that would break up the string.
	udtVMLinearAllocator& alloc = ctx.TempAllocator;
	const uptr startOffset = alloc.GetCurrentByteCount();
	uptr writeOffset = startOffset;
	while(fgets(cmd, sizeof(cmd), pipe) != NULL)
	{
		const uptr size = (uptr)strlen(cmd);
		alloc.Allocate(size);
		u8* const dest = alloc.GetAddressAt(writeOffset);
		memcpy(dest, cmd, (size_t)size);
		writeOffset += size;
	}
	udt_pclose(pipe);
	alloc.Allocate(1);
	alloc.GetAddressAt(writeOffset)[0] = '\0';
	const char* const stdOut = (const char*)(alloc.GetStartAddress() + startOffset);

	return stdOut;
}

bool AreFilesEqual(const char* filePathA, const char* filePathB)
{
	static u8 bufferA[4096];
	static u8 bufferB[4096];
	bool success = false;
	FILE* fileA = NULL;
	FILE* fileB = NULL;
	size_t readA = 0;
	size_t readB = 0;

	fileA = fopen(filePathA, "rb");
	if(fileA == NULL)
	{
		goto clean_up;
	}

	fileB = fopen(filePathB, "rb");
	if(fileB == NULL)
	{
		goto clean_up;
	}

	for(;;)
	{
		readA = fread(bufferA, 1, sizeof(bufferA), fileA);
		readB = fread(bufferB, 1, sizeof(bufferB), fileB);
		if(readA != readB)
		{
			goto clean_up;
		}
		if(readA > 0 && memcmp(bufferA, bufferB, readA) != 0)
		{
			goto clean_up;
		}
		if(readA < sizeof(bufferA))
		{
			success = true;
			break;
		}
	}

clean_up:
	if(fileA != NULL)
	{
		fclose(fileA);
	}
	if(fileB != NULL)
	{
		fclose(fileB);
	}

	return success;
}

static int CompareFiles(const void* aPtr, const void* bPtr)
{
	const udtFileInfo& a = *(udtFileInfo*)aPtr;
	const udtFileInfo& b = *(udtFileInfo*)bPtr;

	return strcmp(a.Name.GetPtrOrEmpty(), b.Name.GetPtrOrEmpty());
}

bool AreDirectoriesEqual(const char* dirPathA, const char* dirPathB)
{
	udtFileListQuery queryA;
	queryA.FileFilter = NULL;
	queryA.FolderPath = udtString::NewConstRef(dirPathA);
	queryA.Recursive = false;
	queryA.UserData = NULL;
	GetDirectoryFileList(queryA);

	udtFileListQuery queryB;
	queryB.FileFilter = NULL;
	queryB.FolderPath = udtString::NewConstRef(dirPathB);
	queryB.Recursive = false;
	queryB.UserData = NULL;
	GetDirectoryFileList(queryB);

	if(queryA.Files.GetSize() != queryB.Files.GetSize())
	{
		return false;
	}

	// we cannot trust the file system to always give us files ordered by names
	qsort(queryA.Files.GetStartAddress(), (size_t)queryA.Files.GetSize(), decltype(queryA.Files)::TypeSize, &CompareFiles);
	qsort(queryB.Files.GetStartAddress(), (size_t)queryB.Files.GetSize(), decltype(queryB.Files)::TypeSize, &CompareFiles);

	for(u32 i = 0, count = queryA.Files.GetSize(); i < count; ++i)
	{
		const udtFileInfo& fileA = queryA.Files[i];
		const udtFileInfo& fileB = queryB.Files[i];
		if(fileA.Size != fileB.Size)
		{
			return false;
		}
		if(!udtString::Equals(fileA.Name, fileB.Name))
		{
			return false;
		}
	}

	for(u32 i = 0, count = queryA.Files.GetSize(); i < count; ++i)
	{
		const udtFileInfo& fileA = queryA.Files[i];
		const udtFileInfo& fileB = queryB.Files[i];
		if(!AreFilesEqual(fileA.Path.GetPtrOrEmpty(), fileB.Path.GetPtrOrEmpty()))
		{
			return false;
		}
	}

	return true;
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
		Queries[i].UserData = NULL;
		Queries[i].Recursive = false;
		Queries[i].FolderPath = udtString::NewConstRef(Path);
	}
}

void Directory::ListFiles()
{
	Queries[WriteIndex].Clear();
	GetDirectoryFileList(Queries[WriteIndex]);
	QueriesValid[WriteIndex] = true;
#if defined(_DEBUG) && defined(UDT_DEBUG_VERBOSE)
	for(int n = 0, nc = Queries[WriteIndex].Files.GetSize(); n < nc; ++n)
	{
		printf("NOW: %s\n", Queries[WriteIndex].Files[n].Name.GetPtrOrEmpty());
	}
#endif
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
#if defined(_DEBUG) && defined(UDT_DEBUG_VERBOSE)
				printf("NEW: %s\n", newFile.Name.GetPtrOrEmpty());
#endif
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
