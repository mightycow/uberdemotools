#include "analysis_awards.hpp"
#include "utils.hpp"


void udtAwardsAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.Snapshot->serverCommandNum == _lastProcessedServerCommandNumber)
	{
		return;
	}

	//
	// @NOTE:
	// On the first snapshot after a new gamestate message, we copy over the persistant data but don't test it.
	// If we didn't do that and the player started recording after getting some medals, 
	// we would get a match on the first snapshot and that match would be invalid.
	//
	
	const idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, parser._protocol);
	if(!_firstSnapshot)
	{
		for(s32 i = 0; i < MAX_PERSISTANT; ++i)
		{
			// @NOTE: Might be reset to 0, so we test only if bigger.
			if(ps->persistant[i] > _persistant[i])
			{
				udtParseDataAward info;
				info.AwardId = i;
				info.GameStateIndex = _gameStateIndex;
				info.ServerTimeMs = arg.ServerTime;
				AwardEvents.Add(info);
			}
		}
	}

	for(s32 i = 0; i < MAX_PERSISTANT; ++i)
	{
		_persistant[i] = ps->persistant[i];
	}

	_lastProcessedServerCommandNumber = arg.Snapshot->serverCommandNum;
	_firstSnapshot = false;
}

void udtAwardsAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& /*parser*/)
{
	RecordingPlayerIndex = arg.ClientNum;
	++_gameStateIndex;
	_firstSnapshot = true;
}

void udtAwardsAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
}
