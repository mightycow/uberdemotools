#include "converter_udt_to_quake.hpp"
#include "utils.hpp"


udtdConverter::udtdConverter()
{
	_input = NULL;
	_output = NULL;
	_protocol = udtProtocol::Invalid;
	_protocolSizeOfEntityState = 0;
	_protocolSizeOfPlayerState = 0;
	_firstMessage = true;
}

udtdConverter::~udtdConverter()
{
}

void udtdConverter::ResetForNextDemo(udtStream& input, udtStream* output, udtProtocol::Id protocol)
{
	_input = &input;
	_output = output;
	_protocol = protocol;
	_protocolSizeOfEntityState = udtGetSizeOfIdEntityState((u32)protocol);
	_protocolSizeOfPlayerState = udtGetSizeOfIdPlayerState((u32)protocol);

	_outMsg.InitContext(&_context);
	_outMsg.InitProtocol(protocol);
	_outCommandSequence = 1;
	_outMessageSequence = 1;

	memset(_snapshots, 0, sizeof(_snapshots));
	_snapshotReadIndex = 0;

	memset(_inBaselineEntities, 0, sizeof(_inBaselineEntities));
	memset(_inReadEntities, 0, sizeof(_inReadEntities));
	memset(_inRemovedEntities, 0, sizeof(_inRemovedEntities));
	memset(_areaMask, 0, sizeof(_areaMask));
	_firstSnapshot = true;
	_firstMessage = true;
}

void udtdConverter::SetStreams(udtStream& input, udtStream* output)
{
	_input = &input;
	_output = output;
}

void udtdConverter::AddPlugIn(udtdConverterPlugIn* plugIn)
{
	if(plugIn != NULL)
	{
		_plugIns.Add(plugIn);
	}
}

void udtdConverter::ClearPlugIns()
{
	_plugIns.Clear();
}

bool udtdConverter::ProcessNextMessage(udtdMessageType::Id& type)
{
	if(_firstMessage)
	{
		_firstMessage = false;
		for(u32 i = 0, count = _plugIns.GetSize(); i < count; ++i)
		{
			_plugIns[i]->InitPlugIn(_protocol);
		}
	}

	s32 messageType = 0;
	if(_input->Read(&messageType, 4, 1) != 1)
	{
		type = udtdMessageType::Invalid;
		return false;
	}

	type = (udtdMessageType::Id)messageType;
	switch((udtdMessageType::Id)messageType)
	{
		case udtdMessageType::GameState:
			ProcessGameState();
			break;

		case udtdMessageType::Command:
			ProcessCommand();
			break;

		case udtdMessageType::Snapshot:
			ProcessSnapshot();
			break;

		case udtdMessageType::EndOfFile:
			ProcessEndOfFile();
			return false;

		default:
			type = udtdMessageType::Invalid;
			return false;
	}

	return true;
}

bool udtdConverter::ProcessNextMessageRead(udtdMessageType::Id& type, SnapshotInfo& snapshot)
{
	s32 messageType = 0;
	if(_input->Read(&messageType, 4, 1) != 1)
	{
		type = udtdMessageType::Invalid;
		return false;
	}

	type = (udtdMessageType::Id)messageType;
	switch((udtdMessageType::Id)messageType)
	{
		case udtdMessageType::GameState:
			ProcessGameState();
			break;

		case udtdMessageType::Command:
			ProcessCommand();
			break;

		case udtdMessageType::Snapshot:
			ReadSnapshot(snapshot);
			break;

		case udtdMessageType::EndOfFile:
			ProcessEndOfFile();
			return false;

		default:
			type = udtdMessageType::Invalid;
			return false;
	}

	return true;
}

bool udtdConverter::ProcessNextMessageWrite(udtdMessageType::Id type, const SnapshotInfo& snapshot)
{
	switch(type)
	{
		case udtdMessageType::GameState:
		case udtdMessageType::Command:
			return true;

		case udtdMessageType::Snapshot:
			WriteSnapshot(snapshot);
			return true;

		case udtdMessageType::EndOfFile:
		case udtdMessageType::Invalid:
		default:
			return false;
	}
}

bool udtdConverter::ProcessGameState()
{
	s32 sequenceAcknowledge = 0;
	s32 messageSequence = 0;
	s32 commandSequence = 0;
	s32 clientNum = 0;
	s32 checksumFeed = 0;
	_input->Read(&sequenceAcknowledge, 4, 1);
	_input->Read(&messageSequence, 4, 1);
	_input->Read(&commandSequence, 4, 1);
	_input->Read(&clientNum, 4, 1);
	_input->Read(&checksumFeed, 4, 1);
	_outCommandSequence = commandSequence;

	_outMsg.Init(_outMsgData, sizeof(_outMsgData));
	_outMsg.Bitstream();
	_outMsg.WriteLong(sequenceAcknowledge);
	_outMsg.WriteByte(svc_gamestate);
	_outMsg.WriteLong(_outCommandSequence);
	++_outCommandSequence;

	s32 configStringCount = 0;
	_input->Read(&configStringCount, 4, 1);

	for(s32 i = 0; i < configStringCount; ++i)
	{
		s32 index = 0;
		s32 length = 0;
		_input->Read(&index, 4, 1);
		_input->Read(&length, 4, 1);
		_input->Read(_inStringData, (u32)length, 1);
		if(length < 0 || length > BIG_INFO_STRING)
		{
			return false;
		}

		_inStringData[length] = '\0';

		for(u32 j = 0, count = _plugIns.GetSize(); j < count; ++j)
		{
			_plugIns[j]->AnalyzeConfigString(index, _inStringData, (u32)length);
		}

		_outMsg.WriteByte(svc_configstring);
		_outMsg.WriteShort(index);
		_outMsg.WriteBigString(_inStringData, length);
	}

	s32 baselineEntityCount = 0;
	_input->Read(&baselineEntityCount, 4, 1);

	idLargestEntityState nullState;
	memset(&nullState, 0, sizeof(nullState));
	for(s32 i = 0; i < baselineEntityCount; ++i)
	{
		s32 index = 0;
		idLargestEntityState es;
		_input->Read(&index, 4, 1);
		_input->Read(&es, _protocolSizeOfEntityState, 1);
		memcpy(GetBaseline(es.number), &es, (size_t)_protocolSizeOfEntityState);

		_outMsg.WriteByte(svc_baseline);
		_outMsg.WriteDeltaEntity(&nullState, &es, true);
	}

	_outMsg.WriteByte(svc_EOF);
	_outMsg.WriteLong(clientNum);
	_outMsg.WriteLong(checksumFeed);
	_outMsg.WriteByte(svc_EOF);

	WriteOutputMessageToFile(false);

	return true;
}

void udtdConverter::ProcessCommand()
{
	s32 messageSequence = 0;
	s32 commandSequence = 0;
	u32 stringLength = 0;
	_input->Read(&messageSequence, 4, 1);
	_input->Read(&commandSequence, 4, 1);
	_input->Read(&stringLength, 4, 1);
	_input->Read(_inStringData, stringLength, 1);
	_inStringData[stringLength] = '\0';

	s32 sequenceAcknowledge = 0;
	_outMsg.Init(_outMsgData, sizeof(_outMsgData));
	_outMsg.Bitstream();
	_outMsg.WriteLong(sequenceAcknowledge);
	_outMsg.WriteByte(svc_serverCommand);
	_outMsg.WriteLong(_outCommandSequence);
	_outMsg.WriteString(_inStringData, (s32)stringLength);
	_outMsg.WriteByte(svc_EOF);
	++_outCommandSequence;

	WriteOutputMessageToFile(false);
}

void udtdConverter::ProcessSnapshot()
{
	SnapshotInfo info;
	ReadSnapshot(info);

	udtdSnapshotData& curSnap = _snapshots[_snapshotReadIndex];
	udtdSnapshotData& oldSnap = _snapshots[_snapshotReadIndex ^ 1];
	for(u32 i = 0, count = _plugIns.GetSize(); i < count; ++i)
	{
		_plugIns[i]->ModifySnapshot(curSnap, oldSnap);
	}

	WriteSnapshot(info);
}

void udtdConverter::ReadSnapshot(SnapshotInfo& info)
{
	s32 messageSequence = 0;
	idLargestPlayerState playerState;
	_input->Read(&messageSequence, 4, 1);
	_input->Read(&info.ServerTime, 4, 1);
	_input->Read(&playerState, _protocolSizeOfPlayerState, 1);
	_input->Read(&info.SnapFlags, 4, 1);
	_input->Read(_areaMask, 32, 1);

	s32 addedOrChangedEntityCount = 0;
	_input->Read(&addedOrChangedEntityCount, 4, 1);
	_input->Read(_inReadEntities, (u32)addedOrChangedEntityCount * _protocolSizeOfEntityState, 1);

	s32 removedEntityCount = 0;
	_input->Read(&removedEntityCount, 4, 1);
	_input->Read(_inRemovedEntities, (u32)removedEntityCount * 4, 1);

	udtdSnapshotData& curSnap = _snapshots[_snapshotReadIndex];
	udtdSnapshotData& oldSnap = _snapshots[_snapshotReadIndex ^ 1];

	curSnap.ServerTime = info.ServerTime;
	memcpy(&curSnap.PlayerState, &playerState, _protocolSizeOfPlayerState);

	if(_firstSnapshot)
	{
		for(s32 i = 0; i < MAX_GENTITIES; ++i)
		{
			curSnap.Entities[i].Valid = false;
		}
	}
	else
	{
		memcpy(curSnap.Entities, oldSnap.Entities, sizeof(udtdClientEntity) * MAX_GENTITIES);
	}

	for(s32 i = 0; i < addedOrChangedEntityCount; ++i)
	{
		const idEntityStateBase* const es = GetEntity(i);
		const s32 number = es->number;
		curSnap.Entities[number].Valid = true;
		memcpy(&curSnap.Entities[number].EntityState, es, (size_t)_protocolSizeOfEntityState);
	}

	for(s32 i = 0; i < removedEntityCount; ++i)
	{
		const s32 number = _inRemovedEntities[i];
		curSnap.Entities[number].Valid = false;
	}
}

void udtdConverter::WriteSnapshot(const SnapshotInfo& info)
{
	udtdSnapshotData& curSnap = _snapshots[_snapshotReadIndex];
	const udtdSnapshotData& oldSnap = _snapshots[_snapshotReadIndex ^ 1];

	const s32 sequenceAcknowledge = 0;
	const s32 deltaNum = _firstSnapshot ? 0 : 1;
	_outMsg.Init(_outMsgData, sizeof(_outMsgData));
	_outMsg.Bitstream();
	_outMsg.WriteLong(sequenceAcknowledge);
	_outMsg.WriteByte(svc_snapshot);
	_outMsg.WriteLong(info.ServerTime);
	_outMsg.WriteByte(deltaNum);
	_outMsg.WriteByte(info.SnapFlags);
	_outMsg.WriteByte(32);
	_outMsg.WriteData(_areaMask, 32);
	_outMsg.WriteDeltaPlayer(_firstSnapshot ? NULL : &oldSnap.PlayerState, &curSnap.PlayerState);

	if(_firstSnapshot)
	{
		for(s32 i = 0; i < MAX_GENTITIES; ++i)
		{
			if(curSnap.Entities[i].Valid)
			{
				_outMsg.WriteDeltaEntity(GetBaseline(i), &curSnap.Entities[i].EntityState, true);
			}
		}
	}
	else
	{
		for(s32 i = 0; i < MAX_GENTITIES; ++i)
		{
			const bool curValid = curSnap.Entities[i].Valid;
			const bool oldValid = oldSnap.Entities[i].Valid;
			const idEntityStateBase& curEnt = curSnap.Entities[i].EntityState;
			const idEntityStateBase& oldEnt = oldSnap.Entities[i].EntityState;
			if(curValid && oldValid && memcmp(&curEnt, &oldEnt, (size_t)_protocolSizeOfEntityState))
			{
				// Entity changed.
				_outMsg.WriteDeltaEntity(&oldEnt, &curEnt, false);
			}
			else if(curValid && !oldValid)
			{
				// Entity added from the baseline.
				_outMsg.WriteDeltaEntity(GetBaseline(i), &curEnt, true);
			}
			else if(oldValid && !curValid)
			{
				// Entity removed.
				_outMsg.WriteDeltaEntity(&oldEnt, NULL, true);
			}
		}
	}

	_outMsg.WriteBits(MAX_GENTITIES - 1, GENTITYNUM_BITS); // Ends the list of entities.
	_outMsg.WriteByte(svc_EOF); // No more commands to follow.

	WriteOutputMessageToFile(true);

	_snapshotReadIndex ^= 1;
	_firstSnapshot = false;
}

void udtdConverter::ProcessEndOfFile()
{
	if(_output == NULL)
	{
		return;
	}

	s32 minusOne = -1;
	_output->Write(&minusOne, 4, 1);
	_output->Write(&minusOne, 4, 1);
}

void udtdConverter::WriteOutputMessageToFile(bool increaseMessageSequence)
{
	if(_output == NULL)
	{
		return;
	}

	if(increaseMessageSequence)
	{
		++_outMessageSequence;
	}

	const s32 messageLength = _outMsg.Buffer.cursize;
	_output->Write(&_outMessageSequence, 4, 1);
	_output->Write(&messageLength, 4, 1);
	_output->Write(_outMsg.Buffer.data, (u32)messageLength, 1);
}

void udtdConverter::MergeEntitiesFrom(const udtdConverter& sourceConv, u32 flipThis, u32 flipSource)
{
	udtdSnapshotData& dest = _snapshots[_snapshotReadIndex ^ flipThis];
	const udtdSnapshotData& source = sourceConv._snapshots[sourceConv._snapshotReadIndex ^ flipSource];

	udtdSnapshotData& destOld = _snapshots[_snapshotReadIndex ^ flipThis ^ 1];
	const udtdSnapshotData& sourceOld = sourceConv._snapshots[sourceConv._snapshotReadIndex ^ flipSource ^ 1];

	MergeEntities(dest, destOld, source, sourceOld);
}

static bool IsMoving(const idEntityStateBase& old, const idEntityStateBase& cur)
{
	return memcmp(old.pos.trBase, cur.pos.trBase, sizeof(idVec3)) != 0;
}

void udtdConverter::MergeEntities(udtdSnapshotData& dest, udtdSnapshotData& destOld, const udtdSnapshotData& source, const udtdSnapshotData& sourceOld)
{
	const s32 idEntityTypePlayerId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Player, _protocol);
	const s32 idEntityTypeItemId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Item, _protocol);
	for(u32 i = 0; i < MAX_GENTITIES; ++i)
	{
		const idEntityStateBase& sourceEnt = source.Entities[i].EntityState;
		if(sourceEnt.eType == idEntityTypePlayerId)
		{
			MergePlayerEntity(dest, destOld, source, sourceOld, i);
		}
		else if(sourceEnt.eType == idEntityTypeItemId)
		{
			MergeItemEntity(dest, destOld, source, sourceOld, i);
		}
		// Here, we filter out any events pertaining to the player in first-person.
		// @FIXME: If the guy in first-person has client number 0, this will prevent a lot of stuff from being merged.
		else if(sourceEnt.clientNum != dest.PlayerState.clientNum &&
				source.Entities[i].Valid && !dest.Entities[i].Valid)
		{
			dest.Entities[i].Valid = true;
			memcpy(&dest.Entities[i].EntityState, &source.Entities[i].EntityState, (size_t)_protocolSizeOfEntityState);
		}
	}

	const s32 firstPersonNumber = source.PlayerState.clientNum;

	dest.Entities[firstPersonNumber].Valid = true;
	s32 eventSeqCopy = sourceOld.PlayerState.eventSequence;
	PlayerStateToEntityState(dest.Entities[firstPersonNumber].EntityState, eventSeqCopy, source.PlayerState, false, dest.ServerTime, _protocol);
}

bool udtdConverter::IsPlayerAlreadyDefined(const udtdSnapshotData& snapshot, s32 clientNum, s32 entityNumber)
{
	const s32 idEntityTypePlayerId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Player, _protocol);
	for(s32 i = 0; i < MAX_GENTITIES; ++i)
	{
		if(snapshot.Entities[i].Valid &&
		   i != entityNumber &&
		   snapshot.Entities[i].EntityState.eType == idEntityTypePlayerId &&
		   snapshot.Entities[i].EntityState.clientNum == clientNum)
		{
			return true;
		}
	}

	return false;
}

void udtdConverter::MergePlayerEntity(udtdSnapshotData& dest, udtdSnapshotData& destOld, const udtdSnapshotData& source, const udtdSnapshotData& sourceOld, u32 i)
{
	const idEntityStateBase& sourceEnt = source.Entities[i].EntityState;
	if(sourceEnt.clientNum == dest.PlayerState.clientNum)
	{
		// Avoid adding the first-person player to the entities list.
		return;
	}

	if(IsPlayerAlreadyDefined(dest, sourceEnt.clientNum, i))
	{
		// Avoid doubling players... alive or dead.
		return;
	}

	if(source.Entities[i].Valid && !dest.Entities[i].Valid)
	{
		// The other demo has a player we don't have.
		dest.Entities[i].Valid = true;
		memcpy(&dest.Entities[i].EntityState, &source.Entities[i].EntityState, (size_t)_protocolSizeOfEntityState);
	}
	else if(source.Entities[i].Valid && dest.Entities[i].Valid &&
			IsMoving(sourceOld.Entities[i].EntityState, source.Entities[i].EntityState) &&
			!IsMoving(destOld.Entities[i].EntityState, dest.Entities[i].EntityState))
	{
		// The other demo says this player is moving and we think it doesn't, so copy some data over.
		dest.Entities[i].Valid = true;
		memcpy(&dest.Entities[i].EntityState, &source.Entities[i].EntityState, (size_t)_protocolSizeOfEntityState);
		// This will help avoid a bunch of problems due to inconsistent event sequences.
		dest.Entities[i].EntityState.event = 0;
		dest.Entities[i].EntityState.eventParm = 0;
	}
}

void udtdConverter::MergeItemEntity(udtdSnapshotData& /*dest*/, udtdSnapshotData& /*destOld*/, const udtdSnapshotData& /*source*/, const udtdSnapshotData& /*sourceOld*/, u32 /*i*/)
{
	// @TODO: Item pickup tracking for all demos?
	// Adding items from other demos when the primary one doesn't have them defined is unfortunately not correct.
	// It can happen that the player of the secondary demos didn't get notified that an item was taken.
	// You would then see a strange thing in the merged demo: a player jumps over an item and it stays there.
}
