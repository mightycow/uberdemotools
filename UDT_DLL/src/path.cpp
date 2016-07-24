#include "path.hpp"


namespace udtPath
{
	static const char* GetSeparator()
	{
#if defined(UDT_WINDOWS)
		return "\\";
#else
		return "/";
#endif
	}
}

bool udtPath::HasTrailingSeparator(const udtString& folderPath)
{
#if defined(UDT_WINDOWS)
	return udtString::EndsWith(folderPath, "\\") || udtString::EndsWith(folderPath, "/");
#else
	return udtString::EndsWith(folderPath, "/");
#endif
}

bool udtPath::HasValidDemoFileExtension(const udtString& filePath)
{
	for(u32 i = 0; i < (u32)udtProtocol::Count; ++i)
	{
		const char* const extension = udtGetFileExtensionByProtocol((udtProtocol::Id)i);
		if(udtString::EndsWithNoCase(filePath, extension))
		{
			return true;
		}
	}

	return false;
}

bool udtPath::HasValidDemoFileExtension(const char* filePath)
{
	return HasValidDemoFileExtension(udtString::NewConstRef(filePath));
}

bool udtPath::Combine(udtString& combinedPath, udtVMLinearAllocator& allocator, const udtString& folderPath, const udtString& extra)
{
	const bool isSeparatorNeeded = !HasTrailingSeparator(folderPath);
	const udtString path = udtString::IsNullOrEmpty(folderPath) ? udtString::NewConstRef(".") : folderPath;
	const udtString separator = isSeparatorNeeded ? udtString::NewConstRef(GetSeparator()) : udtString::NewEmptyConstant();
	const udtString* strings[] =
	{
		&path,
		&separator,
		&extra
	};

	combinedPath = udtString::NewFromConcatenatingMultiple(allocator, strings, (u32)UDT_COUNT_OF(strings));

	return true;
}

bool udtPath::Combine(udtString& combinedPath, udtVMLinearAllocator& allocator, const udtString& folderPath, const char* extra)
{
	return Combine(combinedPath, allocator, folderPath, udtString::NewConstRef(extra));
}

bool udtPath::GetFileName(udtString& fileName, udtVMLinearAllocator& allocator, const udtString& filePath)
{
	u32 fileNameIndex = 0;
	if(udtString::FindLastCharacterListMatch(fileNameIndex, filePath, udtString::NewConstRef("/\\")))
	{
		fileNameIndex += 1;
	}

	fileName = udtString::NewSubstringClone(allocator, filePath, fileNameIndex);

	return true;
}

bool udtPath::GetFileNameWithoutExtension(udtString& fileNameNoExt, udtVMLinearAllocator& allocator, const udtString& filePath)
{
	u32 fileNameIndex = 0;
	if(udtString::FindLastCharacterListMatch(fileNameIndex, filePath, udtString::NewConstRef("/\\")))
	{
		fileNameIndex += 1;
	}

	u32 dotIndex = 0; // Relative to file name!
	const udtString fileNameRef = udtString::NewSubstringRef(filePath, fileNameIndex);
	if(!udtString::FindLastCharacterMatch(dotIndex, fileNameRef, '.'))
	{
		fileNameNoExt = udtString::NewSubstringClone(allocator, filePath, fileNameIndex);
		return true;
	}

	fileNameNoExt = udtString::NewSubstringClone(allocator, filePath, fileNameIndex, dotIndex);

	return true;
}

bool udtPath::GetFilePathWithoutExtension(udtString& filePathNoExt, udtVMLinearAllocator& allocator, const udtString& filePath)
{
	u32 dotIndex = 0;
	if(!udtString::FindLastCharacterMatch(dotIndex, filePath, '.'))
	{
		filePathNoExt = udtString::NewSubstringClone(allocator, filePath, 0);
		return true;
	}

	filePathNoExt = udtString::NewSubstringClone(allocator, filePath, 0, dotIndex);

	return true;
}

bool udtPath::GetFolderPath(udtString& folderPath, udtVMLinearAllocator& allocator, const udtString& filePath)
{
	u32 lastSeparatorIndex = (u32)-1;
	if(!udtString::FindLastCharacterListMatch(lastSeparatorIndex, filePath, udtString::NewConstRef("/\\")))
	{
		folderPath = udtString::NewEmptyConstant();
		return true;
	}

	folderPath = udtString::NewSubstringClone(allocator, filePath, 0, lastSeparatorIndex);

	return true;
}

bool udtPath::GetFileExtension(udtString& fileExtension, udtVMLinearAllocator& allocator, const udtString& filePath)
{
	u32 dotIndex = 0;
	if(!udtString::FindLastCharacterMatch(dotIndex, filePath, '.'))
	{
		fileExtension = udtString::NewEmptyConstant();
		return true;
	}

	fileExtension = udtString::NewSubstringClone(allocator, filePath, dotIndex + 1);

	return true;
}
