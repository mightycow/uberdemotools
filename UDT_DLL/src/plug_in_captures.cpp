#include "plug_in_captures.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


udtParserPlugInCaptures::udtParserPlugInCaptures()
{
}

udtParserPlugInCaptures::~udtParserPlugInCaptures()
{
}

void udtParserPlugInCaptures::InitAllocators(u32 demoCount)
{
	const uptr smallByteCount = 1024;
	FinalAllocator.Init((uptr)demoCount * (uptr)(1 << 16), "udtParserPlugInCaptures::CapturesArray");
	_stringAllocator.Init(ComputeReservedByteCount(smallByteCount, smallByteCount * 4, 16, demoCount), "udtParserPlugInCaptures::Strings");
	_captures.SetAllocator(FinalAllocator);
}

u32 udtParserPlugInCaptures::GetElementSize() const
{
	return (u32)sizeof(udtParseDataCapture);
}

void udtParserPlugInCaptures::StartDemoAnalysis()
{
	_mapName = NULL;
	_gameStateIndex = -1;
	_demoTakerIndex = -1;
	for(u32 i = 0; i < 2; ++i)
	{
		_pickupTimeMs[i] = S32_MIN;
		_previousCaptureCount[i] = 0;
		for(u32 j = 0; j < 64; ++j)
		{
			_previousCapped[i][j] = false;
		}
	}
}

void udtParserPlugInCaptures::FinishDemoAnalysis()
{
}

void udtParserPlugInCaptures::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	++_gameStateIndex;
	for(u32 i = 0; i < 2; ++i)
	{
		_pickupTimeMs[i] = S32_MIN;
		_previousCaptureCount[i] = 0;
		for(u32 j = 0; j < 64; ++j)
		{
			_previousCapped[i][j] = false;
		}
		_prevFlagState[i] = (u8)idFlagStatus::InBase;
		_flagState[i] = (u8)idFlagStatus::InBase;
	}

	_demoTakerIndex = arg.ClientNum;

	udtVMScopedStackAllocator allocScope(*TempAllocator);
	udtString mapName;
	if(ParseConfigStringValueString(mapName, *TempAllocator, "mapname", parser.GetConfigString(CS_SERVERINFO).String))
	{
		_mapName = udtString::NewCloneFromRef(_stringAllocator, mapName).String;
	}

	const s32 flagStatusIdx = idConfigStringIndex::FlagStatus(parser._inProtocol);
	if(flagStatusIdx >= 0)
	{
		const udtString cs = parser.GetConfigString(flagStatusIdx);
		if(cs.Length >= 2)
		{
			_flagState[0] = (u8)(cs.String[0] - '0');
			_flagState[1] = (u8)(cs.String[1] - '0');
		}
	}
}

void udtParserPlugInCaptures::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.IsConfigString && arg.ConfigStringIndex == idConfigStringIndex::FlagStatus(parser._inProtocol))
	{
		const udtString cs = parser.GetTokenizer().GetArg(2);
		if(cs.Length >= 2)
		{
			_prevFlagState[0] = _flagState[0];
			_prevFlagState[1] = _flagState[1];
			_flagState[0] = (u8)(cs.String[0] - '0');
			_flagState[1] = (u8)(cs.String[1] - '0');
		}
	}
}

void udtParserPlugInCaptures::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, parser._inProtocol);
	idEntityStateBase* es = NULL;

	const s32 redFlagIdx = idPowerUpIndex::RedFlag(parser._inProtocol);
	const s32 blueFlagIdx = idPowerUpIndex::BlueFlag(parser._inProtocol);
	const s32 captureCountPersIdx = idPersStatsIndex::FlagCaptures(parser._inProtocol);

	for(u32 i = 0; i < 2; ++i)
	{
		const s32 flagIdx = i == 0 ? redFlagIdx : blueFlagIdx;
		s32 captureCount = _previousCaptureCount[i];
		s32 playerIndex = ps->clientNum;
		bool justCapped = false;
		bool flagTaken = false;

		if(ps->powerups[flagIdx] == S32_MAX)
		{
			flagTaken = true;
		}
		captureCount = ps->persistant[captureCountPersIdx];
		justCapped = captureCount > _previousCaptureCount[i];
		_previousCaptureCount[i] = captureCount;

		if(!flagTaken && !justCapped && parser._inProtocol >= udtProtocol::Dm48) // @NOTE: EF_AWARD_CAP doesn't exist in dm3.
		{
			for(u32 j = 0, count = arg.EntityCount; j < count; ++j)
			{
				es = arg.Entities[j].Entity;
				if(es == NULL || es->eType != ET_PLAYER)
				{
					continue;
				}

				const u32 team = (u32)GetPlayerTeam01(es->clientNum, parser);
				if(team == i)
				{
					continue;
				}

				const bool capped = (es->eFlags & EF_AWARD_CAP) != 0;
				const bool cappedThisSnap = !_previousCapped[i][es->clientNum] && capped;
				_previousCapped[i][es->clientNum] = capped;

				//
				// @NOTE:
				// On the WTF maps, there can be 2 flags per side.
				// That means that for each team, there can be a flag being held while another just got captured.
				// Since this is pretty much only played in CTFS, we only really care about the flag that just got captured.
				//

				if(cappedThisSnap)
				{
					justCapped = true;
					playerIndex = es->clientNum;
				}

				if(!justCapped && (es->powerups & (1 << flagIdx)) != 0)
				{
					flagTaken = true;
					playerIndex = es->clientNum;
				}
			}
		}

		if(justCapped)
		{
			flagTaken = false;
		}

		if(_pickupTimeMs[i] == S32_MIN && flagTaken)
		{
			_pickupTimeMs[i] = arg.ServerTime;
			Float3::Copy(_pickUpPosition[i], es != NULL ? es->pos.trBase : ps->origin);
		}

		if(_pickupTimeMs[i] != S32_MIN && justCapped)
		{
			udtParseDataCapture cap;
			cap.GameStateIndex = _gameStateIndex;
			cap.PickUpTimeMs = _pickupTimeMs[i];
			cap.CaptureTimeMs = arg.ServerTime;
			cap.Distance = Float3::Dist(_pickUpPosition[i], es != NULL ? es->pos.trBase : ps->origin);
			cap.PlayerIndex = playerIndex;
			cap.PlayerName = GetPlayerName(playerIndex, parser);
			cap.MapName = _mapName;
			cap.Flags = 0;
			if(playerIndex == _demoTakerIndex)
			{
				cap.Flags |= (u32)udtParseDataCaptureFlags::DemoTaker;
			}
			else if(playerIndex == ps->clientNum)
			{
				cap.Flags |= (u32)udtParseDataCaptureFlags::FirstPersonPlayer;
			}
			if(_flagState[i] == (u8)idFlagStatus::InBase)
			{
				cap.Flags |= (u32)udtParseDataCaptureFlags::BaseToBase;
			}
			_captures.Add(cap);

			_pickupTimeMs[i] = S32_MIN;
		}

		if(!flagTaken)
		{
			_pickupTimeMs[i] = S32_MIN;
		}
	}
}

const char* udtParserPlugInCaptures::GetPlayerName(s32 playerIndex, udtBaseParser& parser)
{
	if(playerIndex < 0 || playerIndex >= 64)
	{
		return NULL;
	}

	udtVMScopedStackAllocator allocScope(*TempAllocator);

	const s32 csIndex = idConfigStringIndex::FirstPlayer(parser._inProtocol) + playerIndex;

	udtString playerName;
	if(!ParseConfigStringValueString(playerName, *TempAllocator, "n", parser.GetConfigString(csIndex).String))
	{
		return NULL;
	}

	return udtString::NewCleanCloneFromRef(_stringAllocator, parser._inProtocol, playerName).String;
}

s32 udtParserPlugInCaptures::GetPlayerTeam01(s32 playerIndex, udtBaseParser& parser)
{
	if(playerIndex < 0 || playerIndex >= 64)
	{
		return 0;
	}

	udtVMScopedStackAllocator allocScope(*TempAllocator);

	const s32 csIndex = idConfigStringIndex::FirstPlayer(parser._inProtocol) + playerIndex;

	s32 team = 0;
	if(!ParseConfigStringValueInt(team, *TempAllocator, "t", parser.GetConfigString(csIndex).String))
	{
		return 0;
	}

	return team == TEAM_BLUE ? 1 : 0;
}
