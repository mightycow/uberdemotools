#include "analysis_obituaries.hpp"
#include "utils.hpp"


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


void udtObituariesAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.Snapshot->serverCommandNum == _lastProcessedServerCommandNumber)
	{
		return;
	}

	const s32 obituaryEvtId = parser._protocol == udtProtocol::Dm68 ? EV_OBITUARY : EV_OBITUARY_73;
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		const idEntityStateBase* const ent = arg.Entities[i];

		const s32 eventType = ent->eType & (~EV_EVENT_BITS);
		if(eventType == (s32)(ET_EVENTS + obituaryEvtId))
		{
			const s32 target = ent->otherEntityNum;
			if(target < 0 || target >= MAX_CLIENTS)
			{
				continue;
			}

			s32 attacker = ent->otherEntityNum2;
			if(attacker < 0 || attacker >= MAX_CLIENTS)
			{
				attacker = ENTITYNUM_WORLD;
			}

			const char* const targetName = AllocatePlayerName(parser, target);
			const char* const attackerName = AllocatePlayerName(parser, attacker);
			const s32 meanOfDeath = ent->eventParm;

			udtParseDataObituary info;
			info.MeanOfDeath = meanOfDeath;
			info.GameStateIndex = parser._inGameStateIndex;
			info.ServerTimeMs = arg.Snapshot->serverTime;
			info.TargetIdx = target;
			info.AttackerIdx = attacker;
			info.TargetName = targetName;
			info.AttackerName = attackerName;
			info.MeanOfDeathName = GetMeanOfDeathName(meanOfDeath, parser._protocol);
			Obituaries.Add(info);
		}
	}

	_tempAllocator.Clear();
	_lastProcessedServerCommandNumber = arg.Snapshot->serverCommandNum;
}

const char* udtObituariesAnalyzer::AllocatePlayerName(udtBaseParser& parser, s32 playerIdx)
{
	if(playerIdx == ENTITYNUM_WORLD)
	{
		return AllocateString(_playerNamesAllocator, "world");
	}

	const s32 firstPlayerCsIdx = parser._protocol == udtProtocol::Dm68 ? CS_PLAYERS_68 : CS_PLAYERS_73;
	udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(firstPlayerCsIdx + playerIdx);
	if(cs == NULL)
	{
		return NULL;
	}

	char* playerName = NULL;
	if(!ParseConfigStringValueString(playerName, _tempAllocator, "n", cs->String))
	{
		return NULL;
	}

	playerName = Q_CleanStr(playerName);

	return AllocateString(_playerNamesAllocator, playerName);
}
