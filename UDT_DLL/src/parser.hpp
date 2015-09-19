#pragma once


#include "context.hpp"
#include "message.hpp"
#include "tokenizer.hpp"
#include "file_stream.hpp"
#include "linear_allocator.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"
#include "protocol_conversion.hpp"

// For the placement new operator.
#include <new>


struct udtBaseParser;
typedef udtStream* (*udtDemoStreamCreator)(s32 startTime, s32 endTime, const char* veryShortDesc, udtBaseParser* parser, void* userData);

// Don't ever allocate an instance of this on the stack.
struct udtBaseParser
{
public:
	struct udtConfigString;

public:
	udtBaseParser();
	~udtBaseParser();

	void    InitAllocators(); // Once for all demos.
	bool	Init(udtContext* context, udtProtocol::Id protocol, udtProtocol::Id outProtocol, s32 gameStateIndex = 0, bool enablePlugIns = true); // Once for each demo.
	void	SetFilePath(const char* filePath); // Once for each demo. After Init.
	void	Destroy();

	bool	ParseNextMessage(const udtMessage& inMsg, s32 inServerMessageSequence, u32 fileOffset); // Returns true if should continue parsing.
	void	FinishParsing(bool success);

	void	AddCut(s32 gsIndex, s32 startTimeMs, s32 endTimeMs, udtDemoStreamCreator streamCreator, const char* veryShortDesc, void* userData = NULL);
	void    AddPlugIn(udtBaseParserPlugIn* plugIn);

	const udtString       GetConfigString(s32 csIndex) const;

private:
	bool                  ParseServerMessage(); // Returns true if should continue parsing.
	bool                  ShouldWriteMessage() const;
	void                  WriteFirstMessage();
	void                  WriteNextMessage();
	void                  WriteLastMessage();
	void                  WriteGameState();
	void                  WriteBigConfigStringCommand(const udtString& csIndex, const udtString& csData);
	bool                  ParseCommandString();
	bool                  ParseGamestate();
	bool                  ParseSnapshot();
	bool                  ParsePacketEntities(udtMessage& msg, idClientSnapshotBase* oldframe, idClientSnapshotBase* newframe);
	void                  EmitPacketEntities(idClientSnapshotBase* from, idClientSnapshotBase* to);
	void                  DeltaEntity(udtMessage& msg, idClientSnapshotBase *frame, s32 newnum, idEntityStateBase* old, qbool unchanged);
	char*                 AllocateString(udtVMLinearAllocator& allocator, const char* string, u32 stringLength = 0, u32* outStringLength = NULL);
	void                  ResetForGamestateMessage();
	const char*           GetFileName() { return (_inFileName.String != NULL) ? _inFileName.String : "N/A"; }

public:
	idEntityStateBase*    GetEntity(s32 idx) const { return (idEntityStateBase*)&_inParseEntities[idx * _inProtocolSizeOfEntityState]; }
	idEntityStateBase*    GetBaseline(s32 idx) const { return (idEntityStateBase*)&_inEntityBaselines[idx * _inProtocolSizeOfEntityState]; }
	idClientSnapshotBase* GetClientSnapshot(s32 idx) const { return (idClientSnapshotBase*)&_inSnapshots[idx * _inProtocolSizeOfClientSnapshot]; }
	const idTokenizer&    GetTokenizer() { return _tokenizer; }
	
	// You don't have to deallocate the object, but you will have to call the destructor manually yourself.
	template<class T>
	T* CreatePersistentObject()
	{
		return new (_persistentAllocator.Allocate((u32)sizeof(T))) T;
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

public:
	// General.
	udtVMLinearAllocator _persistentAllocator; // Memory we need to be able to access to during the entire parsing phase.
	udtVMLinearAllocator _configStringAllocator; // Gets cleated every time a new gamestate message is encountered.
	udtVMLinearAllocator _tempAllocator;
	udtVMLinearAllocator _privateTempAllocator;
	udtContext* _context; // This instance does *NOT* have ownership of the context.
	udtProtocol::Id _inProtocol;
	s32 _inProtocolSizeOfEntityState;
	s32 _inProtocolSizeOfClientSnapshot;
	udtProtocol::Id _outProtocol;
	udtProtocolConverter* _protocolConverter;

	// Callbacks. Useful for doing additional analysis/processing in the same demo reading pass.
	void* UserData; // Put whatever you want in there. Useful for callbacks.
	udtVMArrayWithAlloc<udtBaseParserPlugIn*> PlugIns;
	bool EnablePlugIns;

	// Input.
	udtString _inFilePath;
	udtString _inFileName;
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
	u8 _inEntityBaselines[MAX_PARSE_ENTITIES * sizeof(idLargestEntityState)]; // Type depends on protocol. Must be zeroed initially.
	u8 _inParseEntities[MAX_PARSE_ENTITIES * sizeof(idLargestEntityState)]; // Type depends on protocol.
	u8 _inSnapshots[PACKET_BACKUP * sizeof(idLargestClientSnapshot)]; // Type depends on protocol.
	s32 _inEntityEventTimesMs[MAX_GENTITIES]; // The server time, in ms, of the last event for a given entity.
	char _inBigConfigString[BIG_INFO_STRING]; // For handling the bcs0, bcs1 and bcs2 server commands.
	udtString _inConfigStrings[2 * MAX_CONFIGSTRINGS]; // Apparently some Quake 3 mods have bumped the original MAX_CONFIGSTRINGS value up?
	udtVMArrayWithAlloc<u32> _inGameStateFileOffsets;
	udtVMArrayWithAlloc<udtChangedEntity> _inChangedEntities; // The entities that were read (added or changed) in the last call to ParsePacketEntities.
	udtVMArrayWithAlloc<s32> _inRemovedEntities; // The entities that were removed in the last call to ParsePacketEntities.
	idLargestClientSnapshot _inSnapshot;

	// Output.
	udtVMArrayWithAlloc<udtCutInfo> _cuts;
	u8 _outMsgData[MAX_MSGLEN];
	udtMessage _outMsg; // This instance *DOES* have ownership of the raw message data.
	s32 _outServerCommandSequence;
	s32 _outSnapshotsWritten;
	bool _outWriteFirstMessage;
	bool _outWriteMessage;

private:
	idTokenizer _tokenizer; // Make sure plug-ins don't get write access to this.
};