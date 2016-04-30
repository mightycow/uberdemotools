#pragma once


#include "common.hpp"
#include "context.hpp"


struct idMessage
{
	u8*  data;
	s32  maxsize;
	s32  cursize;
	s32  readcount;
	s32  bit;   // for bitwise reads and writes
	bool valid;	// the state has become invalid, time to stop reading/writing
	bool oob;
};

struct idNetField
{
	s16 offset;
	s16 bits; // 0 = floating-point number (f32)
};

struct udtMessage
{
public:
	udtMessage();

	void  InitContext(udtContext* context);
	void  InitProtocol(udtProtocol::Id protocol);
	void  Init(u8* data, s32 length);
	void  WriteData (const void* data, s32 length);
	void  Bitstream();
	void  SetHuffman(bool huffman);
	void  GoToNextByte();
	bool  ValidState() const { return Buffer.valid; }
	void  SetFileName(const udtString& fileName) { _fileName = fileName; }

	void  WriteBits(s32 value, s32 bits) { return (this->*_writeBits)(value, bits); }
	void  WriteByte(s32 c) { WriteBits(c, 8); }
	void  WriteShort(s32 c) { WriteBits(c, 16); }
	void  WriteLong(s32 c) { WriteBits(c, 32); }
	void  WriteFloat(s32 c) { return (this->*_writeFloat)(c); }
	void  WriteField(s32 c, s32 bits) { if(bits == 0) WriteFloat(c); else WriteBits(c, bits); }
	void  WriteString(const char* s, s32 length) { (this->*_writeString)(s, length, (s32)sizeof(Context->ReadStringBuffer), Context->ReadStringBuffer); }          // The string must be null-terminated.
	void  WriteBigString(const char* s, s32 length) { (this->*_writeString)(s, length, (s32)sizeof(Context->ReadBigStringBuffer), Context->ReadBigStringBuffer); } // The string must be null-terminated.
	bool  WriteDeltaPlayer(const idPlayerStateBase* from, idPlayerStateBase* to) { return (this->*_writeDeltaPlayer)(from, to); }
	bool  WriteDeltaEntity(const idEntityStateBase* from, const idEntityStateBase* to, bool force) { return (this->*_writeDeltaEntity)(from, to, force); }

	// Functions with return type s32: -1 is returned when the state has become invalid.
	s32   ReadBits(s32 bits) { return (this->*_readBits)(bits); }
	s32   ReadBit() { return (this->*_readBit)(); }
	s32   ReadByte() { return ReadBits(8); }
	s32   ReadShort() { return ReadBits(16); }
	s32   ReadSignedShort() { return ReadBits(-16); }
	s32   ReadLong() { return ReadBits(32); }
	s32   ReadFloat() { return (this->*_readFloat)(); }
	s32   ReadField(s32 bits) { return (bits == 0) ? ReadFloat() : ReadBits(bits); } // If bits is 0, reads a float.
	char* ReadString(s32& length) { return (this->*_readString)(length, (s32)sizeof(Context->ReadStringBuffer), Context->ReadStringBuffer); }
	char* ReadBigString(s32& length) { return (this->*_readString)(length, (s32)sizeof(Context->ReadBigStringBuffer), Context->ReadBigStringBuffer); }
	void  ReadData(void* buffer, s32 size) { (this->*_readData)(buffer, size); }
	s32   PeekByte() { return (this->*_peekByte)(); }
	bool  ReadDeltaPlayer(const idPlayerStateBase* from, idPlayerStateBase* to) { return (this->*_readDeltaPlayer)(from, to); }
	bool  ReadDeltaEntity(bool& addedOrChanged, const idEntityStateBase* from, idEntityStateBase* to, s32 number) { return (this->*_readDeltaEntity)(addedOrChanged, from, to, number); }

private:
	void  ReadDeltaPlayerDM3(idPlayerStateBase* to);
	void  ReadDeltaEntityDM3(const idEntityStateBase* from, idEntityStateBase* to, s32 number);

	s32   DummyRead() { return -1; }
	s32   DummyReadCount(s32) { return -1; }
	char* DummyReadString(s32& length, s32, char* buffer) { length = 0; *buffer = '\0'; return buffer; }
	void  DummyReadData(void* buffer, s32 size) { memset(buffer, 0, (size_t)size); }
	bool  DummyReadDeltaPlayer(const idPlayerStateBase*, idPlayerStateBase*) { return false; }
	bool  DummyReadDeltaEntity(bool&, const idEntityStateBase*, idEntityStateBase*, s32) { return false; }

	void  DummyWrite(s32) {}
	void  DummyWriteCount(s32, s32) {}
	void  DummyWriteString(const char*, s32, s32, char*) {}
	bool  DummyWriteDeltaPlayer(const idPlayerStateBase*, idPlayerStateBase*) { return false; }
	bool  DummyWriteDeltaEntity(const idEntityStateBase*, const idEntityStateBase*, bool) { return false; }

	s32   RealReadBits(s32 bits);
	s32   RealReadBitNoHuffman();
	s32   RealReadBitHuffman();
	s32   RealReadFloat();
	char* RealReadString(s32& length, s32 bufferLength, char* buffer);
	void  RealReadData(void* buffer, s32 size);
	s32   RealPeekByte();
	bool  RealReadDeltaEntity(bool& addedOrChanged, const idEntityStateBase* from, idEntityStateBase* to, s32 number);
	bool  RealReadDeltaPlayer(const idPlayerStateBase* from, idPlayerStateBase* to);

	void  RealWriteBits(s32 value, s32 bits);
	void  RealWriteFloat(s32 c);
	void  RealWriteString(const char* s, s32 length, s32 bufferLength, char* buffer);
	bool  RealWriteDeltaPlayer(const idPlayerStateBase* from, idPlayerStateBase* to);
	bool  RealWriteDeltaEntity(const idEntityStateBase* from, const idEntityStateBase* to, bool force);

	void        SetValid(bool valid);
	const char* GetFileNamePtr() const { return _fileName.GetPtrSafe("N/A"); }

public:
	udtContext* Context;
	idMessage   Buffer;

private:
	typedef s32   (udtMessage::*ReadBitsFunc)(s32);
	typedef s32   (udtMessage::*ReadBitFunc)();
	typedef s32   (udtMessage::*ReadFloatFunc)();
	typedef char* (udtMessage::*ReadStringFunc)(s32&, s32, char*);
	typedef void  (udtMessage::*ReadDataFunc)(void*, s32);
	typedef s32   (udtMessage::*PeekByteFunc)();
	typedef bool  (udtMessage::*ReadDeltaEntityFunc)(bool&, const idEntityStateBase*, idEntityStateBase*, s32);
	typedef bool  (udtMessage::*ReadDeltaPlayerFunc)(const idPlayerStateBase*, idPlayerStateBase*);

	typedef void  (udtMessage::*WriteBitsFunc)(s32, s32);
	typedef void  (udtMessage::*WriteFloatFunc)(s32);
	typedef void  (udtMessage::*WriteStringFunc)(const char*, s32, s32, char*);
	typedef bool  (udtMessage::*WriteDeltaPlayerFunc)(const idPlayerStateBase*, idPlayerStateBase*);
	typedef bool  (udtMessage::*WriteDeltaEntityFunc)(const idEntityStateBase*, const idEntityStateBase*, bool);

private:
	udtProtocol::Id      _protocol;
	const idNetField*    _entityStateFields;
	s32                  _entityStateFieldCount;
	const idNetField*    _playerStateFields;
	s32                  _playerStateFieldCount;
	size_t               _protocolSizeOfEntityState;
	size_t               _protocolSizeOfPlayerState;
	udtString            _fileName;
	ReadBitsFunc         _readBits;
	ReadBitFunc          _readBit;
	ReadFloatFunc        _readFloat;
	ReadStringFunc       _readString;
	ReadDataFunc         _readData;
	PeekByteFunc         _peekByte;
	ReadDeltaEntityFunc  _readDeltaEntity;
	ReadDeltaPlayerFunc  _readDeltaPlayer;
	WriteBitsFunc        _writeBits;
	WriteFloatFunc       _writeFloat;
	WriteStringFunc      _writeString;
	WriteDeltaPlayerFunc _writeDeltaPlayer;
	WriteDeltaEntityFunc _writeDeltaEntity;
};
