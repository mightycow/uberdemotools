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
	udtVMArray<udtFileInfo> Files { "FileListQuery::FilesArray" };      // Output.
	udtVMLinearAllocator PersistAllocator { "FileListQuery::Persist" }; // Output.
	udtVMLinearAllocator TempAllocator { "FileListQuery::Temp" };       // Private data.
	udtString FolderPath;        // Input. Can be modified.
	KeepFileCallback FileFilter; // Input. Can be NULL.
	void* UserData;              // Input. Can be NULL.
	bool Recursive;              // Input.
};

extern bool IsValidDirectory(const char* folderPath);
extern bool GetDirectoryFileList(udtFileListQuery& query);
