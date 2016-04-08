#include "plug_in_captures.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


/*
Sound indices for EV_GLOBAL_TEAM_SOUND in CPMA:
0 blue captures (flag team: red)
1 red captures  (flag team: blue)
4 red taken     (flag team: red)
5 blue taken    (flag team: blue)
*/


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
	_processGamestate = &udtParserPlugInCaptures::ProcessGamestateMessageDummy;
	_processCommand = &udtParserPlugInCaptures::ProcessCommandMessageDummy;
	_processSnapshot = &udtParserPlugInCaptures::ProcessSnapshotMessageDummy;
}

void udtParserPlugInCaptures::FinishDemoAnalysis()
{
}

void udtParserPlugInCaptures::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	++_gameStateIndex;
	_firstSnapshot = true;
	_demoTakerIndex = arg.ClientNum;

	udtVMScopedStackAllocator allocScope(*TempAllocator);

	const udtProtocol::Id protocol = parser._inProtocol;
	if(protocol >= udtProtocol::Dm73 && protocol <= udtProtocol::Dm91)
	{
		_processGamestate = &udtParserPlugInCaptures::ProcessGamestateMessageStrings;
		_processCommand = &udtParserPlugInCaptures::ProcessCommandMessageStrings;
		_processSnapshot = &udtParserPlugInCaptures::ProcessSnapshotMessageStrings;
	}
	// @NOTE: EF_AWARD_CAP doesn't exist in dm3.
	else if(protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm68)
	{
		udtString gameName;
		if(ParseConfigStringValueString(gameName, *TempAllocator, "gamename", parser.GetConfigString(CS_SERVERINFO).GetPtr()) &&
		   udtString::Equals(gameName, "cpma"))
		{
			_processGamestate = &udtParserPlugInCaptures::ProcessGamestateMessageEntities;
			_processCommand = &udtParserPlugInCaptures::ProcessCommandMessageEntities;
			_processSnapshot = &udtParserPlugInCaptures::ProcessSnapshotMessageEntities;
		}
	}

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

void udtParserPlugInCaptures::ProcessGamestateMessageStrings(const udtGamestateCallbackArg&, udtBaseParser&)
{
}

void udtParserPlugInCaptures::ProcessCommandMessageStrings(const udtCommandCallbackArg& arg, udtBaseParser& parser)
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
	flags |= (u32)udtParseDataCaptureFlags::PlayerNameValid;

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

void udtParserPlugInCaptures::ProcessSnapshotMessageStrings(const udtSnapshotCallbackArg&, udtBaseParser&)
{
}

void udtParserPlugInCaptures::ProcessGamestateMessageEntities(const udtGamestateCallbackArg&, udtBaseParser& parser)
{
	memset(_players, 0, sizeof(_players));
	for(u32 i = 0; i < 64; ++i)
	{
		PlayerInfo& player = _players[i];
		player.Defined = false;
		player.PrevDefined = false;
		player.PickupPositionValid = false;
		Float3::Zero(player.PickupPosition);
		Float3::Zero(player.Position);
	}

	memset(_teams, 0, sizeof(_teams));
	for(u32 i = 0; i < 2; ++i)
	{
		TeamInfo& team = _teams[i];
		team.PlayerIndex = -1;
		team.FlagState = (u8)idFlagStatus::InBase;
		team.PrevFlagState = (u8)idFlagStatus::InBase;
		team.BasePickup = false;
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

void udtParserPlugInCaptures::ProcessCommandMessageEntities(const udtCommandCallbackArg& arg, udtBaseParser& parser)
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

void udtParserPlugInCaptures::ProcessSnapshotMessageEntities(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, parser._inProtocol);

	//
	// Step 1: update Defined, PrevDefined, Position
	//

	if(ps->clientNum >= 0 && ps->clientNum < 64)
	{
		PlayerInfo& player = _players[ps->clientNum];
		player.PrevDefined = player.Defined;
		player.Defined = true;
		Float3::Copy(player.Position, ps->origin);
	}

	for(u32 i = 0; i < 64; ++i)
	{
		PlayerInfo& player = _players[i];
		player.PrevDefined = player.Defined;
		player.Defined = false;
	}

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
		player.Defined = true;
		Float3::Copy(player.Position, es->pos.trBase);
	}

	//
	// Step 2: handle pick-up and capture events
	//

	for(u32 i = 0, count = arg.EntityCount; i < count; ++i)
	{
		idEntityStateBase* const es = arg.Entities[i].Entity;
		if(!arg.Entities[i].IsNewEvent || 
		   es == NULL ||
		   es->eType <= ET_EVENTS)
		{
			continue;
		}

		const s32 event = (es->eType - ET_EVENTS) & (~EV_EVENT_BITS);
		if(event == EV_GLOBAL_TEAM_SOUND)
		{
			const s32 soundIndex = es->eventParm;
			if(soundIndex == 0 || soundIndex == 1)
			{
				const s32 captureTimeMs = parser._inServerTime;
				const s32 durationMs = es->time;
				const s32 pickupTimeMs = captureTimeMs - durationMs;

				TeamInfo& team = _teams[soundIndex];
				const s32 playerIndex = team.PlayerIndex;

				udtParseDataCapture capture;
				capture.GameStateIndex = _gameStateIndex;
				capture.PickUpTimeMs = pickupTimeMs;
				capture.CaptureTimeMs = captureTimeMs;
				capture.PlayerIndex = playerIndex;
				WriteStringToApiStruct(capture.MapName, _mapName);
				capture.Flags = 0;
				if(playerIndex == _demoTakerIndex)
				{
					capture.Flags |= (u32)udtParseDataCaptureFlags::DemoTaker;
				}
				else if(playerIndex == ps->clientNum)
				{
					capture.Flags |= (u32)udtParseDataCaptureFlags::FirstPersonPlayer;
				}

				if(playerIndex >= 0 && playerIndex < 64)
				{
					capture.Flags |= (u32)udtParseDataCaptureFlags::PlayerIndexValid;
					capture.Flags |= (u32)udtParseDataCaptureFlags::PlayerNameValid;
					capture.Flags |= (u32)udtParseDataCaptureFlags::DistanceValid;
					PlayerInfo& player = _players[playerIndex];
					capture.Distance = Float3::Dist(player.PickupPosition, player.Position);
					WriteStringToApiStruct(capture.PlayerName, GetPlayerName(playerIndex, parser));
					if(team.BasePickup)
					{
						capture.Flags |= (u32)udtParseDataCaptureFlags::BaseToBase;
					}
					player.PickupPositionValid = false;
					team.BasePickup = false;
					team.PlayerIndex = -1;
				}
				else
				{
					WriteNullStringToApiStruct(capture.PlayerName);
					capture.Distance = -1.0f;
				}

				_captures.Add(capture);
			}
			else if(soundIndex == 4 || soundIndex == 5)
			{
				const s32 playerIndex = es->generic1;
				if(playerIndex >= 0 && playerIndex < 64)
				{
					PlayerInfo& player = _players[playerIndex];
					if(player.Defined && player.PrevDefined)
					{
						Float3::Copy(player.PickupPosition, player.Position);
						player.PickupPositionValid = true;
					}
					else
					{
						player.PickupPositionValid = false;
					}
					
					const u32 teamIndex = (u32)soundIndex - 4;
					TeamInfo& team = _teams[teamIndex];
					team.BasePickup = WasFlagPickedUpInBase(teamIndex);
					team.PlayerIndex = playerIndex;
				}
			}
		}
	}
}

void udtParserPlugInCaptures::ProcessGamestateMessageDummy(const udtGamestateCallbackArg&, udtBaseParser&)
{
}

void udtParserPlugInCaptures::ProcessCommandMessageDummy(const udtCommandCallbackArg&, udtBaseParser&)
{
}

void udtParserPlugInCaptures::ProcessSnapshotMessageDummy(const udtSnapshotCallbackArg&, udtBaseParser&)
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
