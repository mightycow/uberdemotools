#pragma once


#include "stream.hpp"


struct udtReadOnlySequentialFileStreamImpl;

// This is intended to be created once and then used for multiple files.
// The initialization cost would be too high if not amortized over many files.
struct udtReadOnlySequentialFileStream : udtStream
{
public:
	udtReadOnlySequentialFileStream();
	~udtReadOnlySequentialFileStream();

	bool Init();
	bool Open(const char* filePath, u32 offset = 0);

	u32  Read(void* dstBuff, u32 elementSize, u32 count) override;
	u32  Write(const void* srcBuff, u32 elementSize, u32 count) override;
	s32  Seek(s32 offset, udtSeekOrigin::Id origin) override;
	s32  Offset() override;
	u64  Length() override;
	s32  Close() override;

private:
	void RequestBlock(u32 blockIndex);
	void WaitForBlock(u32 blockIndex);
	void Destroy();

	udtReadOnlySequentialFileStreamImpl* _data;
};
