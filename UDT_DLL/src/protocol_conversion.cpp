#include "protocol_conversion.hpp"
#include "scoped_stack_allocator.hpp"
#include "string.hpp"
#include "utils.hpp"


/*
73/90 -> 91

No changes:
- Global team sounds
- Entity events
- Weapons
- Player reward sounds
- Holdable items
- Persistent indices
- Game types
- PMOVE types
- Animation number
- Means of death
- Item types (only difference is the key was added)
- Entity state types

Different:
- PMOVE flags: @TODO: different?
- Stats indices: @TODO: 14 is either frags or attackee armour?
- Entity state flags: @TODO: 0x4000 became "global spectator" instead of "already voted"? if so, when?
- Power-up indices: very different
- Config strings: pretty different after CS_SHADERSTATE_73p
*/


static s32 ConvertConfigStringIndex73or90to91(s32 index)
{
	if(index <= CS_SHADERSTATE_73p ||
	   (index >= CS_TIMEOUT_BEGIN_TIME_73p && index <= CS_BLUE_TEAM_TIMEOUTS_LEFT_73p))
	{
		return index;
	}

	switch(index)
	{
		case CS_MAP_CREATOR_73p: return (s32)CS_AUTHOR_91;
		case CS_ORIGINAL_MAP_CREATOR_73p: return (s32)CS_AUTHOR2_91;
		case CS_PMOVE_SETTINGS_73p: return (s32)CS_PMOVEINFO_91;
		case CS_ARMOR_TIERED_73p: return (s32)CS_ARMORINFO_91;
		case CS_WEAPON_SETTINGS_73p: return (s32)CS_WEAPONINFO_91;
		case CS_CUSTOM_PLAYER_MODELS_73p: return (s32)CS_PLAYERINFO_91;
		case CS_FIRST_PLACE_CLIENT_NUM_73p: return (s32)CS_CLIENTNUM1STPLAYER_91;
		case CS_SECOND_PLACE_CLIENT_NUM_73p: return (s32)CS_CLIENTNUM2NDPLAYER_91;
		case CS_FIRST_PLACE_SCORE_73p: return (s32)CS_SCORE1STPLAYER_91;
		case CS_SECOND_PLACE_SCORE_73p: return (s32)CS_SCORE2NDPLAYER_91;
		case CS_MOST_DAMAGE_DEALT_73p: return (s32)CS_MOST_DAMAGEDEALT_PLYR_91;
		case CS_MOST_ACCURATE_73p: return (s32)CS_MOST_ACCURATE_PLYR_91;
		case CS_BEST_ITEM_CONTROL_73p: return (s32)CS_BEST_ITEMCONTROL_PLYR_91;
		case CS_RED_TEAM_CLAN_NAME_73p: return (s32)CS_REDTEAMBASE_91;
		case CS_BLUE_TEAM_CLAN_NAME_73p: return (s32)CS_BLUETEAMBASE_91;
		case CS_RED_TEAM_CLAN_TAG_73p: return (s32)CS_REDTEAMBASE_91;
		case CS_BLUE_TEAM_CLAN_TAG_73p: return (s32)CS_BLUETEAMBASE_91;
		case CS_MVP_OFFENSE_73p: return (s32)CS_MOST_VALUABLE_OFFENSIVE_PLYR_91;
		case CS_MVP_DEFENSE_73p: return (s32)CS_MOST_VALUABLE_DEFENSIVE_PLYR_91;
		case CS_MVP_73p: return (s32)CS_MOST_VALUABLE_PLYR_91;
		case CS_DOMINATION_RED_POINTS_73p: return (s32)CS_GENERIC_COUNT_RED_91;
		case CS_DOMINATION_BLUE_POINTS_73p: return (s32)CS_GENERIC_COUNT_BLUE_91;
		case CS_ROUND_WINNERS_73p: return (s32)CS_ROUND_WINNER_91;
		case CS_CUSTOM_SERVER_SETTINGS_73p: return (s32)CS_CUSTOM_SETTINGS_91;
		case CS_MAP_VOTE_INFO_73p: return (s32)CS_ROTATIONMAPS_91;
		case CS_MAP_VOTE_COUNT_73p: return (s32)CS_ROTATIONVOTES_91;
		case CS_DISABLE_MAP_VOTE_73p: return (s32)CS_DISABLE_VOTE_UI_91;
		case CS_READY_UP_TIME_73p: return (s32)CS_ALLREADY_TIME_91;
		case CS_NUMBER_OF_RACE_CHECKPOINTS_73p: return (s32)CS_RACE_POINTS_91;
		default: return -1;
	}
}

static s32 ConvertPowerUpIndex73or90to91(s32 index)
{
	// 0 is none, 10+ is invul, scout, guard, doubler, armor regen, frozen
	if(index == 0 || index >= 10)
	{
		return index;
	}

	// quad, bs, haste, invis, regen, flight
	if(index >= 1 && index <= 6)
	{
		return index + 3;
	}

	// red flag, blue flag, neutral flag
	if(index >= 7 && index <= 9)
	{
		return index - 6;
	}

	return -1;
}

static void ConvertPowerUps73or90to91(idPlayerStateBase& out, idPlayerStateBase& in)
{
	for(s32 i = 0; i < MAX_POWERUPS; ++i)
	{
		out.powerups[i] = 0;
	}

	for(s32 i = 0; i < MAX_POWERUPS; ++i)
	{
		const s32 p = ConvertPowerUpIndex73or90to91(i);
		if(p >= 0)
		{
			out.powerups[p] = in.powerups[i];
		}
	}
}

static s32 ConvertPowerUpFlags73or90to91(s32 oldFlags)
{
	s32 flags = 0;
	for(s32 i = 0; i < MAX_POWERUPS; ++i)
	{
		if((oldFlags & (1 << i)) == 0)
		{
			continue;
		}

		const s32 p = ConvertPowerUpIndex73or90to91(i);
		if(p < 0)
		{
			continue;
		}

		flags |= (1 << p);
	}

	return flags;
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

static bool ConvertConfigStringValue73or90to91(udtString& newValue, udtVMLinearAllocator&, const udtString& key, const udtString& value, void*)
{
	newValue = udtString::Equals(key, "protocol") ? udtString::NewConstRef("91") : value;

	return true;
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

void udtProtocolConverter90to91::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	*GetPlayerState(&outSnapshot, udtProtocol::Dm91) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm90);
	idPlayerState91& out = *(idPlayerState91*)GetPlayerState(&outSnapshot, udtProtocol::Dm91);
	out.doubleJumped = 0;
	out.jumpTime = 0;
	out.weaponPrimary = 0;
	out.crouchTime = 0;
	out.crouchSlideTime = 0;
	out.location = 0;
	out.fov = 0;
	out.forwardmove = 0;
	out.rightmove = 0;
	out.upmove = 0;
	ConvertPowerUps73or90to91(*GetPlayerState(&outSnapshot, udtProtocol::Dm91), *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm90));
}

void udtProtocolConverter90to91::ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	idEntityState91& out = (idEntityState91&)outEntityState;
	idEntityState90& in = (idEntityState90&)inEntityState;
	(idEntityStateBase&)outEntityState = inEntityState;
	out.pos_gravity = in.pos_gravity;
	out.apos_gravity = in.apos_gravity;
	out.jumpTime = in.jumpTime;
	out.doubleJumped = in.doubleJumped;
	out.health = 0;
	out.armor = 0;
	out.location = 0;
	out.powerups = ConvertPowerUpFlags73or90to91(inEntityState.powerups);
}

void udtProtocolConverter90to91::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex73or90to91(inIndex);
	result.String = configString;
	result.StringLength = configStringLength;

	if(inIndex == CS_SERVERINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue73or90to91, NULL);
		result.NewString = true;
		result.String = newString.String;
		result.StringLength = newString.Length;
	}
}

/*
void udtProtocolConverter73to91::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	*GetPlayerState(&outSnapshot, udtProtocol::Dm90) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm73);
	idPlayerState90& out = *(idPlayerState90*)GetPlayerState(&outSnapshot, udtProtocol::Dm90);
	out.doubleJumped = 0;
	out.jumpTime = 0;
}

void udtProtocolConverter73to91::ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
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

void udtProtocolConverter73to91::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength)
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
*/
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
