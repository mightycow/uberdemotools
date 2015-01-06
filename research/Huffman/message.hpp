#pragma once


#include "common.hpp"
#include "huffman.hpp"
#include "huffman_test.hpp"


struct idMessage
{
	qbool		allowoverflow;	// if qfalse, do a Com_Error
	qbool		overflowed;		// set to qtrue if the buffer size failed (with allowoverflow set)
	qbool		oob;
	u8*			data;
	s32			maxsize;
	s32			cursize;
	s32			readcount;
	s32			bit;			// for bitwise reads and writes
};

struct idNetField
{
	const char* name;
	s32			offset;
	s32			bits; // 0 = f32
};

struct udtMessage
{
public:
	udtMessage();

	void    PrintDecoderTree();

	void	Init(u8* data, s32 length);
	void	InitOOB(u8* data, s32 length );
	void	Clear ();
	void	WriteData (const void* data, s32 length);
	void	Bitstream();

	// TTimo
	// copy a msg_t in case we need to store it as is for a bit
	// (as I needed this to keep an msg_t from a static var for later use)
	// sets data buffer as Init does prior to do the copy
	void	Copy(u8* data, s32 length, const udtMessage* src);

	void	WriteBits(s32 value, s32 bits);
	void	WriteByte(s32 c);
	void	WriteShort(s32 c);
	void	WriteLong(s32 c);
	void	WriteString(const char* s, s32 length);
	void	WriteBigString(const char* s, s32 length);

	void	BeginReading();
	void	BeginReadingOOB();

	// Reading functions: return -1 if no more s8acters are available.
	s32		ReadBits(s32 bits);
	s32		ReadByte();
	s32		ReadShort();
	s32		ReadLong();

	char*	ReadString(s32& length);
	char*	ReadBigString(s32& length);
	char*	ReadStringLine(s32& length);
	void	ReadData(void* buffer, s32 size);

	s32		PeekByte();

public:
	idMessage Buffer;

private:
	udtHuffman _huffman;
	idHuffmanCodec _huffmanData;
	udtNewHuffmanDecoder _huffmanDecoderNew;
};