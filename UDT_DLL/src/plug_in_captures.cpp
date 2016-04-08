#include "plug_in_captures.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


static bool ParseDuration(s32& durationMs, const udtString& duration)
{
	u32 colonIndex = 0;
	if(!udtString::FindFirstCharacterMatch(colonIndex, duration, ':'))
	{
		return false;
	}

	u32 dotIndex = 0;
	const bool hasDot = udtString::FindFirstCharacterMatch(dotIndex, duration, '.', colonIndex + 2);

	int minutes = 0;
	if(sscanf(duration.GetPtr(), "%d", &minutes) != 1)
	{
		return false;
	}

	int seconds = 0;
	if(sscanf(duration.GetPtr() + colonIndex + 1, "%d", &seconds) != 1)
	{
		return false;
	}
	
	s32 result = s32((minutes * 60 + seconds) * 1000);
	if(hasDot)
	{
		int subSeconds = 0;
		if(sscanf(duration.GetPtr() + dotIndex + 1, "%d", &subSeconds) != 1)
		{
			return false;
		}

		static const u32 multipliers[4] = { 1, 100, 10, 1 };
		const u32 digitCount = duration.GetLength() - dotIndex;
		if(digitCount == 0 || digitCount > 3)
		{
			return false;
		}

		result += (s32)subSeconds * multipliers[digitCount];
	}
	
	durationMs = result;

	return true;
}


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
	_processGamestate = &udtParserPlugInCaptures::ProcessGamestateMessageNada;
	_processCommand = &udtParserPlugInCaptures::ProcessCommandMessageNada;
	_processSnapshot = &udtParserPlugInCaptures::ProcessSnapshotMessageNada;
}

void udtParserPlugInCaptures::FinishDemoAnalysis()
{
}

void udtParserPlugInCaptures::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	++_gameStateIndex;
	_firstSnapshot = true;
	_demoTakerIndex = arg.ClientNum;

	const udtProtocol::Id protocol = parser._inProtocol;
	if(protocol >= udtProtocol::Dm73 && protocol <= udtProtocol::Dm91)
	{
		_processGamestate = &udtParserPlugInCaptures::ProcessGamestateMessageString;
		_processCommand = &udtParserPlugInCaptures::ProcessCommandMessageString;
		_processSnapshot = &udtParserPlugInCaptures::ProcessSnapshotMessageString;
	}
	// @NOTE: EF_AWARD_CAP doesn't exist in dm3.
	else if(protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm68)
	{
		_processGamestate = &udtParserPlugInCaptures::ProcessGamestateMessageEvents;
		_processCommand = &udtParserPlugInCaptures::ProcessCommandMessageEvents;
		_processSnapshot = &udtParserPlugInCaptures::ProcessSnapshotMessageEvents;
	}

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

	(this->*_processGamestate)(arg, parser);
}

void udtParserPlugInCaptures::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	(this->*_processCommand)(arg, parser);
}

void udtParserPlugInCaptures::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	(this->*_processSnapshot)(arg, parser);

	_firstSnapshot = false;
}

void udtParserPlugInCaptures::ProcessGamestateMessageString(const udtGamestateCallbackArg&, udtBaseParser&)
{
}

void udtParserPlugInCaptures::ProcessCommandMessageString(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.IsConfigString)
	{
		return;
	}

	idTokenizer& tokenizer = parser._context->Tokenizer;
	tokenizer.Tokenize(arg.String);
	if(tokenizer.GetArgCount() != 2 || !udtString::EqualsNoCase(tokenizer.GetArg(0), "print"))
	{
		return;
	}

	const udtString message = tokenizer.GetArg(1);
	if(!udtString::ContainsNoCase(message, "CAPTURED the flag!"))
	{
		return;
	}

	const udtString capturedIn = udtString::NewConstRef("captured in");
	const udtString heldFor = udtString::NewConstRef("held for");
	u32 capturedInIdx = 0;
	u32 heldForIdx = 0;
	const bool capturedInFound = udtString::ContainsNoCase(capturedInIdx, message, "captured in");
	const bool heldForFound = udtString::ContainsNoCase(heldForIdx, message, "held for");
	if(!capturedInFound && !heldForFound)
	{
		return;
	}

	u32 leftParenIdx = 0;
	if(!udtString::FindFirstCharacterMatch(leftParenIdx, message, '('))
	{
		return;
	}

	u32 rightParenIdx = 0;
	if(!udtString::FindFirstCharacterMatch(rightParenIdx, message, ')', leftParenIdx + 1))
	{
		return;
	}

	const u32 parenTextIdx = capturedInFound ? capturedInIdx : heldForIdx;
	if(parenTextIdx < leftParenIdx || parenTextIdx > rightParenIdx)
	{
		return;
	}

	const udtString parenText = capturedInFound ? capturedIn : heldFor;
	udtString playerName = udtString::NewSubstringClone(_stringAllocator, message, leftParenIdx + 1, parenTextIdx - leftParenIdx - 2);
	udtString::CleanUp(playerName, parser._inProtocol);

	const u32 durationIdx = parenTextIdx + parenText.GetLength() + 1;
	if(durationIdx > rightParenIdx)
	{
		return;
	}

	udtVMScopedStackAllocator tempAllocatorScope(*TempAllocator);
	const udtString duration = udtString::NewSubstringClone(*TempAllocator, message, durationIdx, rightParenIdx - durationIdx - 1);
	s32 captureDuration = 0;
	if(!ParseDuration(captureDuration, duration))
	{
		return;
	}

	const s32 time = parser._inServerTime;
	u32 flags = 0;
	if(capturedInFound)
	{
		flags |= (u32)udtParseDataCaptureFlags::BaseToBase;
	}

	udtParseDataCapture capture;
	capture.CaptureTimeMs = time;
	capture.Distance = 0.0f;
	capture.Flags = flags;
	capture.GameStateIndex = _gameStateIndex;
	WriteStringToApiStruct(capture.MapName, _mapName);
	capture.PickUpTimeMs = time - captureDuration;
	capture.PlayerIndex = -1;
	WriteStringToApiStruct(capture.PlayerName, playerName);
	_captures.Add(capture);
}

void udtParserPlugInCaptures::ProcessSnapshotMessageString(const udtSnapshotCallbackArg&, udtBaseParser&)
{
}

void udtParserPlugInCaptures::ProcessGamestateMessageEvents(const udtGamestateCallbackArg&, udtBaseParser& parser)
{
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

void udtParserPlugInCaptures::ProcessCommandMessageEvents(const udtCommandCallbackArg& arg, udtBaseParser& parser)
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

void udtParserPlugInCaptures::ProcessSnapshotMessageEvents(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
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
}

void udtParserPlugInCaptures::ProcessGamestateMessageNada(const udtGamestateCallbackArg&, udtBaseParser&)
{
}

void udtParserPlugInCaptures::ProcessCommandMessageNada(const udtCommandCallbackArg&, udtBaseParser&)
{
}

void udtParserPlugInCaptures::ProcessSnapshotMessageNada(const udtSnapshotCallbackArg&, udtBaseParser&)
{
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
