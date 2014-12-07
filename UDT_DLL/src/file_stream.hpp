#pragma once


#include "stream.hpp"

#include <stdio.h>


struct udtFileOpenMode
{
	enum Id
	{
		Read,
		Write,
		ReadWrite,
		Count
	};
};

struct udtFileStream : udtStream
{
public:
	udtFileStream();
	~udtFileStream();

	static bool Exists(const char* filePath);
	static u64  GetFileLength(const char* filePath);

	bool   Open(const char* filePath, udtFileOpenMode::Id mode);
	u32    Read(void* dstBuff, u32 elementSize, u32 count);
	u32    Write(const void* srcBuff, u32 elementSize, u32 count);
	s32	   Seek(s32 offset, udtSeekOrigin::Id origin);
	s32	   Offset();
	u64    Length();
	s32    Close();

private:
	void   Destroy();

	FILE* _file;
};
