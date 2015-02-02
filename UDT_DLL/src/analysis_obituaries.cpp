#include "analysis_obituaries.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


static const char* meansOfDeath_Q3[] =
{
	"unknown",
	"shotgun",
	"gauntlet",
	"machine gun",
	"grenade",
	"grenade splash",
	"rocket",
	"rocket splash",
	"plasma",
	"plasma splash",
	"railgun",
	"lightning",
	"BFG",
	"BFG splash",
	"water",
	"slime",
	"lava",
	"crush",
	"telefrag",
	"fall",
	"suicide",
	"target laser",
	"trigger hurt",
	"grapple"
};

static const char* meansOfDeath_QL[] =
{
	"unknown",
	"shotgun",
	"gauntlet",
	"machine gun",
	"grenade",
	"grenade splash",
	"rocket",
	"rocket splash",
	"plasma",
	"plasma splash",
	"railgun",
	"lightning",
	"BFG",
	"BFG splash",
	"water",
	"slime",
	"lava",
	"crush",
	"telefrag",
	"fall",
	"suicide",
	"target laser",
	"trigger hurt",
// mission pack start
	"nailgun",
	"chaingun",
	"proximity mine",
	"kamikaze",
	"juiced",
// mission pack end
	"grapple",
// QL start
	"team switch",
	"thaw",
	"unknown",
	"heavy machine gun"
// QL end
};


const char* GetMeanOfDeathName(s32 mod, udtProtocol::Id protocol)
{
	if(protocol == udtProtocol::Dm68)
	{
		if(mod < 0 || mod >= (s32)UDT_COUNT_OF(meansOfDeath_Q3))
		{
			mod = 0;
		}

		return meansOfDeath_Q3[mod];
	}

	if(mod < 0 || mod >= (s32)UDT_COUNT_OF(meansOfDeath_QL))
	{
		mod = 0;
	}

	return meansOfDeath_QL[mod];
}


void udtObituariesAnalyzer::InitAllocators(u32 demoCount, udtVMLinearAllocator& finalAllocator, udtVMLinearAllocator& tempAllocator)
{
	if(_enableNameAllocation)
	{
		_playerNamesAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	}

	finalAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	_tempAllocator = &tempAllocator;
	Obituaries.SetAllocator(finalAllocator);
}

void udtObituariesAnalyzer::ResetForNextDemo()
{
	_gameStateIndex = -1;

	for(u32 i = 0; i < 64; ++i)
	{
		_playerTeams[i] = -1;
	}
}

void udtObituariesAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	const s32 obituaryEvtId = parser._inProtocol == udtProtocol::Dm68 ? (s32)EV_OBITUARY_68 : (s32)EV_OBITUARY_73p;
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		if(!arg.Entities[i].IsNewEvent)
		{
			continue;
		}

		const idEntityStateBase* const ent = arg.Entities[i].Entity;
		const s32 eventType = ent->eType & (~EV_EVENT_BITS);
		if(eventType != (s32)(ET_EVENTS + obituaryEvtId))
		{
			continue;
		}

		const s32 targetIdx = ent->otherEntityNum;
		if(targetIdx < 0 || targetIdx >= MAX_CLIENTS)
		{
			continue;
		}

		s32 attackerIdx = ent->otherEntityNum2;
		if(attackerIdx < 0 || attackerIdx >= MAX_CLIENTS)
		{
			attackerIdx = ENTITYNUM_WORLD;
		}

		const s32 targetTeamIdx = _playerTeams[targetIdx];
		const s32 attackerTeamIdx = (attackerIdx == ENTITYNUM_WORLD) ? -1 : _playerTeams[attackerIdx];
		const char* const targetName = AllocatePlayerName(parser, targetIdx);
		const char* const attackerName = AllocatePlayerName(parser, attackerIdx);
		const s32 meanOfDeath = ent->eventParm;

		udtParseDataObituary info;
		info.TargetTeamIdx = targetTeamIdx;
		info.AttackerTeamIdx = attackerTeamIdx;
		info.MeanOfDeath = meanOfDeath;
		info.GameStateIndex = parser._inGameStateIndex;
		info.ServerTimeMs = arg.Snapshot->serverTime;
		info.TargetIdx = targetIdx;
		info.AttackerIdx = attackerIdx;
		info.TargetName = targetName;
		info.AttackerName = attackerName;
		info.MeanOfDeathName = GetMeanOfDeathName(meanOfDeath, parser._inProtocol);
		Obituaries.Add(info);
	}
}

const char* udtObituariesAnalyzer::AllocatePlayerName(udtBaseParser& parser, s32 playerIdx)
{
	if(!_enableNameAllocation)
	{
		return NULL;
	}

	if(playerIdx == ENTITYNUM_WORLD)
	{
		return AllocateString(_playerNamesAllocator, "world");
	}

	const s32 firstPlayerCsIdx = parser._inProtocol == udtProtocol::Dm68 ? CS_PLAYERS_68 : CS_PLAYERS_73p;
	udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(firstPlayerCsIdx + playerIdx);
	if(cs == NULL)
	{
		return NULL;
	}

	udtString playerName;
	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);
	if(!ParseConfigStringValueString(playerName, *_tempAllocator, "n", cs->String))
	{
		return NULL;
	}

	udtString::CleanUp(playerName);

	return AllocateString(_playerNamesAllocator, playerName.String);
}

void udtObituariesAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	const s32 csFirstPlayerIdx = parser._inProtocol == udtProtocol::Dm68 ? (s32)CS_PLAYERS_68 : (s32)CS_PLAYERS_73p;
	for(s32 i = 0; i < 64; ++i)
	{
		udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(csFirstPlayerIdx + i);
		if(cs != NULL)
		{
			udtVMScopedStackAllocator tempAllocScope(*_tempAllocator);
			ParseConfigStringValueInt(_playerTeams[i], *_tempAllocator, "t", cs->String);
		}
	}
}

void udtObituariesAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& parser)
{
	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	const int tokenCount = tokenizer.GetArgCount();
	if(strcmp(tokenizer.GetArgString(0), "cs") != 0 || tokenCount != 3)
	{
		return;
	}

	s32 csIndex = -1;
	if(!StringParseInt(csIndex, tokenizer.GetArgString(1)))
	{
		return;
	}

	const s32 csFirstPlayerIdx = parser._inProtocol == udtProtocol::Dm68 ? (s32)CS_PLAYERS_68 : (s32)CS_PLAYERS_73p;
	const s32 playerIdx = csIndex - csFirstPlayerIdx;
	if(playerIdx < 0 || playerIdx >= 64)
	{
		return;
	}

	udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(csIndex);
	if(cs == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator tempAllocScope(*_tempAllocator);
	ParseConfigStringValueInt(_playerTeams[playerIdx], *_tempAllocator, "t", cs->String);
}
