#include "protocol_conversion.hpp"
#include "scoped_stack_allocator.hpp"
#include "string.hpp"
#include "utils.hpp"


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

static s32 ConvertEntityType90to68(s32 index)
{
	if(index >= 0 && index < ET_EVENTS)
	{
		return index;
	}

	if(index >= ET_EVENTS)
	{
		return ET_EVENTS + udt_private::ConvertEntityEventNumber90to68_helper(index - ET_EVENTS);
	}

	return ET_GENERAL;
}

static s32 ConvertEntityModelIndex90to68_CPMA(s32 modelIndex)
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
typedef bool (*ProcessConfigStringCallback)(udtString& newValue, udtVMLinearAllocator& allocator, const udtString& key, const udtString& value, void* userData);

static void ProcessConfigString(udtString& result, udtVMLinearAllocator& allocator, const udtString& input, ProcessConfigStringCallback callback, void* userData)
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
		if(!(*callback)(newValueString, allocator, keyString, valueString, userData))
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

static bool ConvertConfigStringValue73to90(udtString& newValue, udtVMLinearAllocator&, const udtString& key, const udtString& value, void*)
{
	newValue = udtString::Equals(key, "protocol") ? udtString::NewConstRef("90") : value;

	return true;
}

static bool FindMap(f32* offsets, udtString& outputName, const udtString& inputName, const udtProtocolConversionArg& info)
{
	if(info.MapRuleCount == 0 || info.MapRules == NULL)
	{
		return false;
	}

	for(u32 i = 0; i < info.MapRuleCount; ++i)
	{
		const udtMapConversionRule& rule = info.MapRules[i];
		if(udtString::Equals(inputName, rule.InputName))
		{
			outputName = udtString::NewConstRef(rule.OutputName);
			Float3::Copy(offsets, rule.PositionOffsets);
			return true;
		}
	}

	return false;
}

static bool ConvertConfigStringValue90to68(udtString& newValue, udtVMLinearAllocator&, const udtString& key, const udtString& value, void* userData)
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
		udtProtocolConverter* const converter = (udtProtocolConverter*)userData;
		f32 offsets[3];
		udtString newMapName;
		if(converter != NULL && FindMap(offsets, newMapName, value, *converter->ConversionInfo))
		{
			Float3::Copy(converter->Offsets, offsets);
			newValue = newMapName;
		}

		return true;
	}

	return true;
}

static s32 ConvertMeanOfDeath90to68(s32 mod)
{
	switch((idMeansOfDeath73p::Id)mod)
	{
		case idMeansOfDeath73p::Shotgun: return (s32)idMeansOfDeath68::Shotgun;
		case idMeansOfDeath73p::Gauntlet: return (s32)idMeansOfDeath68::Gauntlet;
		case idMeansOfDeath73p::MachineGun: return (s32)idMeansOfDeath68::MachineGun;
		case idMeansOfDeath73p::Grenade: return (s32)idMeansOfDeath68::Grenade;
		case idMeansOfDeath73p::GrenadeSplash: return (s32)idMeansOfDeath68::GrenadeSplash;
		case idMeansOfDeath73p::Rocket: return (s32)idMeansOfDeath68::Rocket;
		case idMeansOfDeath73p::RocketSplash: return (s32)idMeansOfDeath68::RocketSplash;
		case idMeansOfDeath73p::Plasma: return (s32)idMeansOfDeath68::Plasma;
		case idMeansOfDeath73p::PlasmaSplash: return (s32)idMeansOfDeath68::PlasmaSplash;
		case idMeansOfDeath73p::RailGun: return (s32)idMeansOfDeath68::RailGun;
		case idMeansOfDeath73p::Lightning: return (s32)idMeansOfDeath68::Lightning;
		case idMeansOfDeath73p::BFG: return (s32)idMeansOfDeath68::BFG;
		case idMeansOfDeath73p::BFGSplash: return (s32)idMeansOfDeath68::BFGSplash;
		case idMeansOfDeath73p::Water: return (s32)idMeansOfDeath68::Water;
		case idMeansOfDeath73p::Slime: return (s32)idMeansOfDeath68::Slime;
		case idMeansOfDeath73p::Lava: return (s32)idMeansOfDeath68::Lava;
		case idMeansOfDeath73p::Crush: return (s32)idMeansOfDeath68::Crush;
		case idMeansOfDeath73p::TeleFrag: return (s32)idMeansOfDeath68::TeleFrag;
		case idMeansOfDeath73p::Fall: return (s32)idMeansOfDeath68::Fall;
		case idMeansOfDeath73p::Suicide: return (s32)idMeansOfDeath68::Suicide;
		case idMeansOfDeath73p::TargetLaser: return (s32)idMeansOfDeath68::TargetLaser;
		case idMeansOfDeath73p::HurtTrigger: return (s32)idMeansOfDeath68::HurtTrigger;
		case idMeansOfDeath73p::Grapple: return (s32)idMeansOfDeath68::Grapple;
		case idMeansOfDeath73p::Unknown:
		case idMeansOfDeath73p::NailGun:
		case idMeansOfDeath73p::ChainGun:
		case idMeansOfDeath73p::ProximityMine:
		case idMeansOfDeath73p::Kamikaze:
		case idMeansOfDeath73p::Juiced:
		case idMeansOfDeath73p::TeamSwitch:
		case idMeansOfDeath73p::Thaw:
		case idMeansOfDeath73p::UnknownQlMod1:
		case idMeansOfDeath73p::HeavyMachineGun:
		default:
			return (s32)idMeansOfDeath68::Unknown;
	}
}


udtProtocolConverterIdentity::udtProtocolConverterIdentity()
{
	_protocolSizeOfClientSnapshot = 0;
	_protocolSizeOfEntityState = 0;
}

void udtProtocolConverterIdentity::SetProtocol(udtProtocol::Id protocol)
{
	_protocolSizeOfClientSnapshot = udtGetSizeOfidClientSnapshot(protocol);
	_protocolSizeOfEntityState = udtGetSizeOfIdEntityState(protocol);
}

void udtProtocolConverterIdentity::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	memcpy(&outSnapshot, &inSnapshot, (size_t)_protocolSizeOfClientSnapshot);
}

void udtProtocolConverterIdentity::ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	memcpy(&outEntityState, &inEntityState, (size_t)_protocolSizeOfEntityState);
}

void udtProtocolConverterIdentity::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator&, s32 inIndex, const char* configString, u32 configStringLength)
{
	result.NewString = false;
	result.Index = inIndex;
	result.String = configString;
	result.StringLength = configStringLength;
}

void udtProtocolConverter73to90::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	*GetPlayerState(&outSnapshot, udtProtocol::Dm90) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm73);
	idPlayerState90& out = *(idPlayerState90*)GetPlayerState(&outSnapshot, udtProtocol::Dm90);
	out.doubleJumped = 0;
	out.jumpTime = 0;
}

void udtProtocolConverter73to90::ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
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

void udtProtocolConverter73to90::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength)
{
	result.NewString = false;
	result.Index = inIndex;
	result.String = configString;
	result.StringLength = configStringLength;

	if(inIndex == CS_SERVERINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue73to90, NULL);
		result.NewString = true;
		result.String = newString.String;
		result.StringLength = newString.Length;
	}
}

void udtProtocolConverter90to68_CPMA::StartGameState()
{
	_snapshotIndex = 0;
	for(u32 j = 0; j < 2; ++j)
	{
		SnapshotInfo& snapshot = _snapshots[j];
		snapshot.SnapshotTimeMs = S32_MIN;
		for(u32 i = 0; i < MAX_CLIENTS; ++i)
		{
			ShaftingPlayer& player = snapshot.Players[i];
			player.FirstCellTimeMs = S32_MIN;
			player.FirstSoundTimeMs = S32_MIN;
		}
	}
}

void udtProtocolConverter90to68_CPMA::StartSnapshot(s32 serverTimeMs)
{
	if(serverTimeMs == _snapshots[_snapshotIndex].SnapshotTimeMs)
	{
		return;
	}

	_snapshotIndex ^= 1;
	SnapshotInfo& snapshot = _snapshots[_snapshotIndex];
	snapshot.SnapshotTimeMs = serverTimeMs;
	for(u32 i = 0; i < MAX_CLIENTS; ++i)
	{
		snapshot.Players[i].SnapshotSoundCounter = 0;
	}
}

void udtProtocolConverter90to68_CPMA::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	idClientSnapshot68& out = (idClientSnapshot68&)outSnapshot;
	idClientSnapshot90& in = (idClientSnapshot90&)inSnapshot;
	(idPlayerStateBase&)out.ps = (idPlayerStateBase&)in.ps;

	Float3::Increment(out.ps.origin, Offsets);
	Float3::Increment(out.ps.grapplePoint, Offsets);

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

void udtProtocolConverter90to68_CPMA::ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	memcpy(&outEntityState, &inEntityState, sizeof(idEntityState68));
	outEntityState.weapon = ConvertWeapon90to68(inEntityState.weapon);
	outEntityState.event = ConvertEntityEventNumber90to68(inEntityState.event);
	outEntityState.eType = ConvertEntityType90to68(inEntityState.eType);

	Float3::Increment(outEntityState.pos.trBase, Offsets);
	Float3::Increment(outEntityState.origin, Offsets);
	Float3::Increment(outEntityState.origin2, Offsets);

	// The type can encode an event, so make sure we convert that too.
	if(inEntityState.eType >= ET_EVENTS)
	{
		outEntityState.eType = ET_EVENTS + ConvertEntityEventNumber90to68(inEntityState.eType - ET_EVENTS);
	}

	if(inEntityState.eType == ET_ITEM)
	{
		outEntityState.modelindex = ConvertEntityModelIndex90to68_CPMA(inEntityState.modelindex);
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

	//
	// The LG fire sound (the thunder-like sound) is repeating in q3mme so we tweak the demo.
	//

	if(inEntityState.eType == ET_PLAYER && 
	   inEntityState.clientNum >= 0 && inEntityState.clientNum < MAX_CLIENTS &&
	   (inEntityState.eFlags & EF_FIRING) != 0)
	{
		// Find the time at which we shoot the first LG cell (in a sequence) for the given player.
		SnapshotInfo& players = _snapshots[_snapshotIndex];
		const SnapshotInfo& oldPlayers = _snapshots[_snapshotIndex ^ 1];
		ShaftingPlayer& player = players.Players[inEntityState.clientNum];
		const ShaftingPlayer& oldPlayer = oldPlayers.Players[inEntityState.clientNum];
		player.FirstCellTimeMs = (oldPlayer.FirstCellTimeMs == S32_MIN) ? players.SnapshotTimeMs : oldPlayer.FirstCellTimeMs;
	}

	if(inEntityState.eType >= ET_EVENTS)
	{
		const s32 eventId = (inEntityState.eType - ET_EVENTS) & (~EV_EVENT_BITS);
		if(eventId == EV_FIRE_WEAPON_73p &&
		   inEntityState.clientNum >= 0 && inEntityState.clientNum < MAX_CLIENTS &&
		   inEntityState.weapon == idWeapon73p::LightningGun)
		{
			// Find out if this LG sequence already had the sound being played.
			// If true, we suppress this event.
			SnapshotInfo& players = _snapshots[_snapshotIndex];
			const SnapshotInfo& oldPlayers = _snapshots[_snapshotIndex ^ 1];
			ShaftingPlayer& player = players.Players[inEntityState.clientNum];
			const ShaftingPlayer& oldPlayer = oldPlayers.Players[inEntityState.clientNum];
			++player.SnapshotSoundCounter;
			player.FirstSoundTimeMs = (oldPlayer.FirstSoundTimeMs == S32_MIN) ? players.SnapshotTimeMs : oldPlayer.FirstSoundTimeMs;
			if((player.FirstCellTimeMs != S32_MIN && player.FirstSoundTimeMs < players.SnapshotTimeMs) || 
			   player.SnapshotSoundCounter >= 2 ||
			   player.FirstSoundTimeMs > player.FirstCellTimeMs) // Seems that q3mme likes to repeat it even when getting once in this case.
			{
				outEntityState.eType = ET_EVENTS + EV_NONE;
				outEntityState.event = EV_NONE;
			}
		}

		if(eventId == EV_OBITUARY_73p)
		{
			outEntityState.eventParm = ConvertMeanOfDeath90to68(inEntityState.eventParm);
		}
	}
}

void udtProtocolConverter90to68_CPMA::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex90to68(inIndex);
	result.String = configString;
	result.StringLength = configStringLength;

	if(inIndex == CS_GAME_VERSION_73p)
	{
		result.NewString = true;
		result.String = "cpma-1";
		result.StringLength = (u32)strlen("cpma-1");
	}

	if(inIndex == CS_SERVERINFO || inIndex == CS_SYSTEMINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue90to68, this);
		result.NewString = true;
		result.String = newString.String;
		result.StringLength = newString.Length;
	}
}

static s32 ConvertConfigStringIndex48to68(s32 index, s32 protocolNumber)
{
	if(protocolNumber >= 48)
	{
		return index;
	}

	// It seems that the only thing that dm_48 with real protocol 43 has in common with dm3
	// is that MAX_CLIENTS is 128 and there's nothing above or at index 736+.
	if(index >= CS_LOCATIONS_3 && index < CS_LOCATIONS_3 + MAX_LOCATIONS)
	{
		return index - 64;
	}

	return index;
}

static bool ConvertConfigStringValue3or48to68(udtString& newValue, udtVMLinearAllocator&, const udtString& key, const udtString& value, void*)
{
	newValue = udtString::Equals(key, "protocol") ? udtString::NewConstRef("68") : value;

	return true;
}

void udtProtocolConverter48to68::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	*GetPlayerState(&outSnapshot, udtProtocol::Dm68) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm48);
}

void udtProtocolConverter48to68::ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	(idEntityStateBase&)outEntityState = inEntityState;
}

void udtProtocolConverter48to68::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex48to68(inIndex, _protocolNumber);
	result.String = configString;
	result.StringLength = configStringLength;
	
	if(inIndex == CS_SERVERINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue3or48to68, NULL);
		result.NewString = true;
		result.String = newString.String;
		result.StringLength = newString.Length;

		s32 protocol;
		if(ParseConfigStringValueInt(protocol, allocator, "protocol", configString))
		{
			_protocolNumber = protocol;
		}
	}
}

static s32 ConvertConfigStringIndex3to68(s32 index)
{
	if(index <= 11 || (index >= 27 && index < CS_LOCATIONS_3 - 64))
	{
		return index;
	}

	if(index >= 12 && index <= 15)
	{
		return index + 8;
	}

	// dm3 had MAX_CLIENTS set as 128!
	if(index >= CS_LOCATIONS_3 && index < CS_LOCATIONS_3 + MAX_LOCATIONS)
	{
		return index - 64;
	}

	return -1;
}

static s32 ConvertPersistIndex3to68(s32 index)
{
	if(index <= 4 ||
	   (index >= 8 && index <= 10))
	{
		return index;
	}

	if(index == 7)
	{
		return 6;
	}

	if(index == 11)
	{
		return 13;
	}

	return -1;
}

namespace udt_private
{
	static s32 ConvertEntityEventNumber3to68_helper(s32 index)
	{
		if(index <= 46)
		{
			return index;
		}

		if(index >= 47 && index <= 50)
		{
			return index + 1;
		}

		if(index >= 51 && index <= 62)
		{
			return index + 2;
		}

		if(index == 63)
		{
			return 74;
		}

		if(index == 64)
		{
			return 76;
		}

		return EV_NONE;
	}
}

static s32 ConvertEntityEventNumber3to68(s32 eventId)
{
	const s32 eventSequenceBits = eventId & EV_EVENT_BITS;
	const s32 newEventId = udt_private::ConvertEntityEventNumber3to68_helper(eventId & (~EV_EVENT_BITS));

	return newEventId | eventSequenceBits;
}

static s32 ConvertEntityType3to68(s32 index)
{
	if(index >= 0 && index <= 11)
	{
		return index;
	}

	if(index >= ET_EVENTS_3)
	{
		return ET_EVENTS + udt_private::ConvertEntityEventNumber3to68_helper(index - ET_EVENTS_3);
	}

	return ET_GENERAL;
}

void udtProtocolConverter3to68::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	*GetPlayerState(&outSnapshot, udtProtocol::Dm68) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm3);
	const idPlayerStateBase& psIn = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm3);
	idPlayerStateBase& psOut = *GetPlayerState((idClientSnapshotBase*)&outSnapshot, udtProtocol::Dm68);
	psOut = psIn;

	for(s32 i = 0; i < MAX_PERSISTANT; ++i)
	{
		const s32 newIndex = ConvertPersistIndex3to68(i);
		psOut.persistant[i] = newIndex >= 0 ? psIn.persistant[newIndex] : 0;
	}

	psOut.events[0] = ConvertEntityEventNumber3to68(psIn.events[0]);
	psOut.events[1] = ConvertEntityEventNumber3to68(psIn.events[1]);
	psOut.externalEvent = ConvertEntityEventNumber3to68(psIn.externalEvent);
}

void udtProtocolConverter3to68::ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	(idEntityStateBase&)outEntityState = inEntityState;
	outEntityState.event = ConvertEntityEventNumber3to68(inEntityState.event);
	outEntityState.eType = ConvertEntityType3to68(inEntityState.eType);
}

void udtProtocolConverter3to68::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex3to68(inIndex);
	result.String = configString;
	result.StringLength = configStringLength;

	if(inIndex == CS_SERVERINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue3or48to68, NULL);
		result.NewString = true;
		result.String = newString.String;
		result.StringLength = newString.Length;
	}
}
