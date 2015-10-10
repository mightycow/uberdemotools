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
	const uptr smallByteCount = 1 << 14;
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
}

void udtParserPlugInCaptures::FinishDemoAnalysis()
{
}

void udtParserPlugInCaptures::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	++_gameStateIndex;

	memset(_players, 0, sizeof(_players));
	for(u32 i = 0; i < 64; ++i)
	{
		_players[i].BasePickup = false;
		_players[i].Capped = false;
		_players[i].PrevCapped = false;
		_players[i].HasFlag = false;
		_players[i].PrevHasFlag = false;
		_players[i].PrevCaptureCount = 0;
		_players[i].CaptureCount = 0;
		_players[i].PickupTimeMs = S32_MIN;
		Float3::Zero(_players[i].PickupPosition);
		Float3::Zero(_players[i].Position);
	}

	memset(_teams, 0, sizeof(_teams));
	for(u32 i = 0; i < 2; ++i)
	{
		_teams[i].FlagState = (u8)idFlagStatus::InBase;
		_teams[i].PrevFlagState = (u8)idFlagStatus::InBase;
	}

	_demoTakerIndex = arg.ClientNum;

	udtVMScopedStackAllocator allocScope(*TempAllocator);
	udtString mapName;
	if(ParseConfigStringValueString(mapName, *TempAllocator, "mapname", parser.GetConfigString(CS_SERVERINFO).String))
	{
		_mapName = udtString::NewCloneFromRef(_stringAllocator, mapName).String;
	}
	else
	{
		_mapName = NULL;
	}

	const s32 flagStatusIdx = idConfigStringIndex::FlagStatus(parser._inProtocol);
	if(flagStatusIdx >= 0)
	{
		const udtString cs = parser.GetConfigString(flagStatusIdx);
		if(cs.Length >= 2)
		{
			_teams[0].FlagState = (u8)(cs.String[0] - '0');
			_teams[1].FlagState = (u8)(cs.String[1] - '0');
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
			_teams[0].PrevFlagState = _teams[0].FlagState;
			_teams[1].PrevFlagState = _teams[1].FlagState;
			_teams[0].FlagState = (u8)(cs.String[0] - '0');
			_teams[1].FlagState = (u8)(cs.String[1] - '0');
		}
	}
}

void udtParserPlugInCaptures::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, parser._inProtocol);
	const s32 redFlagIdx = idPowerUpIndex::RedFlag(parser._inProtocol);
	const s32 blueFlagIdx = idPowerUpIndex::BlueFlag(parser._inProtocol);
	const s32 captureCountPersIdx = idPersStatsIndex::FlagCaptures(parser._inProtocol);

	if(ps->clientNum >= 0 && ps->clientNum < 64)
	{
		PlayerInfo& player = _players[ps->clientNum];
		const bool hasFlag = ps->powerups[redFlagIdx] == S32_MAX || ps->powerups[blueFlagIdx] == S32_MAX;
		const bool prevHasFlag = player.HasFlag;
		player.HasFlag = hasFlag;
		player.PrevHasFlag = prevHasFlag;
		if(!prevHasFlag && hasFlag)
		{
			player.PickupTimeMs = arg.ServerTime;
			Float3::Copy(player.PickupPosition, ps->origin);
			const u32 flagTeamIdx = ps->powerups[blueFlagIdx] == S32_MAX ? 1 : 0;
			player.BasePickup = WasFlagPickedUpInBase(flagTeamIdx);
		}
		player.PrevCaptureCount = player.CaptureCount;
		player.CaptureCount = ps->persistant[captureCountPersIdx];
		Float3::Copy(player.Position, ps->origin);
	}

	if(parser._inProtocol >= udtProtocol::Dm48) // @NOTE: EF_AWARD_CAP doesn't exist in dm3.
	{
		for(u32 i = 0, count = arg.EntityCount; i < count; ++i)
		{			
			idEntityStateBase* const es = arg.Entities[i].Entity;
			if(arg.Entities[i].IsNewEvent ||
			   es == NULL ||
			   es->eType != ET_PLAYER ||
			   es->clientNum < 0 ||
			   es->clientNum >= 64)
			{
				continue;
			}

			PlayerInfo& player = _players[es->clientNum];

			const bool capped = (es->eFlags & EF_AWARD_CAP) != 0;
			player.PrevCapped = player.Capped;
			player.Capped = capped;
			Float3::Copy(player.Position, es->pos.trBase);

			const bool hasRedFlag = (es->powerups & (1 << redFlagIdx)) != 0;
			const bool hasBlueFlag = (es->powerups & (1 << blueFlagIdx)) != 0;
			const bool hasFlag = hasRedFlag || hasBlueFlag;
			const bool prevHasFlag = player.HasFlag;
			player.HasFlag = hasFlag;
			player.PrevHasFlag = prevHasFlag;
			if(!prevHasFlag && hasFlag)
			{
				player.PickupTimeMs = arg.ServerTime;
				Float3::Copy(player.PickupPosition, es->pos.trBase);
				const u32 flagTeamIdx = hasBlueFlag ? 1 : 0;
				player.BasePickup = WasFlagPickedUpInBase(flagTeamIdx);
			}
		}
	}
	
	for(u32 i = 0; i < 64; ++i)
	{
		PlayerInfo& player = _players[i];
		const bool justCappedFirstPerson = player.CaptureCount > player.PrevCaptureCount;
		const bool justCappedThirdPerson = player.Capped && !player.PrevCapped;
		const bool justCapped = justCappedFirstPerson || justCappedThirdPerson;
		if(justCapped)
		{
			// @NOTE: It's possible to cap on the same snapshot that you pick up the flag.
			// So when the pick up time is undefined, it can mean one of 2 things:
			// 1. there was an instant cap, or
			// 2. the pick-up happened before demo recording began.
			// If the latter is true, we drop the cap because we don't know enough about it.
			const bool instantCap = !player.PrevHasFlag;
			if(player.PickupTimeMs == S32_MIN && !instantCap)
			{
				continue;
			}

			const s32 pickupTimeMs = player.PickupTimeMs == S32_MIN ? arg.ServerTime : player.PickupTimeMs;
			const s32 playerIndex = (s32)i;
			udtParseDataCapture cap;
			cap.GameStateIndex = _gameStateIndex;
			cap.PickUpTimeMs = pickupTimeMs;
			cap.CaptureTimeMs = arg.ServerTime;
			cap.Distance = Float3::Dist(player.PickupPosition, player.Position);
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
			if(player.BasePickup)
			{
				cap.Flags |= (u32)udtParseDataCaptureFlags::BaseToBase;
			}
			_captures.Add(cap);

			player.PickupTimeMs = S32_MIN;

			// We don't break now because it might be possible to have 2 caps on the same snapshot.
			// Not sure if the Q3 servers can actually handle that properly though.
		}
	}

	for(u32 i = 0; i < 64; ++i)
	{
		PlayerInfo& player = _players[i];
		if(!player.HasFlag)
		{
			player.PickupTimeMs = S32_MIN;
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

bool udtParserPlugInCaptures::WasFlagPickedUpInBase(u32 teamIndex)
{
	if(teamIndex > 1)
	{
		return false;
	}

	const TeamInfo& team = _teams[teamIndex];
	const u8 prevFlagStatus = team.PrevFlagState;
	const u8 currFlagStatus = team.FlagState;
	const u8 flagStatus = currFlagStatus == (u8)idFlagStatus::Captured ? prevFlagStatus : currFlagStatus;
	const bool inBase = flagStatus == (u8)idFlagStatus::InBase;

	return inBase;
}
