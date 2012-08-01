#include "demo.hpp"
#include "utils.hpp"

#include <limits.h>
#include <cstdlib>


#define MSG_START_TIME	((int)1000)


Demo::Demo()
{
	_protocol = Protocol::Invalid;

	_inFile = NULL;
	_outFile = NULL;

	_gameStartTime = -1;
	_serverTime = -1;
	_serverTimeOffset = 0;
	_demoRecordStartTime = INT_MIN;
	_demoRecordEndTime = INT_MIN;
	_demoRecordLastEndTime = INT_MIN;
	_demoFirstSnapTime = -1;
	_demoLastSnapTime = -1;
	_fileStartOffset = 0;
	_lastMessageFileOffset = 0;
	_score1 = -9999;
	_score2 = -9999;
	_gameType = GameType::Unknown;
	_writeDemo = false;
	_writeFirstBlock = false;

	_inServerMessageSequence = -1;
	_inServerCommandSequence = -1;
	_inReliableSequenceAcknowledge = -1;
	_inRawSequenceAcknowledge = -1;
	_inClientNum = -1;
	_inChecksumFeed = -1;
	_inParseEntitiesNum = 0;
	_inNewSnapshots = qfalse;

	_inGamestateMsgSaved = false;
	MSG_Init(&_inGamestateMsg, _inGamestateMsgData, sizeof(_inGamestateMsgData));

	_outServerCommandSequence = 0;
	_outServerMessageSequence = 0;
	_outSnapshotsWritten = 0;

	_mapName = "";
}

Demo::~Demo()
{
}

void Demo::CloseFiles()
{
	if(_inFile)
	{
		fclose(_inFile);
		_inFile = NULL;
	}

	if(_outFile)
	{
		fclose(_outFile);
		_outFile = NULL;
	}
}

void Demo::DemoCompleted()
{
	if(_writeDemo && !_writeFirstBlock)
	{
		WriteFileEnd();
		_writeDemo = false;
	}
}

void Demo::FixLastGameStateLastSnapshotTime()
{
	if(_gameStates.size() == 0)
	{
		return;
	}

	const size_t lastIdx = _gameStates.size() - 1;
	_gameStates[lastIdx].LastSnapshotTime = _demoLastSnapTime;
}

void Demo::FixLastGameStateFirstSnapshotTime()
{
	if(_gameStates.size() == 0)
	{
		return;
	}

	const size_t lastIdx = _gameStates.size() - 1;
	_gameStates[lastIdx].FirstSnapshotTime = _demoFirstSnapTime;
}

void Demo::FixLastGameStateServerTimeOffset()
{
}

void Demo::NewGamestate()
{
	FixLastGameStateLastSnapshotTime();

	GameStateInfo info;
	info.FileOffset = _lastMessageFileOffset;
	info.ServerTimeOffset = 0;
	info.FirstSnapshotTime = -1;
	info.LastSnapshotTime = -1;
	_gameStates.push_back(info);

	if(_demoRecordStartTime < 0 && _demoRecordEndTime < 0 && _demoRecordLastEndTime < 0)
	{
		_demoFirstSnapTime = -1;
		_demoLastSnapTime = -1;
	}
}

bool Demo::Do()
{
	if(_protocol == Protocol::Invalid)
	{
		LogError("Unrecognized protocol: '%s'", _inFilePath.c_str());
		return false;
	}

	//
	// Prepare data that does not depend on the protocol version.
	//

	_inSnapshots.resize(PACKET_BACKUP);
	_inEntityEvents.resize(MAX_PARSE_ENTITIES);
	memset(&_inEntityEvents[0], 0, MAX_PARSE_ENTITIES * sizeof(EntityEventInfo));

	_players.resize(MAX_CLIENTS);
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		_players[i].Valid = false;
		_players[i].Info.Team = -9999;
		_players[i].Info.Handicap = -9999;
		_players[i].Info.BotSkill = -9999;
		_players[i].Info.Health = -9999;
		_players[i].Info.Armor = -9999;
		_players[i].Info.CurrentWeapon = -9999;
		_players[i].Info.CurrentAmmo = -9999;
		_players[i].Info.Rank = -9999;
		_players[i].Info.Score = -9999;
	}

	_entities.resize(MAX_GENTITIES);
	for(int i = 0; i < MAX_GENTITIES; ++i)
	{
		_entities[i].Valid = false;
		_entities[i].Index = 0;
	}

	// Prepare data that depends on the protocol version.
	ProtocolInit();

	// Set up the record range.
	_demoRecordLastEndTime = _demoRecordEndTime;
	if(_cuts.size() > 0)
	{
		_outFilePath = _cuts[0].FilePath;
		_demoRecordStartTime = _cuts[0].StartTimeMs;
		_demoRecordEndTime = _cuts[0].EndTimeMs;
		_demoRecordLastEndTime = _cuts[_cuts.size() - 1].EndTimeMs;
	}

	_inFile = fopen(_inFilePath.c_str(), "rb");
	if(_inFile == NULL)
	{
		LogError("Failed to open input file: '%s'", _inFilePath.c_str());
		return false;
	}

	fseek(_inFile, 0, SEEK_END);
	const long fileBytesTotal = ftell(_inFile);
	fseek(_inFile, (long)_fileStartOffset, SEEK_SET);

	for(;;)
	{
		_lastMessageFileOffset = ftell(_inFile);

		size_t elementsRead = 0;
		elementsRead = fread(&_inServerMessageSequence, 4, 1, _inFile);
		if(elementsRead != 1)
		{
			CloseFiles();
			return false;
		}

		MSG_Init(&_inMsg, _inMsgData, sizeof(_inMsgData));
		MSG_Init(&_outMsg, _outMsgData, sizeof(_outMsgData));

		elementsRead = fread(&_inMsg.cursize, 4, 1, _inFile);
		if(elementsRead != 1)
		{
			CloseFiles();
			return false;
		}

		if(_inMsg.cursize == -1) 
		{
			DemoCompleted();
			FixLastGameStateLastSnapshotTime();
			CloseFiles();
			FinishParsing();
			return true;
		}

		if(_inMsg.cursize > _inMsg.maxsize)
		{
			LogError("Demo message length > MAX_SIZE");
			CloseFiles();
			FinishParsing();
			return false;
		}

		elementsRead = fread(_inMsg.data, _inMsg.cursize, 1, _inFile);
		if(elementsRead != 1)
		{
			LogWarning("Demo file was truncated.");
			DemoCompleted();
			FixLastGameStateLastSnapshotTime();
			CloseFiles();
			FinishParsing();
			return true;
		}

		_inMsg.readcount = 0;
		ParseServerMessage(&_inMsg, &_outMsg);

		//
		// Handle demo recording logic.
		//

		const int gameTime = GetVirtualInputTime();
		if(!_writeDemo && gameTime >= _demoRecordStartTime && gameTime < _demoRecordEndTime)
		{
			_writeDemo = true;
			_writeFirstBlock = true;
		}
		else if(_writeDemo && gameTime > _demoRecordEndTime)
		{
			_writeDemo = false;
			WriteFileEnd();
			if(_cuts.size() > 0)
			{
				_cuts.erase(_cuts.begin());				
			}
			if(_cuts.size() > 0)
			{
				_outFilePath = _cuts[0].FilePath;
				_demoRecordStartTime = _cuts[0].StartTimeMs;
				_demoRecordEndTime = _cuts[0].EndTimeMs;
				_outServerCommandSequence = 0;
				_outServerMessageSequence = 0;
				_outSnapshotsWritten = 0;
			}
			else
			{
				break;
			}
		}

		if(_writeDemo)
		{
			if(_writeFirstBlock)
			{
				WriteFileStart();
				_writeFirstBlock = false;
			}
			else
			{
				WriteFileBlock(&_outMsg);
			}
		}

		const long fileBytesRead = ftell(_inFile);
		if(_progressCallback && _demoRecordStartTime >= 0 && _demoRecordLastEndTime >= 0)
		{
			float cutProgress = (float)(_demoLastSnapTime - _demoFirstSnapTime) / (float)(_demoRecordLastEndTime - _serverTimeOffset - _demoFirstSnapTime);
			cutProgress = std::min(cutProgress, 1.0f);
			cutProgress = std::max(cutProgress, 0.0f);
			const int cancelNow = (*_progressCallback)(cutProgress);
			if(cancelNow)
			{
				CloseFiles();
				return false;
			}
		}
		else if(_progressCallback && fileBytesTotal > 0 && fileBytesRead > 0)
		{
			float cutProgress = (float)fileBytesRead / (float)fileBytesTotal;
			cutProgress = std::min(cutProgress, 1.0f);
			cutProgress = std::max(cutProgress, 0.0f);
			const int cancelNow = (*_progressCallback)(cutProgress);
			if(cancelNow)
			{
				CloseFiles();
				return false;
			}
		}
	}

	FixLastGameStateLastSnapshotTime();
	CloseFiles();
	FinishParsing();

	return true;
}

// Only called when Demo::Do returns true.
void Demo::FinishParsing()
{
	// @FIXME: Fix the pu runs' analysis...
	if(_protocol == Protocol::Dm73)
	{
		_puRuns.clear();
	}
}

void Demo::WriteFileBlock(msg_t* msgOut)
{
	// Finished writing the client packet.
	MSG_WriteByte(&_outMsg, svc_EOF);

	const int length = msgOut->cursize;
	fwrite(&_outServerMessageSequence, 4, 1, _outFile);
	fwrite(&length, 4, 1, _outFile);
	fwrite(msgOut->data, length, 1, _outFile);
	++_outServerMessageSequence;
}

void Demo::WriteFileStart()
{
	_outFile = fopen(_outFilePath.c_str(), "wb");
	if(_outFile == NULL)
	{
		CloseFiles();
		LogError("Failed to open the output file.");
		return;
	}

	LogInfo("Started recording a demo to '%s'", _outFilePath.c_str());

	MSG_Init(&_outGamestateMsg, _outGamestateMsgData, sizeof(_outGamestateMsgData));
	MSG_Bitstream(&_outGamestateMsg);

	_inGamestateMsg.readcount = 0;
	_inGamestateMsg.bit = 0;

	ParseServerMessage(&_inGamestateMsg, &_outGamestateMsg);

	WriteFileBlock(&_outGamestateMsg);
	WriteFileBlock(&_outMsg);
}

void Demo::WriteFileEnd()
{
	int length = -1;
	fwrite(&length, 4, 1, _outFile);
	fwrite(&length, 4, 1, _outFile);
	LogInfo("Stopped recording to '%s'", _outFilePath.c_str());

	if(_outFile)
	{
		fclose(_outFile);
		_outFile = NULL;
	}
}

void Demo::ParseServerMessage(msg_t* msg, msg_t* msgOut)
{
	MSG_Bitstream(msg);

	_inReliableSequenceAcknowledge = MSG_ReadLong(msg);
	MSG_WriteLong(msgOut, _inReliableSequenceAcknowledge);
	_inRawSequenceAcknowledge = _inReliableSequenceAcknowledge;
	
	if(_inReliableSequenceAcknowledge < _inServerCommandSequence - MAX_RELIABLE_COMMANDS) 
	{
		_inReliableSequenceAcknowledge = _inServerCommandSequence;
	}

	for(;;)
	{
		if(msg->readcount > msg->cursize) 
		{
			CloseFiles();
			LogErrorAndCrash("ParseServerMessage: read past the end of the server message");
			break;
		}

		const int command = MSG_ReadByte(msg);
		if(command == svc_EOF) 
		{
			break;
		}

		// @NOTE: Don't write the command byte already, we leave that decision for later.

		switch(command) 
		{
		case svc_nop:
			MSG_WriteByte(msgOut, svc_nop);
			break;

		case svc_serverCommand:
			ParseCommandString(msg, msgOut);
			break;
			
		case svc_gamestate:
			NewGamestate();
			if(!_inGamestateMsgSaved)
			{
				_inGamestateMsg.allowoverflow = msg->allowoverflow;
				_inGamestateMsg.overflowed = msg->overflowed;
				_inGamestateMsg.maxsize = msg->maxsize;
				_inGamestateMsg.cursize = msg->cursize;
				_inGamestateMsg.readcount = msg->readcount;
				_inGamestateMsg.bit = msg->bit;

				memcpy(_inGamestateMsg.data, msg->data, MAX_MSGLEN);
				_inGamestateMsgSaved = true;
			}
			ParseGamestate(msg, msgOut);
			break;
			
		case svc_snapshot:
			ParseSnapshot(msg, msgOut);
			break;
			
		case svc_download:
			ParseDownload(msg, msgOut);
			break;

		default:
			CloseFiles();
			LogErrorAndCrash("ParseServerMessage: unrecognized server message command byte" );
			break;
		}
	}

	MSG_WriteByte(msgOut, svc_EOF);
}

bool Demo::ShouldWriteDemoMessages() const
{
	return _demoRecordStartTime >= 0 && _demoRecordEndTime >= 0;
}

void Demo::ParseCommandString(msg_t* msg, msg_t* msgOut)
{
	const int commandSequence = MSG_ReadLong(msg);
	const char* const command = MSG_ReadString(msg);

	// Do we have it already?
	if(_inServerCommandSequence >= commandSequence) 
	{
		// Yes, don't bother writing it.
		return;
	}

	std::string fixedCommand;
	ProtocolAnalyzeAndFixCommandString(command, fixedCommand);

	// We haven't, so let's store the last sequence number received...
	_inServerCommandSequence = commandSequence;

	// ...and the command as well.
	_inCommands[commandSequence] = fixedCommand;
	
	if(ShouldWriteDemoMessages())
	{
		MSG_WriteByte(msgOut, svc_serverCommand);
		MSG_WriteLong(msgOut, _outServerCommandSequence);
		MSG_WriteString(msgOut, fixedCommand.c_str());
		if(_writeDemo) 
		{
			++_outServerCommandSequence;
		}
	}
}

void Demo::ParseGamestate(msg_t* msg, msg_t* msgOut)
{
	// A game state always marks a server command sequence.
	_inServerCommandSequence = MSG_ReadLong(msg);

	MSG_WriteByte(msgOut, svc_gamestate);
	MSG_WriteLong(msgOut, _outServerCommandSequence);
	if(_writeDemo) 
	{
		++_outServerCommandSequence;
	}

	//
	// Parse all the config strings and baselines.
	//

	for(;;)
	{
		const int command = MSG_ReadByte(msg);
		MSG_WriteByte(msgOut, command);

		if(command == svc_EOF)
		{
			break;
		}

		if(command == svc_configstring)
		{
			const int index = MSG_ReadShort(msg);
			if(index < 0 || index >= MAX_CONFIGSTRINGS) 
			{
				CloseFiles();
				LogErrorAndCrash("ParseGamestate: Config string index out of range: %i", index);
			}

			std::string configString = MSG_ReadBigString(msg);
			if(_writeFirstBlock && ShouldWriteDemoMessages() && _inConfigStrings.find(index) != _inConfigStrings.end())
			{
				configString = _inConfigStrings[index];
			}

			// Get map name
			if(_mapName.empty())
			{
				GetVariable(_mapName, configString, "mapname");
			}

			// Fix the command... Warm-up and game times for instance.
			std::string fixedConfigString;
			ProtocolAnalyzeConfigString(index, configString);
			ProtocolFixConfigString(index, configString, fixedConfigString);

			// Store it.
			_inConfigStrings[index] = fixedConfigString;

			if(ShouldWriteDemoMessages())
			{
				MSG_WriteShort(msgOut, index);
				MSG_WriteBigString(msgOut, fixedConfigString.c_str());
			}
		} 
		else if(command == svc_baseline)
		{
			ProtocolParseBaseline(msg, msgOut);
		} 
		else 
		{
			CloseFiles();
			LogErrorAndCrash("ParseGamestate: Unrecognized command byte");
		}
	}

	_inClientNum = MSG_ReadLong(msg);
	_inChecksumFeed = MSG_ReadLong(msg);
	MSG_WriteLong(msgOut, _inClientNum);
	MSG_WriteLong(msgOut, _inChecksumFeed);
}

void Demo::ParseSnapshot(msg_t* msg, msg_t* msgOut)
{
	//
	// Read in the new snapshot to a temporary buffer
	// We will only save it if it is valid.
	// We will have read any new server commands in this
	// message before we got to svc_snapshot.
	//

	clSnapshot_t newSnap;
	Com_Memset(&newSnap, 0, sizeof(newSnap));
	newSnap.serverCommandNum = _inServerCommandSequence;
	newSnap.serverTime = MSG_ReadLong(msg);
	newSnap.messageNum = _inServerMessageSequence;
	SetServerTime(newSnap.serverTime);
	SetGameTime(newSnap.serverTime);

	int deltaNum = MSG_ReadByte(msg);
	if(!deltaNum) 
	{
		newSnap.deltaNum = -1;
	} 
	else 
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = MSG_ReadByte(msg);

	//
	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message.
	//

	clSnapshot_t* oldSnap;
	if(newSnap.deltaNum <= 0) 
	{
		newSnap.valid = qtrue; // Uncompressed frame.
		oldSnap = NULL;
	} 
	else 
	{
		oldSnap = &_inSnapshots[newSnap.deltaNum & PACKET_MASK];
		if(!oldSnap->valid) 
		{
			// Should never happen. Else what? ahgahaghaghagahgahaghaaha!!!!!!!!!
			LogWarning("ParseSnapshot: Delta from invalid frame %d (not supposed to happen!).", deltaNum);
		} 
		else if(oldSnap->messageNum != newSnap.deltaNum) 
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			LogWarning("ParseSnapshot: Delta frame %d too old.", deltaNum);
		} 
		else if(_inParseEntitiesNum - oldSnap->parseEntitiesNum > MAX_PARSE_ENTITIES - 128) 
		{
			LogWarning("ParseSnapshot: Delta parseEntitiesNum %d too old.", _inParseEntitiesNum);
		} 
		else 
		{
			newSnap.valid = qtrue;
		}
	}
	
	//
	// Read the area mask.
	//

	const int areaMaskLength = MSG_ReadByte(msg);
	if(areaMaskLength > (int)sizeof(newSnap.areamask))
	{
		CloseFiles();
		LogErrorAndCrash("ParseSnapshot: Invalid size %d for areamask.", areaMaskLength);
		return;
	}
	MSG_ReadData(msg, &newSnap.areamask, areaMaskLength);

	// Read the player info.
	MSG_ReadDeltaPlayerstate(msg, oldSnap ? &oldSnap->ps : NULL, &newSnap.ps);

	// Read in all entities.
	ProtocolParsePacketEntities(msg, msgOut, oldSnap, &newSnap);

	// Analyze the snapshot...
	ProtocolAnalyzeSnapshot(oldSnap, &newSnap);

	// Did we write enough snapshots already?
	const bool noDelta = _outSnapshotsWritten < deltaNum;
	if(noDelta) 
	{
		deltaNum = 0;
		oldSnap = NULL;
	}

	newSnap.ps.commandTime = GetFixedOutputTime(newSnap.ps.commandTime);

	//
	// Write to the output message.
	//

	if(ShouldWriteDemoMessages())
	{
		int serverTime = GetFixedOutputTime(newSnap.serverTime);
		MSG_WriteByte(msgOut, svc_snapshot);
		MSG_WriteLong(msgOut, serverTime);
		MSG_WriteByte(msgOut, deltaNum);
		MSG_WriteByte(msgOut, newSnap.snapFlags);
		MSG_WriteByte(msgOut, areaMaskLength);
		MSG_WriteData(msgOut, &newSnap.areamask, areaMaskLength);
		FixPlayerState(oldSnap ? &oldSnap->ps : NULL, &newSnap.ps);
		MSG_WriteDeltaPlayerstate(msgOut, oldSnap ? &oldSnap->ps : NULL, &newSnap.ps);
		ProtocolEmitPacketEntities(deltaNum ? oldSnap : NULL, &newSnap);
		if(_writeDemo)
		{
			++_outSnapshotsWritten;
		}
	}

	// If not valid, dump the entire thing now that 
	// it has been properly read.
	if(newSnap.valid == qfalse) 
	{
		return;
	}

	AnalyzePlayerState(oldSnap ? &oldSnap->ps : NULL, &newSnap.ps);
	
	//
	// Clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer.
	//

	int oldMessageNum = _inSnapshot.messageNum + 1;
	if(newSnap.messageNum - oldMessageNum >= PACKET_BACKUP)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP - 1);
	}

	for(; oldMessageNum < newSnap.messageNum; ++oldMessageNum) 
	{
		_inSnapshots[oldMessageNum & PACKET_MASK].valid = qfalse;
	}

	// Copy to the current good spot.
	_inSnapshot = newSnap;

	// Save the frame off in the backup array for later delta comparisons.
	_inSnapshots[_inSnapshot.messageNum & PACKET_MASK] = _inSnapshot;

	_inNewSnapshots = qtrue;
}

void Demo::AnalyzePlayerState(playerState_t* oldState, playerState_t* newState)
{
	playerState_t nullState;
	if(!oldState)
	{
		oldState = &nullState;
		memset(&nullState, 0, sizeof(nullState));
	}

	const int runCount = (int)_puRuns.size();
	if(runCount >= 10)
	{
		return;
	}
	for(int i = PW_FIRST; i <= PW_LAST; ++i)
	{
		const int newTime = newState->powerups[i];	
		if(newTime == 0)
		{
			continue;
		}
		
		bool old = false;
		for(int j = 0; j < runCount; ++j)
		{
			if(_puRuns[j].PredictedEndTime == newTime)
			{
				old = true;
				break;
			}
		}

		if(old)
		{
			continue;
		}
		
		if(newState->clientNum < 0 || newState->clientNum >= MAX_CLIENTS)
		{
			continue;
		}

		PuRunInfo info;
		info.PlayerName = _players[newState->clientNum].Name;
		info.Player = newState->clientNum;
		info.VirtualServerTime = GetVirtualInputTime();
		info.Duration = 0;
		info.Ended = 0;
		info.Kills = 0;
		info.PredictedEndTime = newTime;
		info.Pu = i;
		info.SelfKill = 0;
		info.TeamKills = 0;
		_puRuns.push_back(info);
	}
}

void Demo::ParseDownload(msg_t* /*msg*/, msg_t* msgOut)
{
	// Ignore it completely.
	MSG_WriteByte(msgOut, svc_nop);
}

void Demo::ExtractPlayerNameFromConfigString(std::string& playerName, const std::string& configString)
{
	// Find the name.
	std::string nameWithColors;
	GetVariable(nameWithColors, configString, "n");

	// Clear the result.
	playerName.clear();

	// Get rid of the color codes.
	const char* s = nameWithColors.c_str();
	while(*s != '\0')
	{
		char c0 = s[0];
		char c1 = s[1];
		if(c0 == '^' && c1 != '\0' && c1 != ' ')
		{
			++s;
		}
		else
		{
			playerName += c0;
		}

		++s;
	}
}

bool Demo::ExtractConfigStringFromCommand(int& csIndex, std::string& configString, const char* command)
{
	csIndex = -1;
	configString = "";

	if(strstr(command, "cs ") != command)
	{
		return false;
	}

	const int idx = atoi(command + 3);
	if(idx < 0 || idx >= MAX_CONFIGSTRINGS)
	{
		return false;
	}

	csIndex = idx;

	const std::string commandString = command;
	const size_t idx1 = commandString.find('\"');
	if(idx1 == std::string::npos)
	{
		return false;
	}

	const size_t idx2 = commandString.find_last_of('\"');
	if(idx2 == std::string::npos)
	{
		return false;
	}

	configString = commandString.substr(idx1 + 1, idx2 - idx1 - 1);

	return true;
}

void Demo::SetServerTime(int sTime)
{
	if(_serverTime >= 0 && sTime < _serverTime)
	{
		_serverTimeOffset += _serverTime;
		FixLastGameStateServerTimeOffset();
	}
	
	_serverTime = sTime;
}

void Demo::SetGameTime(int sTime)
{
	if(_demoFirstSnapTime < 0 || (sTime > 0 && sTime < _demoFirstSnapTime)) 
	{
		_demoFirstSnapTime = sTime;
		FixLastGameStateFirstSnapshotTime();
	}

	_demoLastSnapTime = sTime;
}

void Demo::FixPlayerState(playerState_t* oldState, playerState_t* newState)
{
	playerState_t nullState;
	if(!oldState)
	{
		oldState = &nullState;
		memset(&nullState, 0, sizeof(nullState));
	}
	
	FixPlayerStatePowerUp(oldState, newState, PW_QUAD);
	FixPlayerStatePowerUp(oldState, newState, PW_BATTLESUIT);
	FixPlayerStatePowerUp(oldState, newState, PW_INVIS);
	FixPlayerStatePowerUp(oldState, newState, PW_REGEN);
	FixPlayerStatePowerUp(oldState, newState, PW_FLIGHT);
	FixPlayerStatePowerUp(oldState, newState, PW_REDFLAG);
	FixPlayerStatePowerUp(oldState, newState, PW_BLUEFLAG);
	FixPlayerStatePowerUp(oldState, newState, PW_NEUTRALFLAG);
	FixPlayerStatePowerUp(oldState, newState, PW_SCOUT);
	FixPlayerStatePowerUp(oldState, newState, PW_GUARD);
	FixPlayerStatePowerUp(oldState, newState, PW_DOUBLER);
	FixPlayerStatePowerUp(oldState, newState, PW_AMMOREGEN);
	FixPlayerStatePowerUp(oldState, newState, PW_INVULNERABILITY);
}

void Demo::FixPlayerStatePowerUp(playerState_t* oldState, playerState_t* newState, int powerUpIdx)
{
	const int& oldTime = oldState->powerups[powerUpIdx];
	int& newTime = newState->powerups[powerUpIdx];

	if(newTime != oldTime && newTime != 0)
	{
		newTime = GetFixedOutputTime(newTime);
	}
}

int Demo::GetRealInputTime()
{
	return _serverTime;
}

int Demo::GetVirtualInputTime()
{
	return _serverTime + _serverTimeOffset;
}

int Demo::GetFixedOutputTime(int time)
{
	return time + MSG_START_TIME;
}

int Demo::GetFixedTwTs(int time)
{
	return time + MSG_START_TIME;
}