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

	u32    Read(void* dstBuff, u32 elementSize, u32 count) override;
	u32    Write(const void* srcBuff, u32 elementSize, u32 count) override;
	s32	   Seek(s32 offset, udtSeekOrigin::Id origin) override;
	s32	   Offset() override;
	u64    Length() override;
	s32    Close() override;

private:
	void   Destroy();

	FILE* _file;
};
