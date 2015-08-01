#pragma once


#include "string.hpp"


namespace udtPath
{
	extern bool HasTrailingSeparator(const udtString& folderPath);
	extern bool HasValidDemoFileExtension(const udtString& filePath);
	extern bool HasValidDemoFileExtension(const char* filePath);

	extern bool Combine(udtString& combinedPath, udtVMLinearAllocator& allocator, const udtString& folderPath, const udtString& extra);
	extern bool Combine(udtString& combinedPath, udtVMLinearAllocator& allocator, const udtString& folderPath, const char* extra);

	extern bool GetFileName(udtString& fileName, udtVMLinearAllocator& allocator, const udtString& filePath);
	extern bool GetFileNameWithoutExtension(udtString& fileNameNoExt, udtVMLinearAllocator& allocator, const udtString& filePath);
	extern bool GetFilePathWithoutExtension(udtString& filePathNoExt, udtVMLinearAllocator& allocator, const udtString& filePath);
	extern bool GetFolderPath(udtString& folderPath, udtVMLinearAllocator& allocator, const udtString& filePath); // Doesn't leave a trailing separator.
	extern bool GetFileExtension(udtString& fileExtension, udtVMLinearAllocator& allocator, const udtString& filePath);
}
