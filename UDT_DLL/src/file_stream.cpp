#include "file_stream.hpp"


#if defined(UDT_WINDOWS)

#include "string.hpp"
#include "scoped_stack_allocator.hpp"
#include "thread_local_allocators.hpp"

static const wchar_t* const stdioFileOpenModes[udtFileOpenMode::Count] =
{
	L"rb", // Read binary, file must exist.
	L"wb", // Write binary, file created or emptied if exists.
	L"r+b" // Read/write binary, file must exist.
};

#else

static const char* const stdioFileOpenModes[udtFileOpenMode::Count] =
{
	"rb", // Read binary, file must exist.
	"wb", // Write binary, file created or emptied if exists.
	"r+b" // Read/write binary, file must exist.
};

#endif


bool udtFileStream::Exists(const char* filePath)
{
	udtFileStream file;
	
	return file.Open(filePath, udtFileOpenMode::Read);
}

u64 udtFileStream::GetFileLength(const char* filePath)
{
	udtFileStream file;
	if(!file.Open(filePath, udtFileOpenMode::Read))
	{
		return 0;
	}
	
	return file.Length();
}


udtFileStream::udtFileStream() : _file(NULL) 
{
}

udtFileStream::~udtFileStream() 
{
	Destroy(); 
}

#if defined(UDT_WINDOWS)

bool udtFileStream::Open(const char* filePath, udtFileOpenMode::Id mode)
{
	if(mode < 0 || mode >= udtFileOpenMode::Count)
	{
		return false;
	}

	udtVMLinearAllocator& allocator = udtThreadLocalAllocators::GetTempAllocator();
	udtVMScopedStackAllocator allocatorScope(allocator);
	wchar_t* const wideFilePath = udtString::ConvertToUTF16(allocator, udtString::NewConstRef(filePath));
	
	return (_file = _wfopen(wideFilePath, stdioFileOpenModes[mode])) != NULL;
}

#else

bool udtFileStream::Open(const char* filePath, udtFileOpenMode::Id mode)
{
	if(mode < 0 || mode >= udtFileOpenMode::Count)
	{
		return false;
	}

	return (_file = fopen(filePath, stdioFileOpenModes[mode])) != NULL;
}

#endif

u32 udtFileStream::Read(void* dstBuff, u32 elementSize, u32 count)
{
	return (u32)fread(dstBuff, (size_t)elementSize, (size_t)count, _file);
}

u32 udtFileStream::Write(const void* srcBuff, u32 elementSize, u32 count)
{
	return (u32)fwrite(srcBuff, (size_t)elementSize, (size_t)count, _file);
}

s32	udtFileStream::Seek(s32 offset, udtSeekOrigin::Id origin)
{
	return (s32)fseek(_file, offset, origin);
}

s32 udtFileStream::Offset()
{
	return (s32)ftell(_file);
}

u64 udtFileStream::Length()
{
	const long offset = ftell(_file);
	fseek(_file, 0, SEEK_END);
	const u64 length = (u64)ftell(_file);
	fseek(_file, offset, SEEK_SET);

	return length;
}

s32 udtFileStream::Close()
{
	if(_file)
	{
		fclose(_file);
		_file = NULL;
	}

	return 0;
}

void udtFileStream::Destroy()
{
	Close();
}
