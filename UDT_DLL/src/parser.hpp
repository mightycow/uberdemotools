#pragma once


#include "context.hpp"
#include "message.hpp"
#include "tokenizer.hpp"
#include "file_stream.hpp"
#include "linear_allocator.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"

// For the placement new operator.
#include <new>


#define UDT_BASE_PARSER_ALLOCATOR_LIST(N) \
	N(TempAllocator, 1 << 20) \
	N(PersistentAllocator, 1 << 24) \
	N(PlugInsArray, 1 << 16) \
	N(GameStateFileOffsetsArray, 1 << 16) \
	N(ConfigStringsArray, 2 * MAX_CONFIGSTRINGS * sizeof(udtBaseParser::udtConfigString)) \
	N(ChangedEntitiesArray, 1 << 16) \
	N(RemovedEntitiesArray, 1 << 16) \
	N(CutsArray, 1 << 16)

#define UDT_BASE_PARSER_ALLOCATOR_ITEM(Enum, Bytes) Enum,
struct udtBaseParserAllocator
{
	enum Id
	{
		UDT_BASE_PARSER_ALLOCATOR_LIST(UDT_BASE_PARSER_ALLOCATOR_ITEM)
		Count
	};
};
#undef UDT_BASE_PARSER_ALLOCATOR_ITEM

#define UDT_BASE_PARSER_FIXED_SIZE_ARRAY_LIST(N) \
	N(EntityBaselines, MAX_PARSE_ENTITIES * sizeof(idLargestEntityState)) \
	N(Entities, MAX_PARSE_ENTITIES * sizeof(idLargestEntityState)) \
	N(Snapshots, PACKET_BACKUP * sizeof(idLargestClientSnapshot)) \
	N(EntityEventTimes, MAX_GENTITIES * sizeof(s32))

#define UDT_BASE_PARSER_FIXED_SIZE_ARRAY_ITEM(Enum, Bytes) Enum,
struct udtBaseParserFixedSizeArray
{
	enum Id
	{
		UDT_BASE_PARSER_FIXED_SIZE_ARRAY_LIST(UDT_BASE_PARSER_FIXED_SIZE_ARRAY_ITEM)
		Count
	};
};
#undef UDT_BASE_PARSER_FIXED_SIZE_ARRAY_ITEM

struct udtBaseParser;
typedef udtStream* (*udtDemoStreamCreator)(s32 startTime, s32 endTime, const char* veryShortDesc, udtBaseParser* parser, void* userData);

struct udtBaseParser
{
public:
	struct udtConfigString;

public:
	udtBaseParser();
	~udtBaseParser();

	void    SetAllocators(udtVMLinearAllocator* linearAllocators, u8** fixedSizeArrays); // Once for all demos.
	bool	Init(udtContext* context, udtProtocol::Id protocol, s32 gameStateIndex = 0); // Once for each demo.
	void	SetFilePath(const char* filePath); // Once for each demo.
	void	Destroy();

	bool	ParseNextMessage(const udtMessage& inMsg, s32 inServerMessageSequence, u32 fileOffset); // Returns true if should continue parsing.
	void	FinishParsing(bool success);

	void	AddCut(s32 gsIndex, s32 startTimeMs, s32 endTimeMs, udtDemoStreamCreator streamCreator, const char* veryShortDesc, void* userData = NULL);
	void    AddPlugIn(udtBaseParserPlugIn* plugIn);

	void                  InsertOrUpdateConfigString(const udtConfigString& cs);
	udtConfigString*      FindConfigStringByIndex(s32 csIndex); // Returns NULL when not found.

private:
	bool                  ParseServerMessage(); // Returns true if should continue parsing.
	bool                  ShouldWriteMessage() const;
	void                  WriteFirstMessage();
	void                  WriteNextMessage();
	void                  WriteLastMessage();
	void                  WriteGameState();
	bool                  ParseCommandString();
	bool                  ParseGamestate();
	bool                  ParseSnapshot();
	bool                  ParsePacketEntities(udtMessage& msg, idClientSnapshotBase* oldframe, idClientSnapshotBase* newframe);
	void                  EmitPacketEntities(idClientSnapshotBase* from, idClientSnapshotBase* to);
	void                  DeltaEntity(udtMessage& msg, idClientSnapshotBase *frame, s32 newnum, idEntityStateBase* old, qbool unchanged);
	char*                 AllocateString(udtVMLinearAllocator& allocator, const char* string, u32 stringLength = 0, u32* outStringLength = NULL);
	void                  ResetForGamestateMessage();
	const char*           GetFileName() { return (_inFileName != NULL) ? _inFileName : "N/A"; }

public:
	idEntityStateBase*    GetEntity(s32 idx) const { return (idEntityStateBase*)&_inParseEntities[idx * _protocolSizeOfEntityState]; }
	idEntityStateBase*    GetBaseline(s32 idx) const { return (idEntityStateBase*)&_inEntityBaselines[idx * _protocolSizeOfEntityState]; }
	idClientSnapshotBase* GetClientSnapshot(s32 idx) const { return (idClientSnapshotBase*)&_inSnapshots[idx * _protocolSizeOfClientSnapshot]; }
	
	// You don't have to deallocate the object, but you will have to call the destructor manually yourself.
	template<class T>
	T* CreatePersistentObject()
	{
		return new (GetPersistentAllocator().Allocate((u32)sizeof(T))) T;
	}

	udtVMLinearAllocator& GetPersistentAllocator()
	{
		return _linearAllocators[udtBaseParserAllocator::PersistentAllocator];
	}

	udtVMLinearAllocator& GetTempAllocator()
	{
		return _linearAllocators[udtBaseParserAllocator::TempAllocator];
	}
	
public:
	struct udtCutInfo
	{
		udtDemoStreamCreator StreamCreator;
		udtStream* Stream; // Requires manual destruction.
		void* UserData;
		const char* VeryShortDesc;
		s32 GameStateIndex;
		s32 StartTimeMs;
		s32 EndTimeMs;
	};

	struct udtConfigString
	{
		const char* String;
		u32 StringLength;
		s32 Index;
	};

	struct udtServerCommand
	{
		const char* String;
		u32 StringLength;
	};

public:
	// General.
	udtVMLinearAllocator* _linearAllocators; // Points to udtBaseParserAllocator::Count allocator instances.
	udtContext* _context; // This instance does *NOT* have ownership of the context.
	udtProtocol::Id _protocol;
	s32 _protocolSizeOfEntityState;
	s32 _protocolSizeOfClientSnapshot;

	// Callbacks. Useful for doing additional analysis/processing in the same demo reading pass.
	void* UserData; // Put whatever you want in there. Useful for callbacks.
	udtVMArray<udtBaseParserPlugIn*> PlugIns;

	// Input.
	const char* _inFilePath;
	const char* _inFileName;
	udtMessage _inMsg; // This instance does *NOT* have ownership of the raw message data.
	u32 _inFileOffset;
	s32 _inServerMessageSequence; // Unreliable.
	s32 _inServerCommandSequence; // Reliable.
	s32 _inReliableSequenceAcknowledge;
	s32 _inClientNum;
	s32 _inChecksumFeed;
	s32 _inParseEntitiesNum;
	s32 _inServerTime;
	s32 _inGameStateIndex;
	s32 _inLastSnapshotMessageNumber;
	u8* _inEntityBaselines; // Fixed-size array of size MAX_PARSE_ENTITIES. Must be zeroed initially.
	u8* _inParseEntities; // Fixed-size array of size MAX_PARSE_ENTITIES.
	u8* _inSnapshots; // Fixed-size array of size MAX_PARSE_ENTITIES.
	s32* _inEntityEventTimesMs; // Fixed-size array of size MAX_GENTITIES. The server time, in ms, of the last event for a given entity.
	udtVMArray<u32> _inGameStateFileOffsets;
	udtVMArray<udtConfigString> _inConfigStrings;
	udtVMArray<udtChangedEntity> _inChangedEntities; // The entities that were read (added or changed) in the last call to ParsePacketEntities.
	udtVMArray<idEntityStateBase*> _inRemovedEntities; // The entities that were removed in the last call to ParsePacketEntities.
	idLargestClientSnapshot _inSnapshot;

	// Output.
	udtVMArray<udtCutInfo> _cuts;
	u8 _outMsgData[MAX_MSGLEN];
	udtMessage _outMsg; // This instance *DOES* have ownership of the raw message data.
	s32 _outServerCommandSequence;
	s32 _outSnapshotsWritten;
	bool _outWriteFirstMessage;
	bool _outWriteMessage;
};