#pragma once


#include "parser.hpp"
#include "linear_allocator.hpp"
#include "array.hpp"
#include "string.hpp"


// On Windows, MAX_PATH is 260.
#define UDT_MAX_PATH_LENGTH    320


template<typename T>
T udt_min(const T a, const T b)
{
	return a < b ? a : b;
}

template<typename T>
T udt_max(const T a, const T b)
{
	return a > b ? a : b;
}

template<typename T>
T udt_clamp(const T x, const T a, const T b)
{
	return udt_min(udt_max(x, a), b);
}


struct udtObituaryEvent
{
	s32 AttackerIndex; // A player or -1 if the world.
	s32 TargetIndex; // Always a player.
	s32 MeanOfDeath; // Of type udtMeanOfDeath::Id.
};

struct CallbackCutDemoFileStreamCreationInfo
{
	const char* OutputFolderPath;
};

extern udtString   CallbackCutDemoFileNameCreation(const udtDemoStreamCreatorArg& arg);
extern udtString   CallbackConvertedDemoFileNameCreation(const udtDemoStreamCreatorArg& arg);
extern bool        StringParseInt(s32& output, const char* string);
extern bool        StringSplitLines(udtVMArray<udtString>& lines, udtString& inOutText);
extern udtString   FormatTimeForFileName(udtVMLinearAllocator& allocator, s32 timeMs); // Format is "mmss".
extern udtString   FormatBytes(udtVMLinearAllocator& allocator, u64 byteCount); // Will use the most appropriate unit.
extern bool        StringParseSeconds(s32& duration, const char* buffer); // Format is minutes:seconds or seconds.
extern bool        CopyFileRange(udtStream& input, udtStream& output, udtVMLinearAllocator& allocator, u32 startOffset, u32 endOffset);
extern s32         GetErrorCode(bool success, const s32* cancel);
extern bool        RunParser(udtBaseParser& parser, udtStream& file, const s32* cancelOperation);
extern s32         ConvertPowerUpFlagsToValue(s32 flags);
extern const char* GetTeamName(s32 team);
extern s32         GetUDTPlayerMODBitFromIdMod(s32 idMod, udtProtocol::Id protocol);
extern s32         GetUDTPlayerMODBitFromUDTMod(s32 udtMod); // Returns -1 if unknown or invalid.
extern s32         GetUDTWeaponFromIdWeapon(s32 idWeapon, udtProtocol::Id protocol);
extern s32         GetUDTWeaponFromIdMod(s32 idMod, udtProtocol::Id protocol);
extern s32         GetUDTGameTypeFromIdGameType(s32 gt, udtProtocol::Id protocol, udtGame::Id game);
extern void        LogLinearAllocatorDebugStats(udtContext& context, udtVMLinearAllocator& allocator);
extern bool        StringMatchesCutByChatRule(const udtString& string, const udtCutByChatRule& rule, udtVMLinearAllocator& allocator, udtProtocol::Id procotol);
extern bool        IsObituaryEvent(udtObituaryEvent& info, const idEntityStateBase& entity, udtProtocol::Id protocol);
extern s32         GetUDTModFromIdMod(s32 idMod, udtProtocol::Id protocol); // Returns -1 if unknown or invalid.
extern const char* GetUDTModName(s32 mod); // Where mod is of type udtMeanOfDeath::Id. Never returns a NULL pointer.
extern bool        GetClanAndPlayerName(udtString& clan, udtString& player, bool& hasClan, udtVMLinearAllocator& allocator, udtProtocol::Id protocol, const char* configString);
extern uptr        ComputeReservedByteCount(uptr smallByteCount, uptr bigByteCount, u32 demoCountThreshold, u32 demoCount);
extern bool        IsTeamMode(udtGameType::Id gameType);
extern bool        IsRoundBasedMode(udtGameType::Id gameType);
extern void        PerfStatsInit(u64* perfStats);
extern void        PerfStatsAddCurrentThread(u64* perfStats, u64 totalDemoByteCount);
extern void        PerfStatsFinalize(u64* perfStats, u32 threadCount, u64 durationMs);
extern void        WriteStringToApiStruct(u32& offset, const udtString& string);
extern void        WriteNullStringToApiStruct(u32& offset);

// Gets the integer value of a config string variable.
// The variable name matching is case sensitive.
extern bool ParseConfigStringValueInt(s32& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString);

// Gets the string value of a config string variable.
// The variable name matching is case sensitive.
extern bool ParseConfigStringValueString(udtString& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString);

namespace idEntityEvent
{
	extern s32 Obituary(udtProtocol::Id protocol);
	extern s32 WeaponFired(udtProtocol::Id protocol);
	extern s32 ItemPickup(udtProtocol::Id protocol);
	extern s32 GlobalItemPickup(udtProtocol::Id protocol);
	extern s32 GlobalSound(udtProtocol::Id protocol);
	extern s32 GlobalTeamSound(udtProtocol::Id protocol);
	extern s32 QL_Overtime(udtProtocol::Id protocol);
	extern s32 QL_GameOver(udtProtocol::Id protocol);
};

namespace idEntityType
{
	extern s32 General(udtProtocol::Id protocol);
	extern s32 Player(udtProtocol::Id protocol);
	extern s32 Item(udtProtocol::Id protocol);
	extern s32 Missile(udtProtocol::Id protocol);
	extern s32 Mover(udtProtocol::Id protocol);
	extern s32 Beam(udtProtocol::Id protocol);
	extern s32 Portal(udtProtocol::Id protocol);
	extern s32 Speaker(udtProtocol::Id protocol);
	extern s32 PushTrigger(udtProtocol::Id protocol);
	extern s32 TeleportTrigger(udtProtocol::Id protocol);
	extern s32 Invisible(udtProtocol::Id protocol);
	extern s32 Grapple(udtProtocol::Id protocol);
	extern s32 Team(udtProtocol::Id protocol);
	extern s32 Event(udtProtocol::Id protocol);
}

namespace idConfigStringIndex
{
	extern s32 FirstPlayer(udtProtocol::Id protocol);
	extern s32 Intermission(udtProtocol::Id protocol);
	extern s32 LevelStartTime(udtProtocol::Id protocol);
	extern s32 WarmUpEndTime(udtProtocol::Id protocol);
	extern s32 FirstPlacePlayerName(udtProtocol::Id protocol);
	extern s32 SecondPlacePlayerName(udtProtocol::Id protocol);
	extern s32 PauseStart(udtProtocol::Id protocol);
	extern s32 PauseEnd(udtProtocol::Id protocol);
	extern s32 FlagStatus(udtProtocol::Id protocol);
}

namespace idPowerUpIndex
{
	extern s32 RedFlag(udtProtocol::Id protocol);
	extern s32 BlueFlag(udtProtocol::Id protocol);
	extern s32 NeutralFlag(udtProtocol::Id protocol);
	extern s32 QuadDamage(udtProtocol::Id protocol);
	extern s32 BattleSuit(udtProtocol::Id protocol);
	extern s32 Haste(udtProtocol::Id protocol);
	extern s32 Invisibility(udtProtocol::Id protocol);
	extern s32 Regeneration(udtProtocol::Id protocol);
	extern s32 Flight(udtProtocol::Id protocol);
	extern s32 Scout(udtProtocol::Id protocol);
	extern s32 Guard(udtProtocol::Id protocol);
	extern s32 Doubler(udtProtocol::Id protocol);
	extern s32 ArmorRegeneration(udtProtocol::Id protocol);
	extern s32 Invulnerability(udtProtocol::Id protocol);
}

namespace idPersStatsIndex
{
	extern s32 FlagCaptures(udtProtocol::Id protocol);
	extern s32 Score(udtProtocol::Id protocol);
	extern s32 Hits(udtProtocol::Id protocol);
	extern s32 Rank(udtProtocol::Id protocol);
	extern s32 Team(udtProtocol::Id protocol);
	extern s32 SpawnCount(udtProtocol::Id protocol);
	extern s32 Deaths(udtProtocol::Id protocol);
	extern s32 LastAttacker(udtProtocol::Id protocol);
	extern s32 DamageGiven(udtProtocol::Id protocol);
	extern s32 LastTargetHealthAndArmor(udtProtocol::Id protocol);
	extern s32 Impressives(udtProtocol::Id protocol);
	extern s32 Excellents(udtProtocol::Id protocol);
	extern s32 Defends(udtProtocol::Id protocol);
	extern s32 Assists(udtProtocol::Id protocol);
	extern s32 Humiliations(udtProtocol::Id protocol);
}

namespace idEntityStateFlag
{
	extern s32 Dead(udtProtocol::Id protocol);
	extern s32 TeleportBit(udtProtocol::Id protocol);
	extern s32 AwardExcellent(udtProtocol::Id protocol);
	extern s32 PlayerEvent(udtProtocol::Id protocol);
	extern s32 AwardHumiliation(udtProtocol::Id protocol);
	extern s32 NoDraw(udtProtocol::Id protocol);
	extern s32 Firing(udtProtocol::Id protocol);
	extern s32 AwardCapture(udtProtocol::Id protocol);
	extern s32 Chatting(udtProtocol::Id protocol);
	extern s32 ConnectionInterrupted(udtProtocol::Id protocol);
	extern s32 HasVoted(udtProtocol::Id protocol);
	extern s32 AwardImpressive(udtProtocol::Id protocol);
	extern s32 AwardDefense(udtProtocol::Id protocol);
	extern s32 AwardAssist(udtProtocol::Id protocol);
	extern s32 AwardDenied(udtProtocol::Id protocol);
	extern s32 HasTeamVoted(udtProtocol::Id protocol);
}
