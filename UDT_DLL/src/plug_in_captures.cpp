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
	_stringAllocator.Init(ComputeReservedByteCount(smallByteCount, smallByteCount * 4, 16, demoCount), "udtParserPlugInCaptures::Strings");
	_captures.Init((uptr)demoCount * (uptr)(1 << 16), "udtParserPlugInCaptures::CapturesArray");
}

void udtParserPlugInCaptures::CopyBuffersStruct(void* buffersStruct) const
{
	*(udtParseDataCaptureBuffers*)buffersStruct = _buffers;
}

void udtParserPlugInCaptures::UpdateBufferStruct()
{
	_buffers.CaptureCount = _captures.GetSize();
	_buffers.CaptureRanges = BufferRanges.GetStartAddress();
	_buffers.Captures = _captures.GetStartAddress();
	_buffers.StringBuffer = _stringAllocator.GetStartAddress();
	_buffers.StringBufferSize = (u32)_stringAllocator.GetCurrentByteCount();
}

u32 udtParserPlugInCaptures::GetItemCount() const
{
	return _captures.GetSize();
}

void udtParserPlugInCaptures::StartDemoAnalysis()
{
	_mapName = udtString::NewNull();
	_gameStateIndex = -1;
	_demoTakerIndex = -1;
	_firstSnapshot = true;
}

void udtParserPlugInCaptures::FinishDemoAnalysis()
{
}

void udtParserPlugInCaptures::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	++_gameStateIndex;
	_firstSnapshot = true;

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
	if(ParseConfigStringValueString(mapName, *TempAllocator, "mapname", parser.GetConfigString(CS_SERVERINFO).GetPtr()))
	{
		_mapName = udtString::NewCloneFromRef(_stringAllocator, mapName);
	}
	else
	{
		_mapName = udtString::NewNull();
	}

	const s32 flagStatusIdx = idConfigStringIndex::FlagStatus(parser._inProtocol);
	if(flagStatusIdx >= 0)
	{
		const udtString cs = parser.GetConfigString(flagStatusIdx);
		if(cs.GetLength() >= 2)
		{
			const char* const csString = cs.GetPtr();
			_teams[0].FlagState = (u8)(csString[0] - '0');
			_teams[1].FlagState = (u8)(csString[1] - '0');
		}
	}
}

void udtParserPlugInCaptures::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.IsConfigString && arg.ConfigStringIndex == idConfigStringIndex::FlagStatus(parser._inProtocol))
	{
		const udtString cs = parser.GetTokenizer().GetArg(2);
		if(cs.GetLength() >= 2)
		{
			_teams[0].PrevFlagState = _teams[0].FlagState;
			_teams[1].PrevFlagState = _teams[1].FlagState;
			const char* const csString = cs.GetPtr();
			_teams[0].FlagState = (u8)(csString[0] - '0');
			_teams[1].FlagState = (u8)(csString[1] - '0');
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
		const bool hasFlag = ps->powerups[redFlagIdx] != 0 || ps->powerups[blueFlagIdx] != 0;
		const bool prevHasFlag = player.HasFlag;
		player.HasFlag = hasFlag;
		player.PrevHasFlag = prevHasFlag;
		if(!prevHasFlag && hasFlag)
		{
			player.PickupTimeMs = arg.ServerTime;
			Float3::Copy(player.PickupPosition, ps->origin);
			const u32 flagTeamIdx = ps->powerups[blueFlagIdx] != 0 ? 1 : 0;
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
			if(capped && _firstSnapshot)
			{
				player.PrevCapped = true;
			}

			const bool hasRedFlag = (es->powerups & (1 << redFlagIdx)) != 0;
			const bool hasBlueFlag = (es->powerups & (1 << blueFlagIdx)) != 0;
			const bool hasFlag = hasRedFlag || hasBlueFlag;
			bool prevHasFlag = player.HasFlag;
			if(hasFlag && _firstSnapshot)
			{
				prevHasFlag = true;
			}
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
		const bool justCappedFirstPerson = (player.CaptureCount > player.PrevCaptureCount) && !player.HasFlag && player.PrevHasFlag;
		const bool justCappedThirdPerson = player.Capped && !player.PrevCapped;
		const bool justCapped = justCappedFirstPerson || justCappedThirdPerson;
		if(justCapped)
		{
			// @NOTE: It's possible to cap on the same snapshot that you pick up the flag.
			// So when the pick up time is undefined, it can mean one of 2 things:
			// 1. there was an instant cap, or
			// 2. the pick-up happened before demo recording began.
			// If the latter is true, we drop the cap because we don't know enough about it.
			if(player.PickupTimeMs == S32_MIN && player.PrevHasFlag)
			{
				continue;
			}

			const s32 pickupTimeMs = player.PickupTimeMs == S32_MIN ? arg.ServerTime : player.PickupTimeMs;
			const s32 captureTimeMs = arg.ServerTime;
			const s32 playerIndex = (s32)i;
			udtParseDataCapture cap;
			cap.GameStateIndex = _gameStateIndex;
			cap.PickUpTimeMs = pickupTimeMs;
			cap.CaptureTimeMs = captureTimeMs;
			cap.Distance = Float3::Dist(player.PickupPosition, player.Position);
			cap.PlayerIndex = playerIndex;
			WriteStringToApiStruct(cap.PlayerName, GetPlayerName(playerIndex, parser));
			WriteStringToApiStruct(cap.MapName, _mapName);
			cap.Flags = 0;
			if(playerIndex == _demoTakerIndex)
			{
				cap.Flags |= (u32)udtParseDataCaptureFlags::DemoTaker;
			}
			else if(playerIndex == ps->clientNum)
			{
				cap.Flags |= (u32)udtParseDataCaptureFlags::FirstPersonPlayer;
			}
			if(player.BasePickup && player.PrevHasFlag && captureTimeMs > pickupTimeMs)
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

	_firstSnapshot = false;
}

udtString udtParserPlugInCaptures::GetPlayerName(s32 playerIndex, udtBaseParser& parser)
{
	if(playerIndex < 0 || playerIndex >= 64)
	{
		return udtString::NewNull();
	}

	udtVMScopedStackAllocator allocScope(*TempAllocator);

	const s32 csIndex = idConfigStringIndex::FirstPlayer(parser._inProtocol) + playerIndex;

	udtString playerName;
	if(!ParseConfigStringValueString(playerName, *TempAllocator, "n", parser.GetConfigString(csIndex).GetPtr()))
	{
		return udtString::NewNull();
	}

	return udtString::NewCleanCloneFromRef(_stringAllocator, parser._inProtocol, playerName);
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
