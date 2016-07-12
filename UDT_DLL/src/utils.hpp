#pragma once


#include "macros.hpp"
#include "parser.hpp"
#include "linear_allocator.hpp"
#include "array.hpp"
#include "string.hpp"
#include "look_up_tables.hpp"


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

bool UDT_INLINE IsBitSet(const void* bits, u32 index)
{
	const u32 byteIndex = index >> 3;
	const u32 bitIndex = index & 7;
	return (((const u8*)bits)[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0;
}

void UDT_INLINE SetBit(void* bits, u32 index)
{
	const u32 byteIndex = index >> 3;
	const u32 bitIndex = index & 7;
	((u8*)bits)[byteIndex] |= (u8)1 << (u8)bitIndex;
}

void UDT_INLINE ClearBit(void* bits, u32 index)
{
	const u32 byteIndex = index >> 3;
	const u32 bitIndex = index & 7;
	((u8*)bits)[byteIndex] &= ~((u8)1 << (u8)bitIndex);
}


struct udtObituaryEvent
{
	s32 AttackerIndex; // A player or -1 if the world.
	s32 TargetIndex; // Always a player.
	u32 MeanOfDeath; // Of type udtMeanOfDeath::Id.
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
extern void        LogLinearAllocatorDebugStats(udtContext& context, udtVMLinearAllocator& allocator);
extern bool        StringMatchesCutByChatRule(const udtString& string, const udtChatPatternRule& rule, udtVMLinearAllocator& allocator, udtProtocol::Id procotol);
extern bool        IsObituaryEvent(udtObituaryEvent& info, const idEntityStateBase& entity, udtProtocol::Id protocol);
extern const char* GetUDTModName(s32 mod); // Where mod is of type udtMeanOfDeath::Id. Never returns a NULL pointer.
extern bool        GetClanAndPlayerName(udtString& clan, udtString& player, bool& hasClan, udtVMLinearAllocator& allocator, udtProtocol::Id protocol, const char* configString);
extern bool        IsTeamMode(udtGameType::Id gameType);
extern bool        IsRoundBasedMode(udtGameType::Id gameType);
extern void        PerfStatsInit(u64* perfStats);
extern void        PerfStatsAddCurrentThread(u64* perfStats, u64 totalDemoByteCount);
extern void        PerfStatsFinalize(u64* perfStats, u32 threadCount, u64 durationMs);
extern void        WriteStringToApiStruct(u32& offset, const udtString& string);
extern void        WriteNullStringToApiStruct(u32& offset);
extern void        PlayerStateToEntityState(idEntityStateBase& es, s32& lastEventSequence, const idPlayerStateBase& ps, bool extrapolate, s32 serverTimeMs, udtProtocol::Id protocol);

// Gets the integer value of a config string variable.
// The variable name matching is case sensitive.
extern bool ParseConfigStringValueInt(s32& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString);

// Gets the string value of a config string variable.
// The variable name matching is case sensitive.
extern bool ParseConfigStringValueString(udtString& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString);
