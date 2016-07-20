#include "plug_in_converter_quake_to_udt.hpp"
#include "udtd_types.hpp"
#include "utils.hpp"


udtParserPlugInQuakeToUDT::udtParserPlugInQuakeToUDT()
{
	_outputFile = NULL;
	_data = (udtdData*)_allocator.AllocateAndGetAddress((uptr)sizeof(udtdData));
	_firstSnapshot = true;
	_protocol = udtProtocol::Invalid;
	_protocolSizeOfEntityState = 0;
	_protocolSizeOfPlayerState = 0;
}

udtParserPlugInQuakeToUDT::~udtParserPlugInQuakeToUDT()
{
}

bool udtParserPlugInQuakeToUDT::ResetForNextDemo(udtProtocol::Id protocol)
{
	if(_data == NULL)
	{
		return false;
	}

	memset(_data, 0, sizeof(udtdData));
	_firstSnapshot = true;
	_protocol = protocol;
	_protocolSizeOfEntityState = udtGetSizeOfIdEntityState(protocol);
	_protocolSizeOfPlayerState = udtGetSizeOfIdPlayerState(protocol);

	return true;
}

void udtParserPlugInQuakeToUDT::SetOutputStream(udtStream* output)
{
	_outputFile = output;
}

void udtParserPlugInQuakeToUDT::InitAllocators(u32 /*demoCount*/)
{
}

void udtParserPlugInQuakeToUDT::StartDemoAnalysis()
{
}

void udtParserPlugInQuakeToUDT::FinishDemoAnalysis()
{
	const u32 messageType = (u32)udtdMessageType::EndOfFile;
	_outputFile->Write(&messageType, 4, 1);
}

void udtParserPlugInQuakeToUDT::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	const u32 messageType = (u32)udtdMessageType::GameState;
	_outputFile->Write(&messageType, 4, 1);
	_outputFile->Write(&parser._inReliableSequenceAcknowledge, 4, 1);
	_outputFile->Write(&parser._inServerMessageSequence, 4, 1);
	_outputFile->Write(&parser._inServerCommandSequence, 4, 1);
	_outputFile->Write(&parser._inClientNum, 4, 1);
	_outputFile->Write(&parser._inChecksumFeed, 4, 1);

	s32 configStringCount = 0;
	for(u32 i = 0; i < 2*MAX_CONFIGSTRINGS; ++i)
	{
		if(!udtString::IsNullOrEmpty(parser._inConfigStrings[i]))
		{
			++configStringCount;
		}
	}
	_outputFile->Write(&configStringCount, 4, 1);
	for(u32 i = 0; i < 2 * MAX_CONFIGSTRINGS; ++i)
	{
		const udtString& cs = parser._inConfigStrings[i];
		if(!udtString::IsNullOrEmpty(cs))
		{
			const u32 length = cs.GetLength();
			_outputFile->Write(&i, 4, 1);
			_outputFile->Write(&length, 4, 1);
			_outputFile->Write(cs.GetPtr(), length, 1);
		}
	}

	idLargestEntityState nullState;
	memset(&nullState, 0, sizeof(nullState));
	s32 baselineEntityCount = 0;
	for(s32 i = 0; i < ID_MAX_PARSE_ENTITIES; ++i)
	{
		const idEntityStateBase& es = *parser.GetBaseline(i);
		if(memcmp(&nullState, &es, (size_t)_protocolSizeOfEntityState))
		{
			++baselineEntityCount;
		}
	}

	_outputFile->Write(&baselineEntityCount, 4, 1);
	for(s32 i = 0; i < ID_MAX_PARSE_ENTITIES; ++i)
	{
		const idEntityStateBase& es = *parser.GetBaseline(i);
		if(memcmp(&nullState, &es, (size_t)_protocolSizeOfEntityState))
		{
			_outputFile->Write(&i, 4, 1);
			_outputFile->Write(&es, _protocolSizeOfEntityState, 1);
		}
	}
}

void udtParserPlugInQuakeToUDT::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	assert(arg.Snapshot != NULL);

	if(arg.ServerTime > _data->LastSnapshotTimeMs)
	{
		const s32 lastTime = _data->LastSnapshotTimeMs;
		_data->LastSnapshotTimeMs = arg.ServerTime;
		if(lastTime != 0)
		{
			WriteSnapshot(parser, *arg.Snapshot);
		}
	}

	const s32 writeIndex = _data->SnapshotReadIndex;
	udtdSnapshot& snapshot = _data->Snapshots[writeIndex];
	snapshot.ServerTime = arg.ServerTime;

	for(u32 i = 0; i < MAX_GENTITIES; ++i)
	{
		snapshot.Entities[i].Valid = false;
	}

	for(s32 i = 0, count = arg.Snapshot->numEntities; i < count; ++i)
	{
		const s32 index = (arg.Snapshot->parseEntitiesNum + i) & (ID_MAX_PARSE_ENTITIES - 1);
		idEntityStateBase& entity = *parser.GetEntity(index);
		const s32 number = entity.number;
		snapshot.Entities[number].Valid = true;
		memcpy(&snapshot.Entities[number].EntityState, &entity, _protocolSizeOfEntityState);
	}
}

void udtParserPlugInQuakeToUDT::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	const u32 messageType = (u32)udtdMessageType::Command;
	_outputFile->Write(&messageType, 4, 1);
	_outputFile->Write(&parser._inServerMessageSequence, 4, 1);
	_outputFile->Write(&arg.CommandSequence, 4, 1);
	_outputFile->Write(&arg.StringLength, 4, 1);
	_outputFile->Write(arg.String, arg.StringLength, 1);
}

void udtParserPlugInQuakeToUDT::WriteSnapshot(udtBaseParser& parser, idClientSnapshotBase& snapshot)
{
	idPlayerStateBase* const ps = GetPlayerState(&snapshot, parser._inProtocol);
	assert(ps != NULL);

	const u32 messageType = (u32)udtdMessageType::Snapshot;
	_outputFile->Write(&messageType, 4, 1);
	_outputFile->Write(&parser._inServerMessageSequence, 4, 1);
	_outputFile->Write(&parser._inServerTime, 4, 1);
	_outputFile->Write(ps, _protocolSizeOfPlayerState, 1);
	_outputFile->Write(&snapshot.snapFlags, 4, 1);
	_outputFile->Write(snapshot.areamask, 32, 1);

	const s32 curSnapIdx = _data->SnapshotReadIndex;
	const s32 oldSnapIdx = _data->SnapshotReadIndex ^ 1;
	const udtdClientEntity* const curSnap = _data->Snapshots[curSnapIdx].Entities;
	const udtdClientEntity* const oldSnap = _data->Snapshots[oldSnapIdx].Entities;

	if(_firstSnapshot)
	{
		_firstSnapshot = false;

		u32 addedOrChangedCount = 0;
		for(u32 i = 0; i < MAX_GENTITIES; ++i)
		{
			if(curSnap[i].Valid)
			{
				++addedOrChangedCount;
			}
		}

		_outputFile->Write(&addedOrChangedCount, 4, 1);
		for(u32 i = 0; i < MAX_GENTITIES; ++i)
		{
			if(curSnap[i].Valid)
			{
				_outputFile->Write(&curSnap[i].EntityState, _protocolSizeOfEntityState, 1);
			}
		}

		u32 removedCount = 0;
		_outputFile->Write(&removedCount, 4, 1);
	}
	else
	{
		u32 addedOrChangedCount = 0;
		for(u32 i = 0; i < MAX_GENTITIES; ++i)
		{
			const bool curValid = curSnap[i].Valid;
			const bool oldValid = oldSnap[i].Valid;
			const bool added = curValid && !oldValid;
			const bool changed = curValid && oldValid && memcmp(&curSnap[i].EntityState, &oldSnap[i].EntityState, (size_t)_protocolSizeOfEntityState);
			if(added || changed)
			{
				++addedOrChangedCount;
			}
		}

		_outputFile->Write(&addedOrChangedCount, 4, 1);
		for(u32 i = 0; i < MAX_GENTITIES; ++i)
		{
			const bool curValid = curSnap[i].Valid;
			const bool oldValid = oldSnap[i].Valid;
			const bool added = curValid && !oldValid;
			const bool changed = curValid && oldValid && memcmp(&curSnap[i].EntityState, &oldSnap[i].EntityState, (size_t)_protocolSizeOfEntityState);
			if(added || changed)
			{
				_outputFile->Write(&curSnap[i].EntityState, _protocolSizeOfEntityState, 1);
			}
		}

		const u32 removedCount = parser._inRemovedEntities.GetSize();
		_outputFile->Write(&removedCount, 4, 1);
		_outputFile->Write(parser._inRemovedEntities.GetStartAddress(), 4 * removedCount, 1);
	}

	_data->SnapshotReadIndex ^= 1;
}
