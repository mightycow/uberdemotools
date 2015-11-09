#pragma once


#include "linear_allocator.hpp"
#include "array.hpp"


struct udtFileInfo
{
	const char* Name;
	const char* Path;
	u64 Size;
};

typedef bool(*KeepFileCallback)(const char* name, u64 size, void* userData); // Returns true if the file is to be kept.

struct udtFileListQuery
{
	udtVMArray<udtFileInfo>* Files;
	const char* FolderPath;
	udtVMLinearAllocator* PersistAllocator;
	udtVMLinearAllocator* TempAllocator;
	udtVMLinearAllocator* FolderArrayAllocator;
	KeepFileCallback FileFilter;
	void* UserData;
	bool Recursive;
};

extern bool IsValidDirectory(const char* folderPath);
extern bool GetDirectoryFileList(const udtFileListQuery& query);
