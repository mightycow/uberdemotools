#include "protocol_conversion.hpp"
#include "scoped_stack_allocator.hpp"
#include "string.hpp"


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
	// @NOTE: Model indices are the same except protocol 90 adds a few. So no changes needed.
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

namespace udt_private
{
	static s32 ConvertEntityEventNumber90to68_helper(s32 eventId)
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
}

static s32 ConvertEntityEventNumber90to68(s32 eventId)
{
	const s32 eventSequenceBits = eventId & EV_EVENT_BITS;
	const s32 newEventId = udt_private::ConvertEntityEventNumber90to68_helper(eventId & (~EV_EVENT_BITS));
	
	return newEventId | eventSequenceBits;
}

s32 ConvertEntityModelIndex90to68VQ3(s32 modelIndex)
{
	switch((idItem90::Id)modelIndex)
	{
		case idItem90::Null: return (s32)idItem68_baseq3::Null;
		case idItem90::ItemArmorShard: return (s32)idItem68_baseq3::ItemArmorShard;
		case idItem90::ItemArmorCombat: return (s32)idItem68_baseq3::ItemArmorCombat;
		case idItem90::ItemArmorBody: return (s32)idItem68_baseq3::ItemArmorBody;
		case idItem90::ItemHealthSmall: return (s32)idItem68_baseq3::ItemHealthSmall;
		case idItem90::ItemHealth: return (s32)idItem68_baseq3::ItemHealth;
		case idItem90::ItemHealthLarge: return (s32)idItem68_baseq3::ItemHealthLarge;
		case idItem90::ItemHealthMega: return (s32)idItem68_baseq3::ItemHealthMega;
		case idItem90::WeaponGauntlet: return (s32)idItem68_baseq3::WeaponGauntlet;
		case idItem90::WeaponShotgun: return (s32)idItem68_baseq3::WeaponShotgun;
		case idItem90::WeaponMachinegun: return (s32)idItem68_baseq3::WeaponMachinegun;
		case idItem90::WeaponGrenadelauncher: return (s32)idItem68_baseq3::WeaponGrenadelauncher;
		case idItem90::WeaponRocketlauncher: return (s32)idItem68_baseq3::WeaponRocketlauncher;
		case idItem90::WeaponLightning: return (s32)idItem68_baseq3::WeaponLightning;
		case idItem90::WeaponRailgun: return (s32)idItem68_baseq3::WeaponRailgun;
		case idItem90::WeaponPlasmagun: return (s32)idItem68_baseq3::WeaponPlasmagun;
		case idItem90::WeaponBFG: return (s32)idItem68_baseq3::WeaponBFG;
		case idItem90::WeaponGrapplinghook: return (s32)idItem68_baseq3::WeaponGrapplinghook;
		case idItem90::AmmoShells: return (s32)idItem68_baseq3::AmmoShells;
		case idItem90::AmmoBullets: return (s32)idItem68_baseq3::AmmoBullets;
		case idItem90::AmmoGrenades: return (s32)idItem68_baseq3::AmmoGrenades;
		case idItem90::AmmoCells: return (s32)idItem68_baseq3::AmmoCells;
		case idItem90::AmmoLightning: return (s32)idItem68_baseq3::AmmoLightning;
		case idItem90::AmmoRockets: return (s32)idItem68_baseq3::AmmoRockets;
		case idItem90::AmmoSlugs: return (s32)idItem68_baseq3::AmmoSlugs;
		case idItem90::AmmoBFG: return (s32)idItem68_baseq3::AmmoBFG;
		case idItem90::HoldableTeleporter: return (s32)idItem68_baseq3::HoldableTeleporter;
		case idItem90::HoldableMedkit: return (s32)idItem68_baseq3::HoldableMedkit;
		case idItem90::ItemQuad: return (s32)idItem68_baseq3::ItemQuad;
		case idItem90::ItemEnviro: return (s32)idItem68_baseq3::ItemEnviro;
		case idItem90::ItemHaste: return (s32)idItem68_baseq3::ItemHaste;
		case idItem90::ItemInvis: return (s32)idItem68_baseq3::ItemInvis;
		case idItem90::ItemRegen: return (s32)idItem68_baseq3::ItemRegen;
		case idItem90::ItemFlight: return (s32)idItem68_baseq3::ItemFlight;
		case idItem90::TeamCTFRedflag: return (s32)idItem68_baseq3::TeamCTFRedflag;
		case idItem90::TeamCTFBlueflag: return (s32)idItem68_baseq3::TeamCTFBlueflag;
		// Unsupported items:
		case idItem90::ItemArmorJacket:
			return (s32)idItem68_baseq3::ItemArmorBody; // Make greens look like yellows.
		case idItem90::HoldableKamikaze:
		case idItem90::HoldablePortal:
		case idItem90::HoldableInvulnerability:
		case idItem90::AmmoNails:
		case idItem90::AmmoMines:
		case idItem90::AmmoBelt:
		case idItem90::ItemScout:
		case idItem90::ItemGuard:
		case idItem90::ItemDoubler:
		case idItem90::ItemAmmoregen:
		case idItem90::TeamCTFNeutralflag:
		case idItem90::ItemRedcube:
		case idItem90::ItemBluecube:
		case idItem90::WeaponNailgun:
		case idItem90::WeaponProxLauncher:
		case idItem90::WeaponChaingun:
		case idItem90::ItemSpawnarmor:
		case idItem90::WeaponHMG:
		case idItem90::AmmoHMG:
		case idItem90::AmmoPack:
		default: 
			return (s32)idItem68_baseq3::Null;
	}
}

s32 ConvertEntityModelIndex90to68CPMA(s32 modelIndex)
{
	switch((idItem90::Id)modelIndex)
	{
		case idItem90::Null: return (s32)idItem68_CPMA::Null;
		case idItem90::ItemArmorShard: return (s32)idItem68_CPMA::ItemArmorShard;
		case idItem90::ItemArmorCombat: return (s32)idItem68_CPMA::ItemArmorCombat;
		case idItem90::ItemArmorBody: return (s32)idItem68_CPMA::ItemArmorBody;
		case idItem90::ItemArmorJacket: return (s32)idItem68_CPMA::ItemArmorJacket;
		case idItem90::ItemHealthSmall: return (s32)idItem68_CPMA::ItemHealthSmall;
		case idItem90::ItemHealth: return (s32)idItem68_CPMA::ItemHealth;
		case idItem90::ItemHealthLarge: return (s32)idItem68_CPMA::ItemHealthLarge;
		case idItem90::ItemHealthMega: return (s32)idItem68_CPMA::ItemHealthMega;
		case idItem90::WeaponGauntlet: return (s32)idItem68_CPMA::WeaponGauntlet;
		case idItem90::WeaponShotgun: return (s32)idItem68_CPMA::WeaponShotgun;
		case idItem90::WeaponMachinegun: return (s32)idItem68_CPMA::WeaponMachinegun;
		case idItem90::WeaponGrenadelauncher: return (s32)idItem68_CPMA::WeaponGrenadelauncher;
		case idItem90::WeaponRocketlauncher: return (s32)idItem68_CPMA::WeaponRocketlauncher;
		case idItem90::WeaponLightning: return (s32)idItem68_CPMA::WeaponLightning;
		case idItem90::WeaponRailgun: return (s32)idItem68_CPMA::WeaponRailgun;
		case idItem90::WeaponPlasmagun: return (s32)idItem68_CPMA::WeaponPlasmagun;
		case idItem90::WeaponBFG: return (s32)idItem68_CPMA::WeaponBFG;
		case idItem90::WeaponGrapplinghook: return (s32)idItem68_CPMA::WeaponGrapplinghook;
		case idItem90::AmmoShells: return (s32)idItem68_CPMA::AmmoShells;
		case idItem90::AmmoBullets: return (s32)idItem68_CPMA::AmmoBullets;
		case idItem90::AmmoGrenades: return (s32)idItem68_CPMA::AmmoGrenades;
		case idItem90::AmmoCells: return (s32)idItem68_CPMA::AmmoCells;
		case idItem90::AmmoLightning: return (s32)idItem68_CPMA::AmmoLightning;
		case idItem90::AmmoRockets: return (s32)idItem68_CPMA::AmmoRockets;
		case idItem90::AmmoSlugs: return (s32)idItem68_CPMA::AmmoSlugs;
		case idItem90::AmmoBFG: return (s32)idItem68_CPMA::AmmoBFG;
		case idItem90::HoldableTeleporter: return (s32)idItem68_CPMA::HoldableTeleporter;
		case idItem90::HoldableMedkit: return (s32)idItem68_CPMA::HoldableMedkit;
		case idItem90::ItemQuad: return (s32)idItem68_CPMA::ItemQuad;
		case idItem90::ItemEnviro: return (s32)idItem68_CPMA::ItemEnviro;
		case idItem90::ItemHaste: return (s32)idItem68_CPMA::ItemHaste;
		case idItem90::ItemInvis: return (s32)idItem68_CPMA::ItemInvis;
		case idItem90::ItemRegen: return (s32)idItem68_CPMA::ItemRegen;
		case idItem90::ItemFlight: return (s32)idItem68_CPMA::ItemFlight;
		case idItem90::TeamCTFRedflag: return (s32)idItem68_CPMA::TeamCTFRedflag;
		case idItem90::TeamCTFBlueflag: return (s32)idItem68_CPMA::TeamCTFBlueflag;
		case idItem90::TeamCTFNeutralflag: return (s32)idItem68_CPMA::TeamCTFNeutralflag;
			// Unsupported items.
		case idItem90::AmmoPack: 
			return (s32)idItem68_CPMA::ItemBackpack;
		case idItem90::HoldableKamikaze:
		case idItem90::HoldablePortal:
		case idItem90::HoldableInvulnerability:
		case idItem90::AmmoNails:
		case idItem90::AmmoMines:
		case idItem90::AmmoBelt:
		case idItem90::ItemScout:
		case idItem90::ItemGuard:
		case idItem90::ItemDoubler:
		case idItem90::ItemAmmoregen:
		case idItem90::ItemRedcube:
		case idItem90::ItemBluecube:
		case idItem90::WeaponNailgun:
		case idItem90::WeaponProxLauncher:
		case idItem90::WeaponChaingun:
		case idItem90::ItemSpawnarmor:
		case idItem90::WeaponHMG:
		case idItem90::AmmoHMG:
		default:
			return (s32)idItem68_CPMA::Null;
	}
}

static void ConvertSnapshot90to68(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
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

	// @FIXME: LG hit beeps repeating far too often.
}

static void ConvertEntityState90to68(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	CopyEntityState<idEntityState68>(outEntityState, inEntityState);
	outEntityState.weapon = ConvertWeapon90to68(inEntityState.weapon);
	outEntityState.event = ConvertEntityEventNumber90to68(inEntityState.event);

	// The type can encode an event, so make sure we convert that too.
	if(inEntityState.eType >= ET_EVENTS)
	{
		outEntityState.eType = ET_EVENTS + ConvertEntityEventNumber90to68(inEntityState.eType - ET_EVENTS);
	}
	
	if(inEntityState.eType == ET_ITEM)
	{
		outEntityState.modelindex = ConvertEntityModelIndex90to68CPMA(inEntityState.modelindex);
	}
	else
	{
		// @TODO: investigate
		outEntityState.modelindex = 0;
	}

	// Silence those annoying sounds...
	if(inEntityState.eType == ET_SPEAKER)
	{
		outEntityState.eType = -1;
	}

	// LG sounds repeating too many times...
	// Should probably be able to fix the event bits provided
	// the function can know if this event is a repeat or not.
	if(inEntityState.eType >= ET_EVENTS)
	{
		const s32 eventId = (inEntityState.eType - ET_EVENTS) & (~EV_EVENT_BITS);
		if(eventId == EV_FIRE_WEAPON_73p &&
		   inEntityState.weapon == idWeapon73p::LightningGun)
		{
			// Not sure why the audio repeats in q3mme and not in QL.
			outEntityState.eType = ET_EVENTS + EV_NONE;
			outEntityState.event = EV_NONE;
		}
	}
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

// Return false to drop the key/value pair altogether.
typedef bool (*ProcessConfigStringCallback)(udtString& newValue, udtVMLinearAllocator& allocator, const udtString& key, const udtString& value);

static void ProcessConfigString(udtString& result, udtVMLinearAllocator& allocator, const udtString& input, ProcessConfigStringCallback callback)
{
	const udtString separator = udtString::NewConstRef("\\");
	result = udtString::NewEmpty(allocator, BIG_INFO_STRING);
	const char* key = input.String + 1;
	const char* value = NULL;
	for(;;)
	{
		value = strchr(key, '\\');
		if(value == NULL)
		{
			break;
		}

		value += 1;
		if(*value == '\0')
		{
			break;
		}

		bool done = false;
		const char* sepBeforeNextKey = strchr(value, '\\');
		if(sepBeforeNextKey == NULL)
		{
			sepBeforeNextKey = input.String + input.Length;
			done = true;
		}

		udtVMScopedStackAllocator allocatorScope(allocator);
		const udtString keyString = udtString::NewClone(allocator, key, (u32)(value - 1 - key));
		const udtString valueString = udtString::NewClone(allocator, value, (u32)(sepBeforeNextKey - value));

		key = sepBeforeNextKey + 1;

		udtString newValueString;
		if(!(*callback)(newValueString, allocator, keyString, valueString))
		{
			continue;
		}

		const udtString* toAppend[4] = { &separator, &keyString, &separator, &newValueString };
		udtString::AppendMultiple(result, toAppend, (u32)UDT_COUNT_OF(toAppend));

		if(done)
		{
			break;
		}
	}
}

static bool ConvertConfigStringValue90to68(udtString& newValue, udtVMLinearAllocator&, const udtString& key, const udtString& value)
{
	newValue = value;

	u32 index = 0;
	if(udtString::Equals(key, "gamename"))
	{
		newValue = udtString::NewConstRef("cpma");
		return true;
	}

	if(udtString::ContainsNoCase(index, key, "paks") || udtString::ContainsNoCase(index, key, "pakNames"))
	{
		return false;
	}

	if(udtString::Equals(key, "protocol"))
	{
		newValue = udtString::NewConstRef("68");
		return true;
	}

	if(udtString::Equals(key, "mapname"))
	{
		if(udtString::Equals(value, "bloodrun"))
		{
			newValue = udtString::NewConstRef("ztn3dm1");
		}
		else if(udtString::Equals(value, "lostworld"))
		{
			newValue = udtString::NewConstRef("q3dm13");
		}
		else if(udtString::Equals(value, "battleforged"))
		{
			newValue = udtString::NewConstRef("phantq3dm1");
		}

		return true;
	}

	return true;
}

static void ConvertConfigString90to68(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* string, u32 stringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex90to68(inIndex);
	result.String = string;
	result.StringLength = stringLength;

	if(inIndex == CS_GAME_VERSION_73p)
	{
		result.NewString = true;
		result.String = "cpma-1";
		result.StringLength = (u32)strlen("cpma-1");
	}

	if(inIndex == CS_SERVERINFO || inIndex == CS_SYSTEMINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(string, stringLength), &ConvertConfigStringValue90to68);
		result.NewString = true;
		result.String = newString.String;
		result.StringLength = newString.Length;
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
		converter = udtProtocolConverter();
		return;
	}

	converter = ProtocolConverters[converterIdx];
}
