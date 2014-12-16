#include "analysis_awards.hpp"
#include "utils.hpp"


void udtAwardsAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.Snapshot->serverCommandNum == _lastProcessedServerCommandNumber)
	{
		return;
	}
	
	const idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, parser._protocol);
	for(s32 i = 0; i < MAX_PERSISTANT; ++i)
	{
		if(ps->persistant[i] != _persistant[i])
		{
			udtParseDataAward info;
			info.AwardId = i;
			info.GameStateIndex = _gameStateIndex;
			info.ServerTimeMs = arg.ServerTime;
			AwardEvents.Add(info);
		}
	}

	for(s32 i = 0; i < MAX_PERSISTANT; ++i)
	{
		_persistant[i] = ps->persistant[i];
	}

	_lastProcessedServerCommandNumber = arg.Snapshot->serverCommandNum;
}

void udtAwardsAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& /*parser*/)
{
	RecordingPlayerIndex = arg.ClientNum;
}

void udtAwardsAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
}
