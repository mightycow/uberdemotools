#include "parser.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


udtBaseParser::udtBaseParser() 
{
	_context = NULL;
	_protocol = udtProtocol::Invalid;

	UserData = NULL;

	_inFileName = NULL;
	_inFilePath = NULL;
	_inFileOffset = 0;
	_inServerMessageSequence = -1;
	_inServerCommandSequence = -1;
	_inReliableSequenceAcknowledge = -1;
	_inClientNum = -1;
	_inChecksumFeed = -1;
	_inParseEntitiesNum = 0;
	_inGameStateIndex = -1;
	_inServerTime = S32_MIN;
	_inLastSnapshotMessageNumber = S32_MIN;

	_outServerCommandSequence = 0;
	_outSnapshotsWritten = 0;
	_outWriteFirstMessage = false;
	_outWriteMessage = false;
}

udtBaseParser::~udtBaseParser()
{
	Destroy();
}

void udtBaseParser::SetAllocators(udtVMLinearAllocator* linearAllocators, u8** fixedSizeArrays)
{
	_linearAllocators = linearAllocators;
	PlugIns.SetAllocator(linearAllocators[udtBaseParserAllocator::PlugInsArray]);
	_inGameStateFileOffsets.SetAllocator(linearAllocators[udtBaseParserAllocator::GameStateFileOffsetsArray]);
	_inConfigStrings.SetAllocator(linearAllocators[udtBaseParserAllocator::ConfigStringsArray]);
	_inChangedEntities.SetAllocator(linearAllocators[udtBaseParserAllocator::ChangedEntitiesArray]);
	_inRemovedEntities.SetAllocator(linearAllocators[udtBaseParserAllocator::RemovedEntitiesArray]);
	_cuts.SetAllocator(linearAllocators[udtBaseParserAllocator::CutsArray]);
	_inEntityBaselines = fixedSizeArrays[udtBaseParserFixedSizeArray::EntityBaselines];
	_inParseEntities = fixedSizeArrays[udtBaseParserFixedSizeArray::Entities];
	_inSnapshots = fixedSizeArrays[udtBaseParserFixedSizeArray::Snapshots];
	_inEntityEventTimesMs = (s32*)fixedSizeArrays[udtBaseParserFixedSizeArray::EntityEventTimes];
}

bool udtBaseParser::Init(udtContext* context, udtProtocol::Id protocol, s32 gameStateIndex)
{
	if(context == NULL || !udtIsValidProtocol(protocol) || gameStateIndex < 0)
	{
		return false;
	}

	_context = context;
	_protocol = protocol;
	_protocolSizeOfEntityState = (s32)udtGetSizeOfIdEntityState(protocol);
	_protocolSizeOfClientSnapshot = (s32)udtGetSizeOfidClientSnapshot(protocol);

	_outMsg.InitContext(context);
	_outMsg.InitProtocol(protocol);

	_inFileName = NULL;
	_inFilePath = NULL;

	_cuts.Clear();

	_inGameStateIndex = gameStateIndex - 1;
	_inGameStateFileOffsets.Clear();
	if(gameStateIndex > 0)
	{
		_inGameStateFileOffsets.Resize(gameStateIndex);
		for(s32 i = 0; i < gameStateIndex; ++i)
		{
			_inGameStateFileOffsets[0] = 0;
		}
	}

	for(u32 i = 0; i < PlugIns.GetSize(); ++i)
	{
		PlugIns[i]->StartProcessingDemo();
	}

	return true;
}

void udtBaseParser::ResetForGamestateMessage()
{
	_inServerMessageSequence = -1;
	_inServerCommandSequence = -1;
	_inReliableSequenceAcknowledge = -1;
	_inClientNum = -1;
	_inChecksumFeed = -1;
	_inParseEntitiesNum = 0;
	_inServerTime = S32_MIN;
	_inLastSnapshotMessageNumber = S32_MIN;

	_outServerCommandSequence = 0;
	_outSnapshotsWritten = 0;
	_outWriteFirstMessage = false;
	_outWriteMessage = false;

	memset(_inEntityBaselines, 0, MAX_PARSE_ENTITIES * _protocolSizeOfEntityState);
	memset(_inSnapshots, 0, PACKET_BACKUP * _protocolSizeOfClientSnapshot);
	for(u32 i = 0; i < (u32)MAX_GENTITIES; ++i)
	{
		_inEntityEventTimesMs[i] = S32_MIN;
	}

	_inConfigStrings.Clear();
}

void udtBaseParser::SetFilePath(const char* filePath)
{
	_inFilePath = AllocateString(GetPersistentAllocator(), filePath);

	char* fileName = NULL;
	::GetFileName(fileName, GetPersistentAllocator(), filePath);
	_inFileName = fileName;
}

void udtBaseParser::Destroy()
{
}

bool udtBaseParser::ParseNextMessage(const udtMessage& inMsg, s32 inServerMessageSequence, u32 fileOffset)
{
	_inMsg = inMsg;
	_inServerMessageSequence = inServerMessageSequence;
	_inFileOffset = fileOffset;

	return ParseServerMessage();
}

bool udtBaseParser::ParseServerMessage()
{
	_outMsg.Init(_outMsgData, sizeof(_outMsgData));
	_outMsg.Bitstream();

	_inMsg.Bitstream();

	_inReliableSequenceAcknowledge = _inMsg.ReadLong();
	_outMsg.WriteLong(_inReliableSequenceAcknowledge);

	for(;;)
	{
		if(_inMsg.Buffer.readcount > _inMsg.Buffer.cursize) 
		{
			_context->LogError("ParseServerMessage: read past the end of the server message (in file: %s)", GetFileName());
			return false;
		}

		s32 command = _inMsg.ReadByte();
		
		if(_protocol >= udtProtocol::Dm90)
		{
			// See if this is an extension command after the EOF,
			// which means we got data that a legacy client should ignore.
			if((command == svc_EOF) && (_inMsg.PeekByte() == svc_extension))
			{
				printf("!!!! Command byte: EOF with following svc_extension\n");
				_inMsg.ReadByte(); // Throw the svc_extension byte away.
				command = _inMsg.ReadByte();
				// Sometimes you get a svc_extension at end of stream...
				// dangling bits in the Huffman decoder giving a bogus value?
				if(command == -1)
				{
					command = svc_EOF;
				}
			}
		}
		
		if(command == svc_EOF) 
		{
			break;
		}

		// @NOTE: Don't write the command byte already, we leave that decision for later.

		switch(command) 
		{
		case svc_nop:
			_outMsg.WriteByte(svc_nop);
			break;

		case svc_serverCommand:
			if(!ParseCommandString()) return false;
			break;

		case svc_gamestate:
			if(!ParseGamestate()) return false;
			break;

		case svc_snapshot:
			if(!ParseSnapshot()) return false;
			break;

		case svc_voip:
			// @TODO:
			printf("!!!! Command byte: svc_voip\n");
			_outMsg.WriteByte(svc_nop);
			break;

		case svc_download:
			// @TODO:
			printf("!!!! Command byte: svc_download\n");
			_outMsg.WriteByte(svc_nop);
			break;

		default:
			_context->LogError("ParseServerMessage: unrecognized server message command byte: %d (in file: %s)", command, GetFileName());
			return false;
		}
	}

	_outMsg.WriteByte(svc_EOF);

	if(_cuts.GetSize() > 0)
	{
		const udtCutInfo cut = _cuts[0];
		const s32 gameTime = _inServerTime;

		if(_inGameStateIndex == cut.GameStateIndex && !_outWriteMessage &&
		   gameTime >= cut.StartTimeMs && gameTime <= cut.EndTimeMs)
		{
			const bool wroteMessage = _outWriteMessage;
			_outWriteMessage = true;
			_outWriteFirstMessage = _outWriteMessage && !wroteMessage;
		}
		else if((_inGameStateIndex == cut.GameStateIndex && _outWriteMessage && gameTime > cut.EndTimeMs) ||
				(_inGameStateIndex > cut.GameStateIndex && _outWriteMessage))
		{
			WriteLastMessage();
			_outWriteMessage = false;
			_outWriteFirstMessage = false;
			_outServerCommandSequence = 0;
			_outSnapshotsWritten = 0;
			_cuts[0].Stream->~udtStream();
			_cuts.Remove(0);
			if(_cuts.GetSize() == 0)
			{
				// It was the last cut, we're done parsing the file now.
				return false;
			}
		}
	}

	if(_outWriteFirstMessage)
	{
		udtCutInfo& cut = _cuts[0];
		cut.Stream = (*cut.StreamCreator)(cut.StartTimeMs, cut.EndTimeMs, cut.VeryShortDesc, this, cut.UserData);
		if(cut.Stream != NULL)
		{
			WriteFirstMessage();
			_outWriteFirstMessage = false;
		}
		else
		{
			_cuts.Remove(0);
		}
	}
	else if(_outWriteMessage)
	{
		WriteNextMessage();
	}

	return true;
}

void udtBaseParser::FinishParsing(bool /*success*/)
{
	// Close any output file stream that is still open, if any.
	if(!_cuts.IsEmpty() && _outWriteMessage)
	{
		WriteLastMessage();
		_outWriteMessage = false;
		_outWriteFirstMessage = false;
		_outServerCommandSequence = 0;
		_outSnapshotsWritten = 0;
		_cuts[0].Stream->~udtStream();
		_cuts.Clear();
	}

	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		PlugIns[i]->FinishProcessingDemo();
	}
}

void udtBaseParser::AddCut(s32 gsIndex, s32 startTimeMs, s32 endTimeMs, udtDemoStreamCreator streamCreator, const char* veryShortDesc, void* userData)
{
	udtCutInfo cut;
	cut.VeryShortDesc = veryShortDesc;
	cut.GameStateIndex = gsIndex;
	cut.StartTimeMs = startTimeMs;
	cut.EndTimeMs = endTimeMs;
	cut.Stream = NULL;
	cut.StreamCreator = streamCreator;
	cut.UserData = userData;
	_cuts.Add(cut);
}

bool udtBaseParser::ShouldWriteMessage() const
{
	return _outWriteMessage;
}

void udtBaseParser::WriteFirstMessage()
{
	WriteGameState();
	const s32 length = _outMsg.Buffer.cursize;
	udtStream* const stream = _cuts[0].Stream;
	stream->Write(&_inServerMessageSequence, 4, 1);
	stream->Write(&length, 4, 1);
	stream->Write(_outMsg.Buffer.data, length, 1);
}

void udtBaseParser::WriteNextMessage()
{
	const s32 length = _outMsg.Buffer.cursize;
	udtStream* const stream = _cuts[0].Stream;
	stream->Write(&_inServerMessageSequence, 4, 1);
	stream->Write(&length, 4, 1);
	stream->Write(_outMsg.Buffer.data, length, 1);
}

void udtBaseParser::WriteLastMessage()
{
	udtStream* const stream = _cuts[0].Stream;

	s32 length = -1;
	stream->Write(&length, 4, 1);
	stream->Write(&length, 4, 1);
	stream->Close();

	//_context->LogInfo("Stopped writing the demo.");
}

bool udtBaseParser::ParseCommandString()
{
	s32 commandStringLength = 0;
	const s32 commandSequence = _inMsg.ReadLong();
	const char* const commandStringTemp = _inMsg.ReadString(commandStringLength);

	// Do we have it already?
	if(_inServerCommandSequence >= commandSequence) 
	{
		// Yes, don't bother processing it.
		return true;
	}
	
	// Copy the string to some safe location.
	udtVMScopedStackAllocator scopedTempAllocator(GetTempAllocator());
	char* const commandString = AllocateString(GetTempAllocator(), commandStringTemp, commandStringLength);
	
	// We haven't, so let's store the last sequence number received.
	_inServerCommandSequence = commandSequence;
	
	CommandLineTokenizer& tokenizer = _context->Tokenizer;
	tokenizer.Tokenize(commandString);
	const int tokenCount = tokenizer.argc();
	if(strcmp(tokenizer.argv(0), "cs") == 0 && tokenCount == 3)
	{
		int csIndex = -1;
		if(StringParseInt(csIndex, tokenizer.argv(1)))
		{
			const char* const csStringTemp = tokenizer.argv(2);
			const u32 csStringLength = (u32)strlen(csStringTemp);

			// Copy the string to some safe location.
			char* const csString = AllocateString(GetPersistentAllocator(), csStringTemp, csStringLength);

			udtConfigString cs;
			cs.Index = csIndex;
			cs.String = csString;
			cs.StringLength = csStringLength;
			InsertOrUpdateConfigString(cs);
		}
	}

	if(!PlugIns.IsEmpty())
	{
		udtCommandCallbackArg info;
		info.CommandSequence = commandSequence;
		info.String = commandString;
		info.StringLength = commandStringLength;

		for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
		{
			PlugIns[i]->ProcessCommandMessage(info, *this);
		}
	}

	if(ShouldWriteMessage())
	{
		_outMsg.WriteByte(svc_serverCommand);
		_outMsg.WriteLong(_outServerCommandSequence);
		_outMsg.WriteString(commandString, commandStringLength);
		++_outServerCommandSequence;
	}

	return true;
}

bool udtBaseParser::ParseGamestate()
{
	// @TODO: Reset some data, but not for the 1st gamestate message.
	ResetForGamestateMessage();

	// A game state always marks a server command sequence.
	_inServerCommandSequence = _inMsg.ReadLong();

	//
	// Parse all the config strings and baselines.
	//

	for(;;)
	{
		const s32 command = _inMsg.ReadByte();

		if(command == svc_EOF)
		{
			break;
		}

		if(command == svc_configstring)
		{
			const s32 index = _inMsg.ReadShort();
			if(index < 0 || index >= MAX_CONFIGSTRINGS) 
			{
				_context->LogError("ParseGamestate: config string index out of range: %d (in file: %s)", index, GetFileName());
				return false;
			}

			s32 configStringLength = 0;
			const char* const configStringTemp = _inMsg.ReadBigString(configStringLength);
			
			// Copy the string to a safe location.
			char* const configString = AllocateString(GetPersistentAllocator(), configStringTemp, configStringLength);

			// Store it.
			udtConfigString cs;
			cs.Index = index;
			cs.String = configString;
			cs.StringLength = configStringLength;
			_inConfigStrings.Add(cs);
		} 
		else if(command == svc_baseline)
		{
			const s32 newIndex = _inMsg.ReadBits(GENTITYNUM_BITS);
			if(newIndex < 0 || newIndex >= MAX_GENTITIES) 
			{
				_context->LogError("ParseGamestate: baseline number out of range: %d (in file: %s)", newIndex, GetFileName());
				return false;
			}
			
			idLargestEntityState nullState;
			idEntityStateBase* const newState = GetBaseline(newIndex);

			// We delta from the null state because we read a full entity.
			Com_Memset(&nullState, 0, sizeof(nullState));
			_inMsg.ReadDeltaEntity(&nullState, newState, newIndex);
		} 
		else 
		{
			_context->LogError("ParseGamestate: Unrecognized command byte: %d (in file: %s)", command, GetFileName());
			return false;
		}
	}

	_inClientNum = _inMsg.ReadLong();
	_inChecksumFeed = _inMsg.ReadLong();

	if(!PlugIns.IsEmpty())
	{
		udtGamestateCallbackArg info;
		info.ServerCommandSequence = _inServerCommandSequence;
		info.ClientNum = _inClientNum;
		info.ChecksumFeed = _inChecksumFeed;

		for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
		{
			PlugIns[i]->ProcessGamestateMessage(info, *this);
		}
	}

	++_inGameStateIndex;
	_inGameStateFileOffsets.Add(_inFileOffset);

	return true;
}

bool udtBaseParser::ParseSnapshot()
{
	//
	// Read in the new snapshot to a temporary buffer
	// We will only save it if it is valid.
	// We will have read any new server commands in this
	// message before we got to svc_snapshot.
	//

	_inServerTime = _inMsg.ReadLong();

	idLargestClientSnapshot newSnap;
	Com_Memset(&newSnap, 0, sizeof(newSnap));
	newSnap.serverCommandNum = _inServerCommandSequence;
	newSnap.serverTime = _inServerTime;
	newSnap.messageNum = _inServerMessageSequence;

	//_context->LogInfo("_inServerTime: %d", _inServerTime);

	s32 deltaNum = _inMsg.ReadByte();
	if(!deltaNum) 
	{
		newSnap.deltaNum = -1;
	} 
	else 
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = _inMsg.ReadByte();

	//
	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message.
	//

	idClientSnapshotBase* oldSnap;
	if(newSnap.deltaNum <= 0) 
	{
		newSnap.valid = qtrue; // Uncompressed frame.
		oldSnap = NULL;
	} 
	else 
	{
		if(deltaNum >= PACKET_BACKUP)
		{
			_context->LogWarning("ParseSnapshot: deltaNum %d invalid.", deltaNum);
		}

		if(newSnap.deltaNum > _inServerMessageSequence)
		{
			_context->LogWarning("ParseSnapshot: Need delta from read ahead.\n");
		}

		oldSnap = GetClientSnapshot(newSnap.deltaNum & PACKET_MASK);
		if(!oldSnap->valid) 
		{
			// Should never happen.
			_context->LogWarning("ParseSnapshot: Delta from invalid frame %d (not supposed to happen!).", deltaNum);
		} 
		else if(oldSnap->messageNum != newSnap.deltaNum) 
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			_context->LogWarning("ParseSnapshot: Delta frame %d too old.", deltaNum);
		} 
		else if(_inParseEntitiesNum - oldSnap->parseEntitiesNum > MAX_PARSE_ENTITIES - 128) 
		{
			_context->LogWarning("ParseSnapshot: Delta parseEntitiesNum %d too old.", _inParseEntitiesNum);
		} 
		else 
		{
			newSnap.valid = qtrue;
		}
	}

	//
	// Read the area mask.
	//

	const s32 areaMaskLength = _inMsg.ReadByte();
	if(areaMaskLength > (s32)sizeof(newSnap.areamask))
	{
		_context->LogError("ParseSnapshot: invalid size %d for areamask (in file: %s)", areaMaskLength, GetFileName());
		return false;
	}
	_inMsg.ReadData(&newSnap.areamask, areaMaskLength);

	// Read the player info.
	_inMsg.ReadDeltaPlayerstate(oldSnap ? GetPlayerState(oldSnap, _protocol) : NULL, GetPlayerState(&newSnap, _protocol));

	// Read in all entities.
	if(!ParsePacketEntities(_inMsg, oldSnap, &newSnap))
	{
		return false;
	}

	// Did we write enough snapshots already?
	const bool noDelta = _outSnapshotsWritten < deltaNum;
	if(noDelta) 
	{
		deltaNum = 0;
		oldSnap = NULL;
	}

	//
	// Write to the output message.
	//

	if(ShouldWriteMessage())
	{
		_outMsg.WriteByte(svc_snapshot);
		_outMsg.WriteLong(newSnap.serverTime);
		_outMsg.WriteByte(deltaNum);
		_outMsg.WriteByte(newSnap.snapFlags);
		_outMsg.WriteByte(areaMaskLength);
		_outMsg.WriteData(&newSnap.areamask, areaMaskLength);
		_outMsg.WriteDeltaPlayerstate(oldSnap ? GetPlayerState(oldSnap, _protocol) : NULL, GetPlayerState(&newSnap, _protocol));
		EmitPacketEntities(deltaNum ? oldSnap : NULL, &newSnap);
		++_outSnapshotsWritten;
	}

	// If not valid, dump the entire thing now that 
	// it has been properly read.
	if(newSnap.valid == qfalse) 
	{
		return true;
	}

	//
	// Clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer.
	//

	s32 oldMessageNum = _inSnapshot.messageNum + 1;
	if(newSnap.messageNum - oldMessageNum >= PACKET_BACKUP)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP - 1);
	}

	for(; oldMessageNum < newSnap.messageNum; ++oldMessageNum) 
	{
		GetClientSnapshot(oldMessageNum & PACKET_MASK)->valid = qfalse;
	}

	// Copy to the current good spot.
	_inSnapshot = newSnap;

	// Save the frame off in the backup array for later delta comparisons.
	Com_Memcpy(GetClientSnapshot(_inSnapshot.messageNum & PACKET_MASK), &_inSnapshot, (size_t)_protocolSizeOfClientSnapshot);

	// Don't give the same stuff to the plug-ins more than once.
	if(newSnap.messageNum == _inLastSnapshotMessageNumber)
	{
		return true;
	}
	_inLastSnapshotMessageNumber = newSnap.messageNum;

	if(!PlugIns.IsEmpty())
	{
		udtSnapshotCallbackArg info;
		info.ServerTime = _inServerTime;
		info.SnapshotArrayIndex = _inSnapshot.messageNum & PACKET_MASK;
		info.Snapshot = &newSnap;
		info.Entities = _inChangedEntities.GetStartAddress();
		info.EntityCount = _inChangedEntities.GetSize();
		info.RemovedEntities = _inRemovedEntities.GetStartAddress();
		info.RemovedEntityCount = _inRemovedEntities.GetSize();

		for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
		{
			PlugIns[i]->ProcessSnapshotMessage(info, *this);
		}
	}

	return true;
}

void udtBaseParser::WriteGameState()
{
	_outMsg.Init(_outMsgData, sizeof(_outMsgData));
	_outMsg.Bitstream();

	_outMsg.WriteLong(_inReliableSequenceAcknowledge);

	_outMsg.WriteByte(svc_gamestate);
	_outMsg.WriteLong(_outServerCommandSequence);
	++_outServerCommandSequence;
	
	// Config strings.
	for(u32 i = 0, count = _inConfigStrings.GetSize(); i < count; ++i)
	{
		const udtConfigString& cs = _inConfigStrings[i];
		_outMsg.WriteByte(svc_configstring);
		_outMsg.WriteShort(cs.Index);
		_outMsg.WriteBigString(cs.String, cs.StringLength);
	}
	
	// Baseline entities.
	for(s32 i = 0; i < MAX_PARSE_ENTITIES; ++i)
	{
		// We delta from the null state because we write a full entity.
		const idEntityStateBase* const newState = GetBaseline(i);
		idLargestEntityState nullState;
		Com_Memset(&nullState, 0, sizeof(nullState));

		// Write the baseline entity if it's not filled with 0 integers.
		if(memcmp(&nullState, newState, _protocolSizeOfEntityState))
		{
			_outMsg.WriteByte(svc_baseline);

			// @NOTE: MSG_WriteBits is called in there with newState.number as an argument.
			_outMsg.WriteDeltaEntity(&nullState, newState, qtrue);
		}
	}
	
	_outMsg.WriteByte(svc_EOF);

	_outMsg.WriteLong(_inClientNum);
	_outMsg.WriteLong(_inChecksumFeed);

	_outMsg.WriteByte(svc_EOF);
}

bool udtBaseParser::ParsePacketEntities(udtMessage& msg, idClientSnapshotBase* oldframe, idClientSnapshotBase* newframe)
{
	_inChangedEntities.Clear();
	_inRemovedEntities.Clear();

	newframe->parseEntitiesNum = _inParseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	s32	oldnum;
	s32	newnum;
	s32 oldindex = 0;
	idEntityStateBase* oldstate = NULL;
	if(!oldframe) 
	{
		oldnum = 99999;
	} 
	else 
	{
		if(oldindex >= oldframe->numEntities) 
		{
			oldnum = 99999;
		} 
		else 
		{
			oldstate = GetEntity((oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1));
			oldnum = oldstate->number;
		}
	}

	for(;;)
	{
		newnum = msg.ReadBits(GENTITYNUM_BITS);

		if(newnum == (MAX_GENTITIES - 1))
		{
			break;
		}

		if(msg.Buffer.readcount > msg.Buffer.cursize) 
		{
			_context->LogError("ParsePacketEntities: read past the end of the current message (in file: %s)", GetFileName());
			return false;
		}

		while(oldnum < newnum) 
		{
			//
			// One or more entities from the old packet is unchanged.
			//

			DeltaEntity(msg, newframe, oldnum, oldstate, qtrue);
			oldindex++;

			if(oldindex >= oldframe->numEntities) 
			{
				oldnum = 99999;
			} 
			else 
			{
				oldstate = GetEntity((oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1));
				oldnum = oldstate->number;
			}
		}

		if(oldnum == newnum)
		{
			//
			// Delta from previous state.
			//

			DeltaEntity(msg, newframe, newnum, oldstate, qfalse);
			oldindex++;

			if(oldindex >= oldframe->numEntities) 
			{
				oldnum = 99999;
			} 
			else
			{
				oldstate = GetEntity((oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1));
				oldnum = oldstate->number;
			}
			continue;
		}

		if(oldnum > newnum) 
		{
			//
			// Delta from the baseline.
			//

			DeltaEntity(msg, newframe, newnum, GetBaseline(newnum), qfalse);
			continue;
		}
	}

	// Any remaining entities in the old frame are copied over.
	while(oldnum != 99999) 
	{
		//
		// One or more entities from the old packet is unchanged.
		//

		DeltaEntity(msg, newframe, oldnum, oldstate, qtrue);
		oldindex++;

		if(oldindex >= oldframe->numEntities) 
		{
			oldnum = 99999;
		} 
		else 
		{
			oldstate = GetEntity((oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1));
			oldnum = oldstate->number;
		}
	}

	return true;
}

//
// Write a delta update of an entityState_t list to the output message.
//
void udtBaseParser::EmitPacketEntities(idClientSnapshotBase* from, idClientSnapshotBase* to)
{
	idEntityStateBase* oldent = 0;
	idEntityStateBase* newent = 0;
	s32 oldindex = 0;
	s32 newindex = 0;
	s32 oldnum;
	s32 newnum;

	s32 from_num_entities;
	if(!from) 
	{
		from_num_entities = 0;
	} 
	else 
	{
		from_num_entities = from->numEntities;
	}

	while(newindex < to->numEntities || oldindex < from_num_entities) 
	{
		if(newindex >= to->numEntities) 
		{
			newnum = 9999;
		} 
		else 
		{
			s32 entNum = (to->parseEntitiesNum + newindex) & (MAX_PARSE_ENTITIES - 1);
			newent = GetEntity(entNum);
			newnum = newent->number;
		}

		if(oldindex >= from_num_entities) 
		{
			oldnum = 9999;
		}
		else 
		{
			s32 entNum = (from->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1);
			oldent = GetEntity(entNum);
			oldnum = oldent->number;
		}

		if(newnum == oldnum)
		{
			// Delta update from old position
			// because the force parameter is qfalse, this will not result
			// in any u8s being emitted if the entity has not changed at all.
			_outMsg.WriteDeltaEntity(oldent, newent, qfalse);
			oldindex++;
			newindex++;
			continue;
		}

		if(newnum < oldnum) 
		{
			// This is a new entity, send it from the baseline.
			idEntityStateBase* baseline = GetBaseline(newnum);
			_outMsg.WriteDeltaEntity(baseline, newent, qtrue);
			newindex++;
			continue;
		}

		if(newnum > oldnum) 
		{
			// The old entity isn't present in the new message.
			_outMsg.WriteDeltaEntity(oldent, 0, qtrue);
			oldindex++;
			continue;
		}
	}

	_outMsg.WriteBits(MAX_GENTITIES - 1, GENTITYNUM_BITS);
}

//
// Parses deltas from the given base and adds the resulting entity to the current frame.
//
void udtBaseParser::DeltaEntity(udtMessage& msg, idClientSnapshotBase *frame, s32 newnum, idEntityStateBase* old, qbool unchanged)
{
	// Save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta.
	idEntityStateBase* const state = GetEntity(_inParseEntitiesNum & (MAX_PARSE_ENTITIES-1));

	if(unchanged) 
	{
		Com_Memcpy(state, old, _protocolSizeOfEntityState);
	} 
	else 
	{
		if(msg.ReadDeltaEntity(old, state, newnum))
		{
			const bool isNewEvent = (state->eType >= ET_EVENTS) && (_inServerTime > _inEntityEventTimesMs[newnum] + EVENT_VALID_MSEC);
			udtChangedEntity info;
			info.Entity = state;
			info.IsNewEvent = isNewEvent;
			_inChangedEntities.Add(info);
			if(isNewEvent)
			{
				_inEntityEventTimesMs[newnum] = _inServerTime;
			}
		}
	}

	// The entity was delta removed?
	if(state->number == (MAX_GENTITIES-1)) 
	{
		// We have to return now.
		_inRemovedEntities.Add(old); // @TODO: Is this correct? Using old?
		return;	
	}

	_inParseEntitiesNum++;
	frame->numEntities++;
}

void udtBaseParser::InsertOrUpdateConfigString(const udtConfigString& newCs)
{
	udtConfigString* const oldCs = FindConfigStringByIndex(newCs.Index);
	if(oldCs == NULL)
	{
		_inConfigStrings.Add(newCs);
	}
	else
	{
		*oldCs = newCs;
	}
}

udtBaseParser::udtConfigString* udtBaseParser::FindConfigStringByIndex(s32 csIndex)
{
	for(u32 i = 0, count = _inConfigStrings.GetSize(); i < count; ++i)
	{
		udtConfigString& cs = _inConfigStrings[i];
		if(cs.Index == csIndex)
		{
			return &cs;
		}
	}

	return NULL;
}

char* udtBaseParser::AllocateString(udtVMLinearAllocator& allocator, const char* string, u32 stringLength, u32* outStringLength)
{
	if(stringLength == 0)
	{
		stringLength = (u32)strlen(string);
	}

	if(outStringLength != NULL)
	{
		*outStringLength = stringLength;
	}

	char* const stringCopy = (char*)allocator.Allocate(stringLength + 1);
	memcpy(stringCopy, string, stringLength);
	stringCopy[stringLength] = '\0';

	return stringCopy;
}

void udtBaseParser::AddPlugIn(udtBaseParserPlugIn* plugIn)
{
	PlugIns.Add(plugIn);
}
