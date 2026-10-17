#include "file_system.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"
#include "path.hpp"
#include "thread_local_allocators.hpp"


#if defined(_WIN32)


#include <Windows.h>


bool IsValidDirectory(const char* folderPath)
{
	udtVMLinearAllocator& allocator = udtThreadLocalAllocators::GetTempAllocator();
	udtVMScopedStackAllocator allocatorScope(allocator);

	wchar_t* const wideFolderPath = udtString::ConvertToUTF16(allocator, udtString::NewConstRef(folderPath));
	const DWORD attribs = GetFileAttributesW(wideFolderPath);

	return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

bool GetDirectoryFileList(udtFileListQuery& query)
{
	udtVMArray<udtString> folders("FileListQuery::FoldersArray");
	folders.Add(query.FolderPath);

	int errorCount = 0;
	while(!folders.IsEmpty())
	{
		const udtString folderPath = folders[folders.GetSize() - 1];
		folders.Remove(folders.GetSize() - 1);

		udtString queryPath;
		if(!udtPath::Combine(queryPath, query.TempAllocator, folderPath, "*"))
		{
			errorCount++;
			continue;
		}

		wchar_t* const wideQueryPath = udtString::ConvertToUTF16(query.TempAllocator, queryPath);
		WIN32_FIND_DATAW findData;
		const HANDLE findHandle = FindFirstFileW(wideQueryPath, &findData);
		if(findHandle == INVALID_HANDLE_VALUE)
		{
			errorCount++;
			continue;
		}

		do
		{
			udtString fileName = udtString::NewFromUTF16(query.TempAllocator, findData.cFileName);
			if(udtString::StartsWith(fileName, "."))
			{
				continue;
			}

			if((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				if(query.Recursive)
				{
					udtString subFolderPath;
					if(!udtPath::Combine(subFolderPath, query.TempAllocator, folderPath, fileName))
					{
						errorCount++;
						continue;
					}
					folders.Add(subFolderPath);
				}
				continue;
			}

			const u64 fileSize = (u64)findData.nFileSizeLow + ((u64)findData.nFileSizeHigh << 32);
			if(query.FileFilter != NULL && !(*query.FileFilter)(fileName.GetPtr(), fileSize, query.UserData))
			{
				continue;
			}

			udtString filePath;
			if(!udtPath::Combine(filePath, query.TempAllocator, folderPath, fileName))
			{
				errorCount++;
				continue;
			}

			udtFileInfo info;
			info.Name = udtString::NewCloneFromRef(query.PersistAllocator, fileName);
			info.Path = udtString::NewCloneFromRef(query.PersistAllocator, filePath);
			info.Size = fileSize;
			query.Files.Add(info);
		}
		while(FindNextFileW(findHandle, &findData) != 0);

		FindClose(findHandle);
	}

	return errorCount == 0;
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

bool GetDirectoryFileList(udtFileListQuery& query)
{
	udtVMArray<udtString> folders("FileListQuery::FoldersArray");
	folders.Add(query.FolderPath);

	int errorCount = 0;
	while(!folders.IsEmpty())
	{
		const udtString folderPath = folders[folders.GetSize() - 1];
		folders.Remove(folders.GetSize() - 1);

		DIR* const dirHandle = opendir(folderPath.GetPtr());
		if(dirHandle == NULL)
		{
			errorCount++;
			continue;
		}

		struct dirent* dirEntry;
		while((dirEntry = readdir(dirHandle)) != NULL)
		{
			if(udtString::StartsWith(udtString::NewConstRef(dirEntry->d_name), "."))
			{
				continue;
			}

			if((dirEntry->d_type & DT_DIR) != 0)
			{
				if(query.Recursive)
				{
					udtString subFolderPath;
					if(!udtPath::Combine(subFolderPath, query.TempAllocator, folderPath, dirEntry->d_name))
					{
						errorCount++;
						continue;
					}
					folders.Add(subFolderPath);
				}
				continue;
			}

			if((dirEntry->d_type & DT_REG) == 0)
			{
				// Not a regular file.
				continue;
			}

			udtString filePath;
			if(!udtPath::Combine(filePath, query.TempAllocator, folderPath, dirEntry->d_name))
			{
				errorCount++;
				continue;
			}

			const u64 fileSize = udtFileStream::GetFileLength(filePath.GetPtr());
			if(query.FileFilter != NULL && !(*query.FileFilter)(dirEntry->d_name, fileSize, query.UserData))
			{
				continue;
			}

			udtFileInfo info;
			info.Name = udtString::NewClone(query.PersistAllocator, dirEntry->d_name);
			info.Path = udtString::NewCloneFromRef(query.PersistAllocator, filePath);
			info.Size = fileSize;
			query.Files.Add(info);
		}

		closedir(dirHandle);
	}

	return errorCount == 0;
}


#endif
