#include "protocol_conversion.hpp"


template<typename T>
static void CopySnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	memcpy(&outSnapshot, &inSnapshot, sizeof(T));
}

template<typename T>
static void CopyEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	memcpy(&outEntityState, &inEntityState, sizeof(T));
}

static void DontChangeConfigStrings(udtConfigStringConversion& result, udtVMLinearAllocator&, s32 inIndex, const char* string, u32 stringLength)
{
	result.NewString = false;
	result.Index = inIndex;
	result.String = string;
	result.StringLength = stringLength;
}

static void ConvertSnapshot73to90(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	*GetPlayerState(&outSnapshot, udtProtocol::Dm90) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm73);
	idPlayerState90& out = *(idPlayerState90*)GetPlayerState(&outSnapshot, udtProtocol::Dm90);
	//idPlayerState73& in = *(idPlayerState73*)GetPlayerState((idClientSnapshotBase*)&inSnapshot, inProtocol);
	out.doubleJumped = 0;
	out.jumpTime = 0;
	out.unknown1 = 0;
	out.unknown2 = 0;
	out.unknown3 = 0;
	out.pm_flags = 0; // @NOTE: This field is 24 bits large in protocol 90, 16 bits large in protocol 73. 
	// @TODO: Investigate this. Output demo invalid if only copying first 16 bits (in_flags & 0xFFFF).
}

static void ConvertEntityState73to90(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	idEntityState90& out = (idEntityState90&)outEntityState;
	idEntityState73& in = (idEntityState73&)inEntityState;
	(idEntityStateBase&)outEntityState = inEntityState;
	out.pos_gravity = in.pos_gravity;
	out.apos_gravity = in.apos_gravity;
	out.jumpTime = 0;
	out.doubleJumped = 0;
}

static int ConvertWeapon90to68(int weapon)
{
	return weapon <= 10 ? weapon : 0;
}

static s32 ConvertEntityEventNumber90to68_impl(s32 eventId)
{
	if(eventId >= 0 && eventId <= 5)
	{
		return eventId;
	}

	if(eventId >= 6 && eventId <= 18)
	{
		return eventId + 4;
	}

	if(eventId >= EV_USE_ITEM0_73p && eventId < EV_USE_ITEM0_73p + 16)
	{
		return eventId - EV_USE_ITEM0_73p + EV_USE_ITEM0_68;
	}

	if(eventId >= 37 && eventId <= 51 && eventId != EV_BULLET_HIT_FLESH_73p)
	{
		return eventId + 3;
	}

	if(eventId >= 53 && eventId <= 56)
	{
		return eventId + 3;
	}

	if(eventId >= 58 && eventId <= 61)
	{
		return eventId + 2;
	}

	if(eventId >= 63 && eventId <= 70)
	{
		return eventId + 1;
	}

	if(eventId >= 72 && eventId <= 80)
	{
		return eventId + 2;
	}

	switch(eventId)
	{
		case EV_DROP_WEAPON_73p: return EV_NONE;
		case EV_FIRE_WEAPON_73p: return EV_FIRE_WEAPON_68;
		case EV_BULLET_HIT_FLESH_73p: return EV_BULLET;
		case 52: // ???
		case 57: // drown
		case 62: // armor regen?
		case 71: // ???
			return EV_NONE;

		default: return eventId;
	}
}

static s32 ConvertEntityEventNumber90to68(s32 eventId)
{
	const s32 eventSequenceBits = eventId & EV_EVENT_BITS;
	const s32 newEventId = ConvertEntityEventNumber90to68_impl(eventId & (~EV_EVENT_BITS));
	
	return newEventId | eventSequenceBits;
}

static void ConvertSnapshot90to68(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	//(idClientSnapshotBase&)outSnapshot = inSnapshot;
	//*GetPlayerState(&outSnapshot, udtProtocol::Dm68) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm90);
	idClientSnapshot68& out = (idClientSnapshot68&)outSnapshot;
	idClientSnapshot90& in = (idClientSnapshot90&)inSnapshot;
	(idPlayerStateBase&)out.ps = (idPlayerStateBase&)in.ps;

	out.ps.weapon = ConvertWeapon90to68(in.ps.weapon);

	for(int i = 0; i < MAX_STATS; ++i)
	{
		out.ps.stats[i] = 0;
	}
	out.ps.stats[STAT_HEALTH_68] = in.ps.stats[STAT_HEALTH_73p];
	out.ps.stats[STAT_HOLDABLE_ITEM_68] = in.ps.stats[STAT_HOLDABLE_ITEM_73p];
	out.ps.stats[STAT_WEAPONS_68] = in.ps.stats[STAT_WEAPONS_73p];
	out.ps.stats[STAT_ARMOR_68] = in.ps.stats[STAT_ARMOR_73p];
	out.ps.stats[STAT_CLIENTS_READY_68] = in.ps.stats[STAT_CLIENTS_READY_73p];
	out.ps.stats[STAT_MAX_HEALTH_68] = in.ps.stats[STAT_MAX_HEALTH_73p];

	for(int i = 0; i < MAX_PERSISTANT; ++i)
	{
		out.ps.persistant[i] = 0;
	}
	out.ps.persistant[PERS_SCORE_68] = in.ps.persistant[PERS_SCORE_73p];
	out.ps.persistant[PERS_HITS_68] = in.ps.persistant[PERS_HITS_73p];
	out.ps.persistant[PERS_RANK_68] = in.ps.persistant[PERS_RANK_73p];
	out.ps.persistant[PERS_TEAM_68] = in.ps.persistant[PERS_TEAM_73p];
	out.ps.persistant[PERS_SPAWN_COUNT_68] = in.ps.persistant[PERS_SPAWN_COUNT_73p];
	out.ps.persistant[PERS_PLAYEREVENTS_68] = in.ps.persistant[PERS_PLAYEREVENTS_73p];
	out.ps.persistant[PERS_ATTACKER_68] = in.ps.persistant[PERS_ATTACKER_73p];
	out.ps.persistant[PERS_KILLED_68] = in.ps.persistant[PERS_KILLED_73p];
	out.ps.persistant[PERS_IMPRESSIVE_COUNT_68] = in.ps.persistant[PERS_IMPRESSIVE_COUNT_73p];
	out.ps.persistant[PERS_EXCELLENT_COUNT_68] = in.ps.persistant[PERS_EXCELLENT_COUNT_73p];
	out.ps.persistant[PERS_DEFEND_COUNT_68] = in.ps.persistant[PERS_DEFEND_COUNT_73p];
	out.ps.persistant[PERS_ASSIST_COUNT_68] = in.ps.persistant[PERS_ASSIST_COUNT_73p];
	out.ps.persistant[PERS_GAUNTLET_FRAG_COUNT_68] = in.ps.persistant[PERS_GAUNTLET_FRAG_COUNT_73p];
	out.ps.persistant[PERS_CAPTURES_68] = in.ps.persistant[PERS_CAPTURES_73p];
	out.ps.persistant[PERS_ATTACKEE_ARMOR_68] = in.ps.persistant[PERS_ATTACKEE_ARMOR_73p];

	out.ps.events[0] = ConvertEntityEventNumber90to68(in.ps.events[0]);
	out.ps.events[1] = ConvertEntityEventNumber90to68(in.ps.events[1]);
	out.ps.externalEvent = ConvertEntityEventNumber90to68(in.ps.externalEvent);
}

static void ConvertEntityState90to68(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	//(idEntityStateBase&)outEntityState = inEntityState;
	CopyEntityState<idEntityState68>(outEntityState, inEntityState);
	//for(int i = 0; i < 3; ++i) outEntityState.origin[i] = inEntityState.origin[i];
	//for(int i = 0; i < 3; ++i) outEntityState.origin2[i] = inEntityState.origin2[i];
	outEntityState.weapon = ConvertWeapon90to68(inEntityState.weapon);
	outEntityState.event = ConvertEntityEventNumber90to68(inEntityState.event);
}

static s32 ConvertConfigStringIndex90to68(s32 index)
{
	if(index >= CS_MODELS_73p && index < CS_MODELS_73p + MAX_MODELS)
	{
		return index - CS_MODELS_73p + CS_MODELS_68;
	}

	if(index >= CS_SOUNDS_73p && index < CS_PLAYERS_73p)
	{
		return index - CS_SOUNDS_73p + CS_SOUNDS_68;
	}

	if(index >= CS_PLAYERS_73p && index < CS_PLAYERS_73p + MAX_CLIENTS)
	{
		return index - CS_PLAYERS_73p + CS_PLAYERS_68;
	}

	if(index >= CS_LOCATIONS_73p && index < CS_LOCATIONS_73p + MAX_LOCATIONS)
	{
		return index - CS_LOCATIONS_73p + CS_LOCATIONS_68;
	}

	if(index >= CS_PARTICLES_73p && index < CS_PARTICLES_73p + MAX_LOCATIONS)
	{
		return index - CS_PARTICLES_73p + CS_PARTICLES_68;
	}

	switch(index)
	{
		case CS_GAME_VERSION_73p: return CS_GAME_VERSION_68;
		case CS_LEVEL_START_TIME_73p: return CS_LEVEL_START_TIME_68;
		case CS_INTERMISSION_73p: return CS_INTERMISSION_68;
		case CS_FLAGSTATUS_73p: return CS_FLAGSTATUS_68;
		case CS_SHADERSTATE_73p: return CS_SHADERSTATE_68;
		case CS_ITEMS_73p: return CS_ITEMS_68;
		default:
			if(index >= CS_PAST_LAST_INDEX_68) return -1;
			return index;
	}
}

static const char* ConfigStringServerInfo68 = R"(\g_teamSizeMin\1\g_compmode\1\sv_hostname\ mejtisson\sv_advertising\0\g_voteFlags\2762\sv_owner\mejtisson\ruleset\1\g_gametype\1\sv_ranked\1\sv_maxclients\16\fraglimit\0\g_overtime\120\gt_realm\quakelive\sv_location\DE\sv_premium\1\timelimit\10\sv_allowDownload\1\version\Quake Live  0.1.0.947 linux-i386 Sep 12 2014 11:46:11\dmflags\0\protocol\90\mapname\ztn3dm1\sv_privateClients\0\sv_gtid\597071\sv_adXmitDelay\300000\sv_skillRating\22\g_levelStartTime\1411688816\gamename\baseq3\g_adCaptureScoreBonus\3\g_adElimScoreBonus\2\g_adTouchScoreBonus\1\g_aspectEnable\0\capturelimit\8\g_customSettings\0\g_freezeRoundDelay\4000\g_gameState\PRE_GAME\g_gravity\800\g_holiday\0\g_instaGib\0\g_loadout\0\g_maxGameClients\0\g_maxSkillTier\0\g_maxStandardClients\0\mercylimit\0\g_needpass\0\g_quadDamageFactor\3\g_raceAllowStandard\0\g_roundWarmupDelay\10000\roundlimit\5\roundtimelimit\180\scorelimit\150\g_startingHealth\100\g_teamForceBalance\1\teamsize\0\g_timeoutCount\3\g_weaponrespawn\5)";
static const char* ConfigStringSystemInfo68 = R"(\sv_cheats\0\sv_pure\1\timescale\1\sv_serverid\2858499\g_skipTrainingEnable\0\g_training\0)";
static const char* ConfigStringGameVersion68 = "baseq3-1";

static void ConvertConfigString90to68(udtConfigStringConversion& result, udtVMLinearAllocator&, s32 inIndex, const char* string, u32 stringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex90to68(inIndex);
	result.String = string;
	result.StringLength = stringLength;

	// Override baseqz with baseq3-1.
	if(inIndex == CS_GAME_VERSION_73p)
	{
		result.String = ConfigStringGameVersion68;
		result.StringLength = (u32)strlen(ConfigStringGameVersion68);
	}
	
	// For debugging/testing.
	if(inIndex == CS_SERVERINFO)
	{
		result.String = ConfigStringServerInfo68;
		result.StringLength = (u32)strlen(ConfigStringServerInfo68);
	}
	else if(inIndex == CS_SYSTEMINFO)
	{
		result.String = ConfigStringSystemInfo68;
		result.StringLength = (u32)strlen(ConfigStringSystemInfo68);
	}
}

static const udtProtocolConverter ProtocolConverters[udtProtocol::Count * udtProtocol::Count] =
{
	{ &CopySnapshot<idClientSnapshot68>, &CopyEntityState<idEntityState68>, &DontChangeConfigStrings },
	{ NULL, NULL, NULL }, // 68 => 73
	{ NULL, NULL, NULL }, // 68 => 90
	{ NULL, NULL, NULL }, // 73 => 68
	{ &CopySnapshot<idClientSnapshot73>, &CopyEntityState<idEntityState73>, &DontChangeConfigStrings },
	{ &ConvertSnapshot73to90, &ConvertEntityState73to90, &DontChangeConfigStrings },
	{ &ConvertSnapshot90to68, &ConvertEntityState90to68, &ConvertConfigString90to68 },
	{ NULL, NULL, NULL }, // 90 => 73
	{ &CopySnapshot<idClientSnapshot90>, &CopyEntityState<idEntityState90>, &DontChangeConfigStrings }
};

void GetProtocolConverter(udtProtocolConverter& converter, udtProtocol::Id outProtocol, udtProtocol::Id inProtocol)
{
	const u32 converterIdx = ((u32)(inProtocol - 1) * (u32)udtProtocol::Count) + (u32)(outProtocol - 1);
	if(converterIdx >= (u32)UDT_COUNT_OF(ProtocolConverters))
	{
		converter.ConvertSnapshot = NULL;
		converter.ConvertEntityState = NULL;
		return;
	}

	converter = ProtocolConverters[converterIdx];
}
