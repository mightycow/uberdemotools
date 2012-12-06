#include "api.h"
#include "demo.hpp"
#include "demo68.hpp"
#include "demo73.hpp"
#include "cutter.hpp"

#include <limits.h>


#define UDT_API(ReturnType) UDT_EXPORT_DLL ReturnType


static const char* VersionString = "0.2.1";


struct UdtLibrary
{
	Demo* demo;
};


UDT_API(const char*) udtGetVersionString()
{
	return VersionString;
}

UDT_API(UdtHandle) udtCreate(int protocolId)
{
	if(protocolId != Protocol::Dm68 && protocolId != Protocol::Dm73)
	{
		return NULL;
	}

	UdtLibrary* lib = new UdtLibrary;
	switch(protocolId)
	{
	case Protocol::Dm68:
		lib->demo = new Demo68;
		break;

	case Protocol::Dm73:
		lib->demo = new Demo73;
		break;

	default:
		delete lib;
		return NULL;
	}

	lib->demo->_protocol = (Protocol::Id)protocolId;

	return lib;
}

UDT_API(void) udtDestroy(UdtHandle lib)
{
	if(lib)
	{
		if(lib->demo)
		{
			delete lib->demo;
		}

		delete lib;
	}

	_progressCallback = NULL;
	_messageCallback = NULL;
}

UDT_API(int) udtSetProgressCallback(UdtHandle lib, ProgressCallback callback)
{
	if(lib == NULL || callback == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	_progressCallback = callback;

	return ErrorCode::None;
}

UDT_API(int) udtSetMessageCallback(UdtHandle lib, MessageCallback callback)
{
	if(lib == NULL || callback == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	_messageCallback = callback;

	return ErrorCode::None;
}

UDT_API(int) udtSetFileStartOffset(UdtHandle lib, int fileStartOffset)
{
	if(lib == NULL || fileStartOffset < 0)
	{
		return ErrorCode::InvalidArgument;
	}

	lib->demo->_fileStartOffset = fileStartOffset;

	return ErrorCode::None;
}

UDT_API(int) udtSetServerTimeOffset(UdtHandle lib, int serverTimeOffset)
{
	if(lib == NULL || serverTimeOffset < 0)
	{
		return ErrorCode::InvalidArgument;
	}

	lib->demo->_serverTimeOffset = serverTimeOffset;

	return ErrorCode::None;
}

UDT_API(int) udtAddCut(UdtHandle lib, const char* filePath, int startTimeMs, int endTimeMs)
{
	if(lib == NULL || filePath == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	DemoCut cut;
	cut.StartTimeMs = startTimeMs;
	cut.EndTimeMs = endTimeMs;
	cut.FilePath = filePath;
	lib->demo->_cuts.push_back(cut);

	return ErrorCode::None;
}

UDT_API(int) udtParse(UdtHandle lib, const char* filePath, ProgressCallback progressCb, MessageCallback messageCb)
{
	if(lib == NULL || filePath == NULL)
	{
		return ErrorCode::InvalidArgument;
	}
	
	_progressCallback = progressCb;
	_messageCallback = messageCb;

	lib->demo->_inFilePath = filePath;
	lib->demo->_outFilePath = "[invalid]";
	lib->demo->_demoRecordStartTime = INT_MIN;
	lib->demo->_demoRecordEndTime = INT_MIN;
	lib->demo->Do();
	
	return ErrorCode::None;
}

UDT_API(int) udtGetWarmupEndTimeMs(UdtHandle lib, int* warmupEndTimeMs)
{
	if(lib == NULL || warmupEndTimeMs == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	*warmupEndTimeMs = lib->demo->_gameStartTime;

	return ErrorCode::None;
}

UDT_API(int) udtGetFirstSnapshotTimeMs(UdtHandle lib, int* firstSnapshotTimeMs)
{
	if(lib == NULL || firstSnapshotTimeMs == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	*firstSnapshotTimeMs = lib->demo->_demoFirstSnapTime;

	return ErrorCode::None;
}

UDT_API(int) udtGetGameStateCount(UdtHandle lib, int* gameStateCount)
{
	if(lib == NULL || gameStateCount == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	*gameStateCount = (int)lib->demo->_gameStates.size();

	return ErrorCode::None;
}

UDT_API(int) udtGetGameStateFileOffset(UdtHandle lib, int gameStateIdx, int* fileOffset)
{
	if(lib == NULL || fileOffset == NULL || gameStateIdx < 0 || gameStateIdx >= (int)lib->demo->_gameStates.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*fileOffset = (int)lib->demo->_gameStates[gameStateIdx].FileOffset;

	return ErrorCode::None;
}

UDT_API(int) udtGetGameStateServerTimeOffset(UdtHandle lib, int gameStateIdx, int* serverTimeOffset)
{
	if(lib == NULL || serverTimeOffset == NULL || gameStateIdx < 0 || gameStateIdx >= (int)lib->demo->_gameStates.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*serverTimeOffset = (int)lib->demo->_gameStates[gameStateIdx].ServerTimeOffset;

	return ErrorCode::None;
}

UDT_API(int) udtGetGameStateFirstSnapshotTime(UdtHandle lib, int gameStateIdx, int* firstSnapshotTime)
{
	if(lib == NULL || firstSnapshotTime == NULL || gameStateIdx < 0 || gameStateIdx >= (int)lib->demo->_gameStates.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*firstSnapshotTime = (int)lib->demo->_gameStates[gameStateIdx].FirstSnapshotTime;

	return ErrorCode::None;
}

UDT_API(int) udtGetGameStateLastSnapshotTime(UdtHandle lib, int gameStateIdx, int* lastSnapshotTime)
{
	if(lib == NULL || lastSnapshotTime == NULL || gameStateIdx < 0 || gameStateIdx >= (int)lib->demo->_gameStates.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*lastSnapshotTime = (int)lib->demo->_gameStates[gameStateIdx].LastSnapshotTime;

	return ErrorCode::None;
}

UDT_API(int) udtGetServerCommandCount(UdtHandle lib, int* cmdCount)
{
	if(lib == NULL || cmdCount == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	*cmdCount = (int)lib->demo->_inCommands.size();

	return ErrorCode::None;
}

UDT_API(int) udtGetServerCommandSequence(UdtHandle lib, int cmdIndex, int* seqNumber)
{
	if(lib == NULL || seqNumber == NULL || cmdIndex < 0 || cmdIndex >= (int)lib->demo->_inCommands.size())
	{
		return ErrorCode::InvalidArgument;
	}

	Demo::ChatMap::const_iterator it = lib->demo->_inCommands.begin();
	for(int i = 0; i < cmdIndex; ++i)
	{
		++it;
	}

	*seqNumber = it->first;

	return ErrorCode::None;
}

UDT_API(int) udtGetServerCommandMessage(UdtHandle lib, int cmdIndex, char* valueBuffer, int bufferLength)
{
	if(lib == NULL || valueBuffer == NULL || bufferLength <= 0 || cmdIndex < 0 || cmdIndex >= (int)lib->demo->_inCommands.size())
	{
		return ErrorCode::InvalidArgument;
	}

	Demo::ChatMap::const_iterator it = lib->demo->_inCommands.begin();
	for(int i = 0; i < cmdIndex; ++i)
	{
		++it;
	}

	Q_strncpyz(valueBuffer, it->second.c_str(), bufferLength);

	return ErrorCode::None;
}

UDT_API(int) udtGetConfigStringCount(UdtHandle lib, int* csCount)
{
	if(lib == NULL || csCount == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	*csCount = (int)lib->demo->_inConfigStrings.size();

	return ErrorCode::None;
}

UDT_API(int) udtGetConfigStringValue(UdtHandle lib, int csIndex, char* valueBuffer, int bufferLength)
{
	if(lib == NULL || valueBuffer == NULL || bufferLength <= 0 || csIndex < 0 || csIndex >= MAX_CONFIGSTRINGS)
	{
		return ErrorCode::InvalidArgument;
	}

	Demo::ChatMap::const_iterator it = lib->demo->_inConfigStrings.begin();
	for(int i = 0; i < csIndex; ++i)
	{
		++it;
	}

	Q_strncpyz(valueBuffer, it->second.c_str(), bufferLength);

	return ErrorCode::None;
}

UDT_API(int) udtGetConfigStringIndex(UdtHandle lib, int udtCsIndex, int* quakeCsIndex)
{
	if(lib == NULL || quakeCsIndex == NULL || udtCsIndex < 0 || udtCsIndex >= (int)lib->demo->_inConfigStrings.size())
	{
		return ErrorCode::InvalidArgument;
	}

	Demo::ChatMap::const_iterator it = lib->demo->_inConfigStrings.begin();
	for(int i = 0; i < udtCsIndex; ++i)
	{
		++it;
	}

	*quakeCsIndex = it->first;

	return ErrorCode::None;
}

UDT_API(int) udtGetChatStringCount(UdtHandle lib, int* chatCount)
{
	if(lib == NULL || chatCount == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	*chatCount = (int)lib->demo->_chatMessages.size();

	return ErrorCode::None;
}

UDT_API(int) udtGetChatStringMessage(UdtHandle lib, int chatIndex, char* valueBuffer, int bufferLength)
{
	if(lib == NULL || valueBuffer == NULL || bufferLength <= 0 || chatIndex < 0 || chatIndex >= (int)lib->demo->_chatMessages.size())
	{
		return ErrorCode::InvalidArgument;
	}

	Demo::ChatMap::const_iterator it = lib->demo->_chatMessages.begin();
	for(int i = 0; i < chatIndex; ++i)
	{
		++it;
	}

	Q_strncpyz(valueBuffer, it->second.c_str(), bufferLength);

	return ErrorCode::None;
}

UDT_API(int) udtGetChatStringTime(UdtHandle lib, int chatIndex, int* time)
{
	if(lib == NULL || time == NULL || chatIndex < 0 || chatIndex >= (int)lib->demo->_chatMessages.size())
	{
		return ErrorCode::InvalidArgument;
	}

	Demo::ChatMap::const_iterator it = lib->demo->_chatMessages.begin();
	for(int i = 0; i < chatIndex; ++i)
	{
		++it;
	}

	*time = it->first;

	return ErrorCode::None;
}

UDT_API(int) udtGetObituaryCount(UdtHandle lib, int* obituaryCount)
{
	if(lib == NULL || obituaryCount == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	*obituaryCount = (int)lib->demo->_obituaries.size();

	return ErrorCode::None;
}

UDT_API(int) udtGetObituaryTime(UdtHandle lib, int obituaryIndex, int* time)
{
	if(lib == NULL || time == NULL || obituaryIndex < 0 || obituaryIndex >= (int)lib->demo->_obituaries.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*time = lib->demo->_obituaries[obituaryIndex].VirtualServerTime;

	return ErrorCode::None;
}

UDT_API(int) udtGetObituaryAttackerName(UdtHandle lib, int obituaryIndex, char* nameBuffer, int bufferLength)
{
	if(lib == NULL || nameBuffer == NULL || bufferLength <= 0 || obituaryIndex < 0 || obituaryIndex >= (int)lib->demo->_obituaries.size())
	{
		return ErrorCode::InvalidArgument;
	}

	Q_strncpyz(nameBuffer, lib->demo->_obituaries[obituaryIndex].AttackerName.c_str(), bufferLength);

	return ErrorCode::None;
}

// Get the target player name of an obituary event.
UDT_API(int) udtGetObituaryTargetName(UdtHandle lib, int obituaryIndex, char* nameBuffer, int bufferLength)
{
	if(lib == NULL || nameBuffer == NULL || bufferLength <= 0 || obituaryIndex < 0 || obituaryIndex >= (int)lib->demo->_obituaries.size())
	{
		return ErrorCode::InvalidArgument;
	}

	Q_strncpyz(nameBuffer, lib->demo->_obituaries[obituaryIndex].TargetName.c_str(), bufferLength);

	return ErrorCode::None;
}

UDT_API(int) udtGetObituaryMeanOfDeath(UdtHandle lib, int obituaryIndex, int* mod)
{
	if(lib == NULL || mod == NULL || obituaryIndex < 0 || obituaryIndex >= (int)lib->demo->_obituaries.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*mod = lib->demo->_obituaries[obituaryIndex].MeanOfDeath;

	return ErrorCode::None;
}

UDT_API(int) udtGetPuRunCount(UdtHandle lib, int* puRunCount)
{
	if(lib == NULL || puRunCount == NULL)
	{
		return ErrorCode::InvalidArgument;
	}

	*puRunCount = (int)lib->demo->_puRuns.size();

	return ErrorCode::None;
}

UDT_API(int) udtGetPuRunTime(UdtHandle lib, int puRunIndex, int* time)
{
	if(lib == NULL || time == NULL || puRunIndex < 0 || puRunIndex >= (int)lib->demo->_puRuns.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*time = lib->demo->_puRuns[puRunIndex].VirtualServerTime;

	return ErrorCode::None;
}

UDT_API(int) udtGetPuRunPlayerName(UdtHandle lib, int puRunIndex, char* nameBuffer, int bufferLength)
{
	if(lib == NULL || nameBuffer == NULL || bufferLength <= 0 || puRunIndex < 0 || puRunIndex >= (int)lib->demo->_puRuns.size())
	{
		return ErrorCode::InvalidArgument;
	}

	Q_strncpyz(nameBuffer, lib->demo->_puRuns[puRunIndex].PlayerName.c_str(), bufferLength);

	return ErrorCode::None;
}

UDT_API(int) udtGetPuRunPu(UdtHandle lib, int puRunIndex, int* pu)
{
	if(lib == NULL || pu == NULL || puRunIndex < 0 || puRunIndex >= (int)lib->demo->_puRuns.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*pu = lib->demo->_puRuns[puRunIndex].Pu;

	return ErrorCode::None;
}

UDT_API(int) udtGetPuRunDuration(UdtHandle lib, int puRunIndex, int* durationMs)
{
	if(lib == NULL || durationMs == NULL || puRunIndex < 0 || puRunIndex >= (int)lib->demo->_puRuns.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*durationMs = lib->demo->_puRuns[puRunIndex].Duration;

	return ErrorCode::None;
}

UDT_API(int) udtGetPuRunKillCount(UdtHandle lib, int puRunIndex, int* kills)
{
	if(lib == NULL || kills == NULL || puRunIndex < 0 || puRunIndex >= (int)lib->demo->_puRuns.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*kills = lib->demo->_puRuns[puRunIndex].Kills;

	return ErrorCode::None;
}

UDT_API(int) udtGetPuRunTeamKillCount(UdtHandle lib, int puRunIndex, int* teamKills)
{
	if(lib == NULL || teamKills == NULL || puRunIndex < 0 || puRunIndex >= (int)lib->demo->_puRuns.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*teamKills = lib->demo->_puRuns[puRunIndex].TeamKills;

	return ErrorCode::None;
}

UDT_API(int) udtGetPuRunSelfKill(UdtHandle lib, int puRunIndex, int* selfKill)
{
	if(lib == NULL || selfKill == NULL || puRunIndex < 0 || puRunIndex >= (int)lib->demo->_puRuns.size())
	{
		return ErrorCode::InvalidArgument;
	}

	*selfKill = lib->demo->_puRuns[puRunIndex].SelfKill;

	return ErrorCode::None;
}

UDT_API(int) udtCutDemoTime(const char* inFilePath, const char* outFilePath, int startTimeMs, int endTimeMs, ProgressCallback progressCb, MessageCallback messageCb)
{
	if(inFilePath == NULL || outFilePath == NULL || startTimeMs < 0 || endTimeMs < 0)
	{
		return ErrorCode::InvalidArgument;
	}

	CutDemoTime(inFilePath, outFilePath, startTimeMs, endTimeMs, progressCb, messageCb);

	return ErrorCode::None;
}

UDT_API(int) udtCutDemoChat(const char* inFilePath, const char* outFilePath, const char** chatEntries, int chatEntryCount, int startOffsetSecs, int endOffsetSecs)
{
	if(inFilePath == NULL || outFilePath == NULL || chatEntries == NULL || chatEntryCount <= 0 || startOffsetSecs < 0 || endOffsetSecs < 0)
	{
		return ErrorCode::InvalidArgument;
	}

	std::vector<std::string> chatVector;
	for(int i = 0; i < chatEntryCount; ++i)
	{
		chatVector.push_back(chatEntries[i]);
	}

	CutDemoChat(inFilePath, outFilePath, chatVector, startOffsetSecs, endOffsetSecs);

	return ErrorCode::None;
}