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
- Stats
- Game types
- PMOVE types
- Animation number
- Means of death
- Item types (only difference is the key was added)
- Entity state types

Different:
- PMOVE flags: scoreboard was dropped, waterjump moved
- Persistent stats: 14 was attackee armor but is now frags, 15 is new: XP points
- Entity state flags: 0x4000 became "global spectator" instead of "already voted"
- Power-up indices: very different
- Config strings: pretty different after CS_SHADERSTATE_73p
*/


static s32 ConvertConfigStringIndex73or90to91(s32 index)
{
	if(index <= 665 ||
	   (index >= 669 && index <= 672))
	{
		return index;
	}

	switch(index)
	{
		case 679: return (s32)678;
		case 680: return (s32)679;
		case 682: return (s32)681;
		case 683: return (s32)682;
		case 684: return (s32)683;
		case 685: return (s32)684;
		case 686: return (s32)687;
		case 687: return (s32)688;
		case 688: return (s32)685;
		case 689: return (s32)686;
		case 691: return (s32)692;
		case 692: return (s32)693;
		case 697: return (s32)696;
		case 693: return (s32)694;
		case 694: return (s32)695;
		case 695: return (s32)694;
		case 696: return (s32)695;
		case 699: return (s32)697;
		case 700: return (s32)698;
		case 701: return (s32)699;
		case 702: return (s32)700;
		case 703: return (s32)701;
		case 705: return (s32)703;
		case 706: return (s32)704;
		case 707: return (s32)705;
		case 708: return (s32)706;
		case 709: return (s32)707;
		case 710: return (s32)708;
		case 713: return (s32)710;
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

static void ConvertPowerUps73or90to91(idPlayerStateBase& out, const idPlayerStateBase& in)
{
	for(s32 i = 0; i < ID_MAX_PS_POWERUPS; ++i)
	{
		out.powerups[i] = 0;
	}

	for(s32 i = 0; i < ID_MAX_PS_POWERUPS; ++i)
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
	for(s32 i = 0; i < ID_MAX_PS_POWERUPS; ++i)
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

static s32 ConvertPersistStatIndex73or90to91(s32 index)
{
	if(index >= 0 && index <= 13)
	{
		return index;
	}

	return -1;
}

static void ConvertPersistStat73or90to91(idPlayerStateBase& out, const idPlayerStateBase& in)
{
	for(s32 i = 0; i < ID_MAX_PS_PERSISTANT; ++i)
	{
		out.persistant[i] = 0;
	}

	for(s32 i = 0; i < ID_MAX_PS_PERSISTANT; ++i)
	{
		const s32 p = ConvertPersistStatIndex73or90to91(i);
		if(p >= 0)
		{
			out.persistant[p] = in.persistant[i];
		}
	}
}

static s32 ConvertPMoveIndex73or90to91(s32 index)
{
	// PMF_TIME_WATERJUMP moved.
	if(index == 8)
	{
		return 7;
	}

	// PMF_SCOREBOARD no longer exists.
	if(index == 13)
	{
		return -1;
	}

	return index;
}

static s32 ConvertPMoveFlags73or90to91(s32 oldFlags)
{
	s32 flags = 0;
	for(s32 i = 0; i < 32; ++i)
	{
		if((oldFlags & (1 << i)) == 0)
		{
			continue;
		}

		const s32 f = ConvertPMoveIndex73or90to91(i);
		if(f < 0)
		{
			continue;
		}

		flags |= (1 << f);
	}

	return flags;
}

static s32 ConvertEntityStateFlags73or90to91(s32 oldFlags)
{
	// EF_VOTED (0x4000) was dropped and EF_GLOBAL_SPECTATOR (0x4000) replaces it.
	return oldFlags & (~s32(0x4000));
}


// Return false to drop the key/value pair altogether.
typedef bool (*ProcessConfigStringCallback)(udtString& newValue, udtVMLinearAllocator& allocator, const udtString& key, const udtString& value, void* userData);

static void ProcessConfigString(udtString& result, udtVMLinearAllocator& allocator, const udtString& input, ProcessConfigStringCallback callback, void* userData)
{
	const udtString separator = udtString::NewConstRef("\\");
	result = udtString::NewEmpty(allocator, BIG_INFO_STRING);
	const char* key = input.GetPtr() + 1;
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
			sepBeforeNextKey = input.GetPtr() + input.GetLength();
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
	result.String = udtString::NewConstRef(configString, configStringLength);
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
	idPlayerStateBase& outBasePs = *GetPlayerState(&outSnapshot, udtProtocol::Dm91);
	const idPlayerStateBase& inBasePs = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm90);
	ConvertPowerUps73or90to91(outBasePs, inBasePs);
	ConvertPersistStat73or90to91(outBasePs, inBasePs);
	out.pm_flags = ConvertPMoveFlags73or90to91(inBasePs.pm_flags);
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
	out.eFlags = ConvertEntityStateFlags73or90to91(inEntityState.eFlags);
}

void udtProtocolConverter90to91::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex73or90to91(inIndex);
	result.String = udtString::NewConstRef(configString, configStringLength);

	if(inIndex == CS_SERVERINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue73or90to91, NULL);
		result.NewString = true;
		result.String = newString;
	}
}

void udtProtocolConverter73to91::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	*GetPlayerState(&outSnapshot, udtProtocol::Dm91) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm73);
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
	idPlayerStateBase& outBasePs = *GetPlayerState(&outSnapshot, udtProtocol::Dm91);
	const idPlayerStateBase& inBasePs = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm73);
	ConvertPowerUps73or90to91(outBasePs, inBasePs);
	ConvertPersistStat73or90to91(outBasePs, inBasePs);
	out.pm_flags = ConvertPMoveFlags73or90to91(inBasePs.pm_flags);
}

void udtProtocolConverter73to91::ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState)
{
	idEntityState91& out = (idEntityState91&)outEntityState;
	idEntityState73& in = (idEntityState73&)inEntityState;
	(idEntityStateBase&)outEntityState = inEntityState;
	out.pos_gravity = in.pos_gravity;
	out.apos_gravity = in.apos_gravity;
	out.jumpTime = 0;
	out.doubleJumped = 0;
	out.health = 0;
	out.armor = 0;
	out.location = 0;
	out.powerups = ConvertPowerUpFlags73or90to91(inEntityState.powerups);
	out.eFlags = ConvertEntityStateFlags73or90to91(inEntityState.eFlags);
}

void udtProtocolConverter73to91::ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength)
{
	result.NewString = false;
	result.Index = ConvertConfigStringIndex73or90to91(inIndex);
	result.String = udtString::NewConstRef(configString, configStringLength);

	if(inIndex == CS_SERVERINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue73or90to91, NULL);
		result.NewString = true;
		result.String = newString;
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
	if(index >= 672 && index < 672 + MAX_LOCATIONS)
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
	result.String = udtString::NewConstRef(configString, configStringLength);
	
	if(inIndex == CS_SERVERINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue3or48to68, NULL);
		result.NewString = true;
		result.String = newString;

		s32 protocol;
		if(ParseConfigStringValueInt(protocol, allocator, "protocol", configString))
		{
			_protocolNumber = protocol;
		}
	}
}

static s32 ConvertConfigStringIndex3to68(s32 index)
{
	if(index <= 11 || (index >= 27 && index < 672 - 64))
	{
		return index;
	}

	if(index >= 12 && index <= 15)
	{
		return index + 8;
	}

	// dm3 had MAX_CLIENTS set as 128!
	if(index >= 672 && index < 672 + MAX_LOCATIONS)
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
	const s32 eventSequenceBits = eventId & ID_ES_EVENT_BITS;
	const s32 newEventId = udt_private::ConvertEntityEventNumber3to68_helper(eventId & (~ID_ES_EVENT_BITS));

	return newEventId | eventSequenceBits;
}

static s32 ConvertEntityType3to68(s32 index)
{
	if(index >= 0 && index <= 11)
	{
		return index;
	}

	if(index >= 12)
	{
		return 13 + udt_private::ConvertEntityEventNumber3to68_helper(index - 12);
	}

	return 0;
}

void udtProtocolConverter3to68::ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot)
{
	(idClientSnapshotBase&)outSnapshot = inSnapshot;
	*GetPlayerState(&outSnapshot, udtProtocol::Dm68) = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm3);
	const idPlayerStateBase& psIn = *GetPlayerState((idClientSnapshotBase*)&inSnapshot, udtProtocol::Dm3);
	idPlayerStateBase& psOut = *GetPlayerState((idClientSnapshotBase*)&outSnapshot, udtProtocol::Dm68);
	psOut = psIn;

	for(s32 i = 0; i < ID_MAX_PS_PERSISTANT; ++i)
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
	result.String = udtString::NewConstRef(configString, configStringLength);

	if(inIndex == CS_SERVERINFO)
	{
		udtString newString;
		ProcessConfigString(newString, allocator, udtString::NewConstRef(configString, configStringLength), &ConvertConfigStringValue3or48to68, NULL);
		result.NewString = true;
		result.String = newString;
	}
}
