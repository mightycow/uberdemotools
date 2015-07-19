#pragma once


struct udtdMessageType
{
	enum Id
	{
		GameState = 0xDEADBEEF,
		Snapshot = 0xCAFEBABE,
		Command = 0xFEE1DEAD,
		EndOfFile = 0xDEADC0DE,
		Invalid = 0xDEADDEAD
	};
};

#if 0

// This is the file format of UDT's non-delta-encoded demo files.

struct GenericMessage
{
	s32 MessageType;
	u8 Message[?];
};

struct GameStateConfigString
{
	s32 Index;
	s32 StringLength;
	u8 String[StringLength];
};

struct GameStateBaselineEntity
{
	s32 Index;
	idEntityStateBase EntityState;
};

struct GameStateMessage
{
	s32 SequenceAcknowledge;
	s32 MessageSequence;
	s32 CommandSequence;
	s32 ClientNum;
	s32 ChecksumFeed;
	s32 ConfigStringCount;
	GameStateConfigString ConfigStrings[ConfigStringCount];
	s32 BaselineEntityCount;
	GameStateBaselineEntity BaselineEntities[BaselineEntityCount];
};

struct SnapshotMessage
{
	s32 ServerTime;
	s32 MessageSequence;
	idPlayerStateBase PlayerState;
	u8 SnapFlags;
	u8 AreaMask[32];
	s32 AddedOrChangedEntityCount;
	idEntityStateBase AddedOrChangedEntities[EntityCount];
	s32 RemovedEntityCount;
	s32 RemovedEntityNumbers[RemovedEntityCount];
};

struct CommandMessage
{
	s32 MessageSequence;
	s32 CommandSequence;
	s32 StringLength;
	u8 String[StringLength];
};

#endif
