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


struct udtDemoStreamCreatorArg
{
	s32 StartTimeMs;
	s32 EndTimeMs;
	const char* VeryShortDesc;
	udtBaseParser* Parser;
	void* UserData;
	udtVMLinearAllocator* TempAllocator;
	udtVMLinearAllocator* FilePathAllocator;
};

struct udtBaseParser;
typedef udtString (*udtDemoNameCreator)(const udtDemoStreamCreatorArg& info);


// Don't ever allocate an instance of this on the stack.
struct udtBaseParser
{
public:
	struct udtConfigString;

public:
	udtBaseParser();
	~udtBaseParser();

	bool	Init(udtContext* context, udtProtocol::Id protocol, udtProtocol::Id outProtocol, s32 gameStateIndex = 0, bool enablePlugIns = true); // Once for each demo.
	void	SetFilePath(const char* filePath); // Once for each demo. After Init.
	void	Destroy();

	bool	ParseNextMessage(const udtMessage& inMsg, s32 inServerMessageSequence, u32 fileOffset); // Returns true if should continue parsing.
	void	FinishParsing(bool success);

	void	AddCut(s32 gsIndex, s32 startTimeMs, s32 endTimeMs, udtDemoNameCreator streamCreator, const char* veryShortDesc, void* userData = NULL);
	void	AddCut(s32 gsIndex, s32 startTimeMs, s32 endTimeMs, const char* filePath);
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
	bool                  DeltaEntity(udtMessage& msg, idClientSnapshotBase *frame, s32 newnum, idEntityStateBase* old, bool unchanged);
	void                  ResetForGamestateMessage();

public:
	idEntityStateBase*    GetEntity(s32 idx) const { return (idEntityStateBase*)&_inParseEntities[idx * _inProtocolSizeOfEntityState]; }
	idEntityStateBase*    GetBaseline(s32 idx) const { return (idEntityStateBase*)&_inEntityBaselines[idx * _inProtocolSizeOfEntityState]; }
	idClientSnapshotBase* GetClientSnapshot(s32 idx) const { return (idClientSnapshotBase*)&_inSnapshots[idx * _inProtocolSizeOfClientSnapshot]; }
	const idTokenizer&    GetTokenizer() { return _tokenizer; }
	const char*           GetFileNamePtr() { return _inFileName.GetPtrSafe("N/A"); }
	
public:
	struct udtCutInfo
	{
		const char* FilePath;
		udtDemoNameCreator StreamCreator;
		void* UserData;
		const char* VeryShortDesc;
		s32 GameStateIndex;
		s32 StartTimeMs;
		s32 EndTimeMs;
	};

public:
	// General.
	udtVMLinearAllocator _persistentAllocator { "Parser::Persistent" }; // Memory we need to be able to access to during the entire parsing phase.
	udtVMLinearAllocator _configStringAllocator { "Parser::ConfigStrings" }; // Gets cleated every time a new gamestate message is encountered.
	udtVMLinearAllocator _tempAllocator { "Parser::Temp" };
	udtVMLinearAllocator _privateTempAllocator { "Parser::PrivateTemp" };
	udtContext* _context; // This instance does *NOT* have ownership of the context.
	udtProtocol::Id _inProtocol;
	s32 _inProtocolSizeOfEntityState;
	s32 _inProtocolSizeOfClientSnapshot;
	udtProtocol::Id _outProtocol;
	udtProtocolConverter* _protocolConverter;

	// Callbacks. Useful for doing additional analysis/processing in the same demo reading pass.
	void* UserData; // Put whatever you want in there. Useful for callbacks.
	udtVMArray<udtBaseParserPlugIn*> PlugIns { "Parser::PlugInsArray" };
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
	u8 _inMsgData[ID_MAX_MSG_LENGTH];
	u8 _inEntityBaselines[ID_MAX_PARSE_ENTITIES * sizeof(idLargestEntityState)]; // Type depends on protocol. Must be zeroed initially.
	u8 _inParseEntities[ID_MAX_PARSE_ENTITIES * sizeof(idLargestEntityState)]; // Type depends on protocol.
	u8 _inSnapshots[PACKET_BACKUP * sizeof(idLargestClientSnapshot)]; // Type depends on protocol.
	s32 _inEntityEventTimesMs[MAX_GENTITIES]; // The server time, in ms, of the last event for a given entity.
	char _inBigConfigString[BIG_INFO_STRING]; // For handling the bcs0, bcs1 and bcs2 server commands.
	udtString _inConfigStrings[2 * MAX_CONFIGSTRINGS]; // Apparently some Quake 3 mods have bumped the original MAX_CONFIGSTRINGS value up?
	udtVMArray<u32> _inGameStateFileOffsets { "Parser::GameStateFileOffsetsArray" };
	udtVMArray<udtChangedEntity> _inChangedEntities { "Parser::ChangedEntitiesArray" }; // The entities that were read (added or changed) in the last call to ParsePacketEntities.
	udtVMArray<s32> _inRemovedEntities { "Parser::RemovedEntitiesArray" }; // The entities that were removed in the last call to ParsePacketEntities.
	udtVMArray<idEntityStateBase*> _inEntities { "Parser::EntitiesArray" }; // All entities that were read in the last call to ParsePacketEntities.
	udtVMArray<u8> _inEntityFlags { "Parser::EntityFlagsArray" };

	// Output.
	udtFileStream _outFile;
	udtString _outFilePath;
	udtString _outFileName;
	udtVMArray<udtCutInfo> _cuts { "Parser::CutsArray" };
	u8 _outMsgData[ID_MAX_MSG_LENGTH];
	udtMessage _outMsg; // This instance *DOES* have ownership of the raw message data.
	s32 _outServerCommandSequence;
	s32 _outSnapshotsWritten;
	bool _outWriteFirstMessage;
	bool _outWriteMessage;

private:
	idTokenizer _tokenizer; // Make sure plug-ins don't get write access to this.
};