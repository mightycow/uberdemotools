#include "analysis_captures.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"

#include <stdlib.h>


/*
Sound indices for EV_GLOBAL_TEAM_SOUND in CPMA:
0 blue captures (flag team: red)
1 red captures  (flag team: blue)
4 red taken     (flag team: red)
5 blue taken    (flag team: blue)
*/


#define    MAX_ALLOWED_TIME_DELTA_QL_MS    1500


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
		const u32 digitCount = duration.GetLength() - dotIndex - 1;
		if(digitCount == 0 || digitCount > 3)
		{
			return false;
		}

		result += (s32)subSeconds * multipliers[digitCount];
	}

	durationMs = result;

	return true;
}


udtCapturesAnalyzer::udtCapturesAnalyzer()
{
	_tempAllocator = NULL;
}

udtCapturesAnalyzer::~udtCapturesAnalyzer()
{
}

void udtCapturesAnalyzer::Init(u32, udtVMLinearAllocator* tempAllocator)
{
	_tempAllocator = tempAllocator;
}

void udtCapturesAnalyzer::StartDemoAnalysis()
{
	_mapName = udtString::NewNull();
	_gameStateIndex = -1;
	_demoTakerIndex = -1;
	_firstSnapshot = true;
	_processGamestate = &udtCapturesAnalyzer::ProcessGamestateMessageDummy;
	_processCommand = &udtCapturesAnalyzer::ProcessCommandMessageDummy;
	_processSnapshot = &udtCapturesAnalyzer::ProcessSnapshotMessageDummy;

	_playerNameAllocator.Clear();
	for(u32 i = 0; i < 64; ++i)
	{
		_playerNames[i] = udtString::NewNull();
		_playerClanNames[i] = udtString::NewNull();
	}
}

void udtCapturesAnalyzer::FinishDemoAnalysis()
{
}

void udtCapturesAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	++_gameStateIndex;
	_firstSnapshot = true;
	_demoTakerIndex = arg.ClientNum;

	udtVMScopedStackAllocator allocScope(*_tempAllocator);

	const udtProtocol::Id protocol = parser._inProtocol;
	if(protocol >= udtProtocol::Dm73 && protocol <= udtProtocol::Dm91)
	{
		_processGamestate = &udtCapturesAnalyzer::ProcessGamestateMessageQLorOSP;
		_processCommand = &udtCapturesAnalyzer::ProcessCommandMessageQLorOSP;
		_processSnapshot = &udtCapturesAnalyzer::ProcessSnapshotMessageQLorOSP;
	}
	// @NOTE: EF_AWARD_CAP doesn't exist in dm3.
	else if(protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm68)
	{
		udtString gameName;
		if(ParseConfigStringValueString(gameName, *_tempAllocator, "gamename", parser.GetConfigString(CS_SERVERINFO).GetPtr()))
		{
			if(udtString::Equals(gameName, "cpma"))
			{
				_processGamestate = &udtCapturesAnalyzer::ProcessGamestateMessageCPMA;
				_processCommand = &udtCapturesAnalyzer::ProcessCommandMessageCPMA;
				_processSnapshot = &udtCapturesAnalyzer::ProcessSnapshotMessageCPMA;
			}
			else if(udtString::Equals(gameName, "osp"))
			{
				_processGamestate = &udtCapturesAnalyzer::ProcessGamestateMessageQLorOSP;
				_processCommand = &udtCapturesAnalyzer::ProcessCommandMessageQLorOSP;
				_processSnapshot = &udtCapturesAnalyzer::ProcessSnapshotMessageQLorOSP;
			}
		}
	}

	udtString mapName;
	if(ParseConfigStringValueString(mapName, *_tempAllocator, "mapname", parser.GetConfigString(CS_SERVERINFO).GetPtr()))
	{
		_mapName = udtString::NewCloneFromRef(StringAllocator, mapName);
	}
	else
	{
		_mapName = udtString::NewNull();
	}

	(this->*_processGamestate)(arg, parser);
}

void udtCapturesAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	(this->*_processCommand)(arg, parser);
}

void udtCapturesAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	(this->*_processSnapshot)(arg, parser);

	_firstSnapshot = false;
}

void udtCapturesAnalyzer::ProcessGamestateMessageQLorOSP(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	ProcessGamestateMessageClearStates(arg, parser);
	_lastCaptureQL.Clear();
	_playerStateQL.Clear();

	const s32 firstPlayerCsIdx = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol);
	for(s32 i = 0; i < 64; ++i)
	{
		const udtString cs = parser.GetConfigString(firstPlayerCsIdx + i);
		if(!udtString::IsNullOrEmpty(cs))
		{
			ProcessPlayerConfigStringQLorOSP(cs.GetPtr(), parser, i);
		}
	}
}

void udtCapturesAnalyzer::ProcessCommandMessageQLorOSP(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.IsConfigString)
	{
		if(arg.ConfigStringIndex == GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FlagStatus, parser._inProtocol))
		{
			ProcessFlagStatusCommandQLorOSP(arg, parser);
			return;
		}

		const s32 firstPlayerCsIdx = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol);
		if(arg.ConfigStringIndex >= firstPlayerCsIdx &&
		   arg.ConfigStringIndex < firstPlayerCsIdx + 64)
		{
			const s32 playerIndex = arg.ConfigStringIndex - firstPlayerCsIdx;
			ProcessPlayerConfigStringQLorOSP(parser.GetTokenizer().GetArgString(2), parser, playerIndex);
			return;
		}
	}
	else
	{
		idTokenizer& tokenizer = parser._context->Tokenizer;
		tokenizer.Tokenize(arg.String);
		if(tokenizer.GetArgCount() == 2 &&
		   udtString::EqualsNoCase(tokenizer.GetArg(0), "print"))
		{
			ProcessPrintCommandQLorOSP(arg, parser);
			return;
		}
	}
}

void udtCapturesAnalyzer::ProcessSnapshotMessageQLorOSP(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
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
		if((s32)i == ps->clientNum)
		{
			continue;
		}

		PlayerInfo& player = _players[i];
		player.PrevDefined = player.Defined;
		player.Defined = false;
	}

	const s32 entityTypePlayerId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Player, parser._inProtocol);
	for(u32 i = 0, count = arg.ChangedEntityCount; i < count; ++i)
	{
		idEntityStateBase* const es = arg.ChangedEntities[i].Entity;
		if(arg.ChangedEntities[i].IsNewEvent ||
		   es == NULL ||
		   es->eType != entityTypePlayerId ||
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

	if(ps->clientNum >= 0 && ps->clientNum < 64)
	{
		const s32 redFlagIdx = GetIdNumber(udtMagicNumberType::PowerUpIndex, udtPowerUpIndex::RedFlag, parser._inProtocol);
		const s32 blueFlagIdx = GetIdNumber(udtMagicNumberType::PowerUpIndex, udtPowerUpIndex::BlueFlag, parser._inProtocol);
		const s32 captureCountPersIdx = GetIdNumber(udtMagicNumberType::PersStatsIndex, udtPersStatsIndex::FlagCaptures, parser._inProtocol);

		PlayerInfo& player = _players[ps->clientNum];
		const bool hasFlag = ps->powerups[redFlagIdx] != 0 || ps->powerups[blueFlagIdx] != 0;
		const bool prevHasFlag = _firstSnapshot ? hasFlag : _playerStateQL.HasFlag;
		_playerStateQL.HasFlag = hasFlag;
		_playerStateQL.PrevHasFlag = prevHasFlag;
		const s32 captureCount = ps->persistant[captureCountPersIdx];
		const s32 prevCaptureCount = _firstSnapshot ? captureCount : _playerStateQL.CaptureCount;
		_playerStateQL.CaptureCount = captureCount;
		_playerStateQL.PrevCaptureCount = prevCaptureCount;
		if(!prevHasFlag && hasFlag)
		{
			Float3::Copy(player.PickupPosition, ps->origin);
		}
		else if(captureCount > prevCaptureCount)
		{
			const u32 storedCaptureCount = Captures.GetSize();
			if(!hasFlag && prevHasFlag && storedCaptureCount > 0)
			{
				udtParseDataCapture& capture = Captures[storedCaptureCount - 1];
				if(abs((int)(capture.CaptureTimeMs - parser._inServerTime)) < (int)MAX_ALLOWED_TIME_DELTA_QL_MS)
				{
					const f32 distance = Float3::Dist(player.PickupPosition, player.Position);
					capture.Distance = distance;
					capture.Flags |= (u32)udtParseDataCaptureMask::DistanceValid;
					capture.Flags |= (u32)udtParseDataCaptureMask::FirstPersonPlayer;
					if(ps->clientNum == _demoTakerIndex)
					{
						capture.Flags |= (u32)udtParseDataCaptureMask::DemoTaker;
					}
				}
			}
			_lastCaptureQL.Clear();
		}
	}

	const s32 entityTypeEventId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Event, parser._inProtocol);
	const s32 globalTeamSoundId = GetIdNumber(udtMagicNumberType::EntityEvent, udtEntityEvent::GlobalTeamSound, parser._inProtocol);
	for(u32 i = 0, count = arg.ChangedEntityCount; i < count; ++i)
	{
		idEntityStateBase* const es = arg.ChangedEntities[i].Entity;
		if(!arg.ChangedEntities[i].IsNewEvent ||
		   es == NULL ||
		   es->eType <= entityTypeEventId)
		{
			continue;
		}

		const s32 event = (es->eType - entityTypeEventId) & (~ID_ES_EVENT_BITS);
		if(event == globalTeamSoundId)
		{
			const s32 soundIndex = es->eventParm;
			if(soundIndex == 0 || soundIndex == 1)
			{
				TeamInfo& team = _teams[soundIndex];
				const s32 playerIndex = team.PlayerIndex;
				if(playerIndex < 0 || playerIndex >= 64)
				{
					continue;
				}

				PlayerInfo& player = _players[playerIndex];
				if(player.PickupPositionValid &&
				   (player.Defined || player.PrevDefined))
				{
					const f32 distance = Float3::Dist(player.PickupPosition, player.Position);
					const u32 captureCount = Captures.GetSize();
					if(captureCount > 0)
					{
						udtParseDataCapture& cap = Captures[captureCount - 1];
						if(abs((int)(cap.CaptureTimeMs - parser._inServerTime)) < (int)MAX_ALLOWED_TIME_DELTA_QL_MS)
						{
							_lastCaptureQL.Time = parser._inServerTime;
							_lastCaptureQL.Distance = distance;
						}
						else
						{
							_lastCaptureQL.Clear();
						}
					}
					else
					{
						_lastCaptureQL.Clear();
					}
				}
			}
			else if(soundIndex == 4 || soundIndex == 5)
			{
				const s32 playerIndex = es->otherEntityNum;
				if(playerIndex < 0 || playerIndex >= 64)
				{
					continue;
				}

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
				team.PlayerIndex = playerIndex;
			}
		}
	}
}

void udtCapturesAnalyzer::ProcessGamestateMessageCPMA(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	ProcessGamestateMessageClearStates(arg, parser);
	_flagStatusCPMA[0].Clear();
	_flagStatusCPMA[1].Clear();
}

void udtCapturesAnalyzer::ProcessCommandMessageCPMA(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.IsConfigString && arg.ConfigStringIndex == GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FlagStatus, parser._inProtocol))
	{
		const udtString cs = parser.GetTokenizer().GetArg(2);
		if(cs.GetLength() >= 2)
		{
			_teams[0].PrevFlagState = _teams[0].FlagState;
			_teams[1].PrevFlagState = _teams[1].FlagState;
			const char* const csString = cs.GetPtr();
			_teams[0].FlagState = (u8)(csString[0] - '0');
			_teams[1].FlagState = (u8)(csString[1] - '0');

			const s32 time = parser._inServerTime;
			for(u32 i = 0; i < 2; ++i)
			{
				FlagStatusCPMA& flagStatus = _flagStatusCPMA[i];
				const TeamInfo& team = _teams[i];
				const s32 prevTime = flagStatus.ChangeTime;
				const bool returned = team.PrevFlagState == (u8)idFlagStatus::Carried && team.FlagState == (u8)idFlagStatus::InBase;
				if(time == prevTime && returned)
				{
					flagStatus.InstantCapture = true;
				}
				if(team.FlagState != team.PrevFlagState)
				{
					flagStatus.ChangeTime = time;
				}
			}
		}
	}
}

void udtCapturesAnalyzer::ProcessSnapshotMessageCPMA(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
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
		if((s32)i == ps->clientNum)
		{
			continue;
		}

		PlayerInfo& player = _players[i];
		player.PrevDefined = player.Defined;
		player.Defined = false;
	}

	const s32 entityTypePlayerId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Player, parser._inProtocol);
	for(u32 i = 0, count = arg.ChangedEntityCount; i < count; ++i)
	{
		idEntityStateBase* const es = arg.ChangedEntities[i].Entity;
		if(arg.ChangedEntities[i].IsNewEvent ||
		   es == NULL ||
		   es->eType != entityTypePlayerId ||
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

	const s32 entityTypeEventId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Event, parser._inProtocol);
	const s32 globalTeamSoundId = GetIdNumber(udtMagicNumberType::EntityEvent, udtEntityEvent::GlobalTeamSound, parser._inProtocol);
	for(u32 i = 0, count = arg.ChangedEntityCount; i < count; ++i)
	{
		idEntityStateBase* const es = arg.ChangedEntities[i].Entity;
		if(!arg.ChangedEntities[i].IsNewEvent ||
		   es == NULL ||
		   es->eType <= entityTypeEventId)
		{
			continue;
		}

		const s32 event = (es->eType - entityTypeEventId) & (~ID_ES_EVENT_BITS);
		if(event == globalTeamSoundId)
		{
			const s32 soundIndex = es->eventParm;
			if(soundIndex == 0 || soundIndex == 1)
			{
				const s32 playerIndex = es->generic1;
				if(playerIndex < 0 || playerIndex >= 64)
				{
					continue;
				}

				const s32 captureTimeMs = parser._inServerTime;
				const s32 durationMs = es->time;
				const s32 pickupTimeMs = captureTimeMs - durationMs;

				TeamInfo& team = _teams[soundIndex];
				const s32 udtPlayerIndex = team.PlayerIndex;

				udtParseDataCapture capture;
				capture.GameStateIndex = _gameStateIndex;
				capture.PickUpTimeMs = pickupTimeMs;
				capture.CaptureTimeMs = captureTimeMs;
				capture.PlayerIndex = playerIndex;
				WriteStringToApiStruct(capture.MapName, _mapName);
				capture.Flags = 0;
				capture.Flags |= (u32)udtParseDataCaptureMask::PlayerIndexValid;
				capture.Flags |= (u32)udtParseDataCaptureMask::PlayerNameValid;
				if(playerIndex == _demoTakerIndex)
				{
					capture.Flags |= (u32)udtParseDataCaptureMask::DemoTaker;
				}
				else if(playerIndex == ps->clientNum)
				{
					capture.Flags |= (u32)udtParseDataCaptureMask::FirstPersonPlayer;
				}
				WriteStringToApiStruct(capture.PlayerName, GetPlayerName(playerIndex, parser));

				FlagStatusCPMA& flagStatus = _flagStatusCPMA[soundIndex];
				if(udtPlayerIndex == playerIndex)
				{
					capture.Flags |= (u32)udtParseDataCaptureMask::DistanceValid;
					PlayerInfo& player = _players[playerIndex];
					capture.Distance = Float3::Dist(player.PickupPosition, player.Position);
					if(team.BasePickup && !flagStatus.InstantCapture)
					{
						capture.Flags |= (u32)udtParseDataCaptureMask::BaseToBase;
					}
					player.PickupPositionValid = false;
					team.BasePickup = false;
					team.PlayerIndex = -1;
				}
				else
				{
					// The last player for which we got a pick-up sound event
					// is not the one the server says.
					capture.Distance = -1.0f;
				}

				Captures.Add(capture);
				flagStatus.InstantCapture = false;
			}
			else if(soundIndex == 4 || soundIndex == 5)
			{
				const s32 playerIndex = es->generic1;
				if(playerIndex < 0 || playerIndex >= 64)
				{
					continue;
				}

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

void udtCapturesAnalyzer::ProcessGamestateMessageDummy(const udtGamestateCallbackArg&, udtBaseParser&)
{
}

void udtCapturesAnalyzer::ProcessCommandMessageDummy(const udtCommandCallbackArg&, udtBaseParser&)
{
}

void udtCapturesAnalyzer::ProcessSnapshotMessageDummy(const udtSnapshotCallbackArg&, udtBaseParser&)
{
}

void udtCapturesAnalyzer::ProcessGamestateMessageClearStates(const udtGamestateCallbackArg&, udtBaseParser& parser)
{
	memset(_players, 0, sizeof(_players));
	for(u32 i = 0; i < 64; ++i)
	{
		PlayerInfo& player = _players[i];
		player.PickupTime = UDT_S32_MIN;
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

	const s32 flagStatusIdx = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FlagStatus, parser._inProtocol);
	if(flagStatusIdx >= 0)
	{
		const udtString cs = parser.GetConfigString(flagStatusIdx);
		if(cs.GetLength() >= 2)
		{
			const char* const csString = cs.GetPtr();
			_teams[0].FlagState = (u8)(csString[0] - '0');
			_teams[1].FlagState = (u8)(csString[1] - '0');
			_teams[0].PrevFlagState = _teams[0].FlagState;
			_teams[1].PrevFlagState = _teams[1].FlagState;
		}
	}
}

udtString udtCapturesAnalyzer::GetPlayerName(s32 playerIndex, udtBaseParser& parser)
{
	if(playerIndex < 0 || playerIndex >= 64)
	{
		return udtString::NewNull();
	}

	udtVMScopedStackAllocator allocScope(*_tempAllocator);

	const s32 csIndex = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol) + playerIndex;

	udtString playerName;
	if(!ParseConfigStringValueString(playerName, *_tempAllocator, "n", parser.GetConfigString(csIndex).GetPtr()))
	{
		return udtString::NewNull();
	}

	return udtString::NewCleanCloneFromRef(StringAllocator, parser._inProtocol, playerName);
}

bool udtCapturesAnalyzer::WasFlagPickedUpInBase(u32 teamIndex)
{
	if(teamIndex > 1)
	{
		return false;
	}

	const TeamInfo& team = _teams[teamIndex];
	const u8 prevFlagStatus = team.PrevFlagState;
	const u8 currFlagStatus = team.FlagState;
	const u8 flagStatus = currFlagStatus == (u8)idFlagStatus::Carried ? prevFlagStatus : currFlagStatus;
	const bool inBase = flagStatus == (u8)idFlagStatus::InBase;

	return inBase;
}

void udtCapturesAnalyzer::ProcessPlayerConfigStringQLorOSP(const char* configString, udtBaseParser& parser, s32 playerIndex)
{
	udtVMScopedStackAllocator tempAllocScope(*_tempAllocator);

	udtString name;
	if(ParseConfigStringValueString(name, *_tempAllocator, "n", configString))
	{
		_playerNames[playerIndex] = udtString::NewCloneFromRef(_playerNameAllocator, name);
	}
	else
	{
		_playerNames[playerIndex] = udtString::NewNull();
	}

	if(parser._inProtocol >= udtProtocol::Dm73 && parser._inProtocol <= udtProtocol::Dm90)
	{
		udtString clan;
		if(ParseConfigStringValueString(clan, *_tempAllocator, "cn", configString))
		{
			_playerClanNames[playerIndex] = udtString::NewCloneFromRef(_playerNameAllocator, clan);
		}
		else
		{
			_playerClanNames[playerIndex] = udtString::NewNull();
		}
	}
}

void udtCapturesAnalyzer::ProcessFlagStatusCommandQLorOSP(const udtCommandCallbackArg&, udtBaseParser& parser)
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

void udtCapturesAnalyzer::ProcessPrintCommandQLorOSP(const udtCommandCallbackArg&, udtBaseParser& parser)
{
	// QL : "^4BLUE TEAM^3 CAPTURED the flag!^7 (^4BREAK ^7whaz captured in 0:12.490)\n"
	// OSP: "^xFF00FF^6Raistlin^2 captured the BLUE flag! (held for 0:42.70)\n"

	idTokenizer& tokenizer = parser._context->Tokenizer;
	const udtString message = tokenizer.GetArg(1);
	const bool qlMode = udtString::ContainsNoCase(message, "CAPTURED the flag!");
	if(!qlMode &&
	   !udtString::ContainsNoCase(message, "captured the RED flag!") &&
	   !udtString::ContainsNoCase(message, "captured the BLUE flag!"))
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

	udtVMScopedStackAllocator tempAllocatorScope(*_tempAllocator);

	udtString playerName;
	if(qlMode)
	{
		playerName = udtString::NewSubstringClone(*_tempAllocator, message, leftParenIdx + 1, parenTextIdx - leftParenIdx - 2);
	}
	else
	{
		u32 capturedTheIdx = 0;
		if(!udtString::ContainsNoCase(capturedTheIdx, message, " captured the "))
		{
			return;
		}
		playerName = udtString::NewSubstringClone(*_tempAllocator, message, 0, capturedTheIdx);
	}
	const udtString cleanPlayerName = udtString::NewCleanCloneFromRef(StringAllocator, parser._inProtocol, playerName);

	const udtString parenText = capturedInFound ? capturedIn : heldFor;
	const u32 durationIdx = parenTextIdx + parenText.GetLength() + 1;
	if(durationIdx > rightParenIdx)
	{
		return;
	}

	const udtString duration = udtString::NewSubstringClone(*_tempAllocator, message, durationIdx, rightParenIdx - durationIdx);
	s32 captureDuration = 0;
	if(!ParseDuration(captureDuration, duration))
	{
		return;
	}

	const s32 time = parser._inServerTime;
	u32 flags = 0;
	if(capturedInFound)
	{
		flags |= (u32)udtParseDataCaptureMask::BaseToBase;
	}
	flags |= (u32)udtParseDataCaptureMask::PlayerNameValid;

	f32 distance = -1.0f;

	if(_lastCaptureQL.IsValid())
	{
		distance = _lastCaptureQL.Distance;
		flags |= (u32)udtParseDataCaptureMask::DistanceValid;
		_lastCaptureQL.Clear();
	}

	s32 playerIndex = -1;
	if(ExtractPlayerIndexFromCaptureMessageQLorOSP(playerIndex, playerName, parser._inProtocol))
	{
		flags |= (u32)udtParseDataCaptureMask::PlayerIndexValid;
	}

	udtParseDataCapture capture;
	capture.CaptureTimeMs = time;
	capture.Distance = distance;
	capture.Flags = flags;
	capture.GameStateIndex = _gameStateIndex;
	WriteStringToApiStruct(capture.MapName, _mapName);
	capture.PickUpTimeMs = time - captureDuration;
	capture.PlayerIndex = -1;
	WriteStringToApiStruct(capture.PlayerName, cleanPlayerName);
	Captures.Add(capture);
}

bool udtCapturesAnalyzer::ExtractPlayerIndexFromCaptureMessageQLorOSP(s32& playerIndex, const udtString& playerName, udtProtocol::Id protocol)
{
	// dm_73: sprintf(name, "%s %s", cn, n) OR sprintf(name, "%s^7 %s", cn, n)
	// dm_90: sprintf(name, "%s %s", cn, n) OR sprintf(name, "%s^7 %s", cn, n)
	// dm_91: sprintf(name, "%s", n)

	if(protocol == udtProtocol::Dm91)
	{
		for(s32 i = 0; i < 64; ++i)
		{
			if(!_playerNames[i].IsValid())
			{
				continue;
			}

			if(udtString::Equals(_playerNames[i], playerName))
			{
				playerIndex = i;
				return true;
			}
		}

		return false;
	}

	for(s32 i = 0; i < 64; ++i)
	{
		if(!_playerNames[i].IsValid() ||
		   !_playerClanNames[i].IsValid())
		{
			continue;
		}

		if(udtString::IsEmpty(_playerClanNames[i]) &&
		   udtString::Equals(_playerNames[i], playerName))
		{
			playerIndex = i;
			return true;
		}

		const udtString space = udtString::NewConstRef(" ");
		const udtString* formattedNameParts[] =
		{
			&_playerClanNames[i],
			&space,
			&_playerNames[i]
		};

		udtVMScopedStackAllocator tempAllocScope(*_tempAllocator);

		const udtString formattedName = udtString::NewFromConcatenatingMultiple(*_tempAllocator, formattedNameParts, UDT_COUNT_OF(formattedNameParts));
		if(udtString::Equals(formattedName, playerName))
		{
			playerIndex = i;
			return true;
		}

		const udtString spaceColor = udtString::NewConstRef("^7 ");
		const udtString* formattedNamePartsColor[] =
		{
			&_playerClanNames[i],
			&spaceColor,
			&_playerNames[i]
		};

		const udtString formattedNameColor = udtString::NewFromConcatenatingMultiple(*_tempAllocator, formattedNamePartsColor, UDT_COUNT_OF(formattedNamePartsColor));
		if(udtString::Equals(formattedNameColor, playerName))
		{
			playerIndex = i;
			return true;
		}
	}

	return false;
}

void udtCapturesAnalyzer::Clear()
{
	_playerNameAllocator.Clear();
	StringAllocator.Clear();
	Captures.Clear();
}
