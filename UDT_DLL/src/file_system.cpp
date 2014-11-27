#include "file_system.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


#if defined(_WIN32)


#include <Windows.h>


bool IsValidDirectory(const char* folderPath)
{
	const DWORD attribs = GetFileAttributes(folderPath);

	return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

bool GetDirectoryFileList(const udtFileListQuery& query)
{
	if(query.Files == NULL || 
	   query.FolderPath == NULL || 
	   query.PersistAllocator == NULL || 
	   query.TempAllocator == NULL)
	{
		return false;
	}

	char* queryPath = NULL;
	if(!StringPathCombine(queryPath, *query.TempAllocator, query.FolderPath, "*"))
	{
		return false;
	}

	WIN32_FIND_DATAA findData;
	const HANDLE findHandle = FindFirstFileA(queryPath, &findData);
	if(findHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	udtVMArray<const char*> folders;
	do
	{
		if((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			if(query.Recursive && strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0)
			{
				folders.Add(AllocateString(*query.TempAllocator, findData.cFileName));
			}
			continue;
		}

		const u64 fileSize = (u64)findData.nFileSizeLow + ((u64)findData.nFileSizeHigh << 32);
		if(query.FileFilter != NULL && !(*query.FileFilter)(findData.cFileName, fileSize))
		{
			continue;
		}

		char* filePath = NULL;
		if(!StringPathCombine(filePath, *query.TempAllocator, query.FolderPath, findData.cFileName))
		{
			return false;
		}

		udtFileInfo info;
		info.Name = AllocateString(*query.PersistAllocator, findData.cFileName);
		info.Path = AllocateString(*query.PersistAllocator, filePath);
		info.Size = fileSize;
		query.Files->Add(info);
	}
	while(FindNextFile(findHandle, &findData) != 0);

	FindClose(findHandle);

	if(query.Recursive)
	{
		for(u32 i = 0; i < folders.GetSize(); ++i)
		{
			char* subFolderPath = NULL;
			if(!StringPathCombine(subFolderPath, *query.TempAllocator, query.FolderPath, folders[i]))
			{
				return false;
			}

			udtFileListQuery newQuery = query;
			newQuery.FolderPath = subFolderPath;
			if(!GetDirectoryFileList(newQuery))
			{
				return false;
			}
		}
	}

	return true;
}


#else


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>


bool IsValidDirectory(const char* folderPath)
{
	if(access(folderPath, 0) != 0)
	{
		return false;
	}

	struct stat status;
	stat(folderPath, &status);

	return (status.st_mode & S_IFDIR) != 0;
}

bool GetDirectoryFileList(const udtFileListQuery& query)
{
	if(query.Files == NULL || 
	   query.FolderPath == NULL || 
	   query.PersistAllocator == NULL || 
	   query.TempAllocator == NULL)
	{
		return false;
	}

	DIR* const dirHandle = opendir(query.FolderPath);
	if(dirHandle == NULL)
	{
		return false;
	}

	udtVMArray<const char*> folders;
	
	struct dirent* dirEntry;
	while((dirEntry = readdir(dirHandle)) != NULL)
	{
		if((dirEntry->d_type & DT_DIR) != 0)
		{
			if(query.Recursive && strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0)
			{
				folders.Add(AllocateString(*query.TempAllocator, dirEntry->d_name));
			}
			continue;
		}
		
		if((dirEntry->d_type & DT_REG) == 0)
		{
			// Not a regular file.
			continue;
		}
		
		char* filePath = NULL;
		if(!StringPathCombine(filePath, *query.TempAllocator, query.FolderPath, dirEntry->d_name))
		{
			return false;
		}
		
		const u64 fileSize = udtFileStream::GetFileLength(filePath);
		if(query.FileFilter != NULL && !(*query.FileFilter)(dirEntry->d_name, fileSize))
		{
			continue;
		}

		udtFileInfo info;
		info.Name = AllocateString(*query.PersistAllocator, dirEntry->d_name);
		info.Path = AllocateString(*query.PersistAllocator, filePath);
		info.Size = fileSize;
		query.Files->Add(info);
	}

	closedir(dirHandle);

	if(query.Recursive)
	{
		for(u32 i = 0; i < folders.GetSize(); ++i)
		{
			char* subFolderPath = NULL;
			if(!StringPathCombine(subFolderPath, *query.TempAllocator, query.FolderPath, folders[i]))
			{
				return false;
			}

			udtFileListQuery newQuery = query;
			newQuery.FolderPath = subFolderPath;
			if(!GetDirectoryFileList(newQuery))
			{
				return false;
			}
		}
	}

	return true;
}


#endif
