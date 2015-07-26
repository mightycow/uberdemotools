#pragma once


#include "common.hpp"
#include "context.hpp"


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
	s32			bits; // 0 = floating-point number (f32)
};

struct udtMessage
{
public:
	udtMessage();

	void	InitContext(udtContext* context);
	void	InitProtocol(udtProtocol::Id protocol);
	void	Init(u8* data, s32 length);
	void	InitOOB(u8* data, s32 length );
	void	Clear ();
	void	WriteData (const void* data, s32 length);
	void	Bitstream();
	void	SetHuffman(bool huffman);

	void	GoToNextByte();

	void	WriteBits(s32 value, s32 bits);
	void	WriteByte(s32 c);
	void	WriteShort(s32 c);
	void	WriteLong(s32 c);
	void	WriteString(const char* s, s32 length);    // The string must be null-terminated.
	void	WriteBigString(const char* s, s32 length); // The string must be null-terminated.

	void	BeginReading();
	void	BeginReadingOOB();

	// Reading functions: return -1 if no more characters are available.
	s32		ReadBits(s32 bits);
	s32		ReadByte();
	s32		ReadShort();
	s32		ReadLong();
	f32		ReadFloat();

	char*	ReadString(s32& length);
	char*	ReadBigString(s32& length);
	char*	ReadStringLine(s32& length);
	void	ReadData(void* buffer, s32 size);

	s32		PeekByte();

	// User commands always have time, then either all fields or none.
	void	WriteDeltaUsercmdKey(s32 key, const usercmd_t* from, usercmd_t* to);
	void	ReadDeltaUsercmdKey(s32 key, const usercmd_t* from, usercmd_t* to);

	void	WriteDeltaPlayerstate(const idPlayerStateBase* from, idPlayerStateBase* to);
	void	ReadDeltaPlayerstate(const idPlayerStateBase* from, idPlayerStateBase* to);

	void	WriteDeltaEntity(const idEntityStateBase* from, const idEntityStateBase* to, qbool force);
	bool	ReadDeltaEntity(const idEntityStateBase* from, idEntityStateBase* to, s32 number); // True if entity was added or changed.

private:
	void	WriteDeltaKey(s32 key, s32 oldV, s32 newV, s32 bits);
	s32		ReadDeltaKey(s32 key, s32 oldV, s32 bits);
	void	ReadDeltaPlayerstateDM3(idPlayerStateBase* to);

public:
	udtContext*		Context;
	idMessage		Buffer;

private:
	udtProtocol::Id		_protocol;
	const idNetField*	_entityStateFields;
	s32					_entityStateFieldCount;
	const idNetField*	_playerStateFields;
	s32					_playerStateFieldCount;
	s32					_protocolSizeOfEntityState;
};
