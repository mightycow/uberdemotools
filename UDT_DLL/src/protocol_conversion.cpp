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
}

static void ConvertEntityState90to68(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	//(idEntityStateBase&)outEntityState = inEntityState;
	CopyEntityState<idEntityState68>(outEntityState, inEntityState);
	//for(int i = 0; i < 3; ++i) outEntityState.origin[i] = inEntityState.origin[i];
	//for(int i = 0; i < 3; ++i) outEntityState.origin2[i] = inEntityState.origin2[i];
	outEntityState.weapon = ConvertWeapon90to68(inEntityState.weapon);
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
			return index;
	}
}

static void ConvertConfigString90to68(udtConfigStringConversion& result, udtVMLinearAllocator&, s32 inIndex, const char* string, u32 stringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex90to68(inIndex);
	result.String = string;
	result.StringLength = stringLength;
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
