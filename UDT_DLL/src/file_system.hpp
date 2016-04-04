#pragma once


#include "linear_allocator.hpp"
#include "array.hpp"
#include "string.hpp"


struct udtFileInfo
{
	udtString Name;
	udtString Path;
	u64 Size;
};

typedef bool (*KeepFileCallback)(const char* name, u64 size, void* userData); // Returns true if the file is to be kept.

struct udtFileListQuery
{
	void InitAllocators(u32 expectedFileCount)
	{
		Files.Init((uptr)sizeof(udtFileInfo) * (uptr)expectedFileCount, "FileListQuery::FilesArray");
		PersistAllocator.Init(32 * (uptr)expectedFileCount, "FileListQuery::Persist");
		TempAllocator.Init(UDT_KB(16), "FileListQuery::Temp");
	}

	udtVMArray<udtFileInfo> Files;         // Output.
	udtVMLinearAllocator PersistAllocator; // Output.
	udtVMLinearAllocator TempAllocator;    // Private data.
	udtString FolderPath;                  // Input. Can be modified.
	KeepFileCallback FileFilter;           // Input. Can be NULL.
	void* UserData;                        // Input. Can be NULL.
	bool Recursive;                        // Input.
};

extern bool IsValidDirectory(const char* folderPath);
extern bool GetDirectoryFileList(udtFileListQuery& query);
