#pragma once


#include "parser.hpp"
#include "linear_allocator.hpp"
#include "array.hpp"
#include "string.hpp"


// On Windows, MAX_PATH is 260.
#define UDT_MAX_PATH_LENGTH    320

#define UDT_MIN_PROGRESS_TIME_MS    100


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


struct CallbackCutDemoFileStreamCreationInfo
{
	const char* OutputFolderPath;
};

extern udtStream*  CallbackCutDemoFileStreamCreation(s32 startTimeMs, s32 endTimeMs, const char* veryShortDesc, udtBaseParser* parser, void* userData);
extern udtStream*  CallbackConvertedDemoFileStreamCreation(s32 startTimeMs, s32 endTimeMs, const char* veryShortDesc, udtBaseParser* parser, void* userData);
extern void        CallbackConsoleMessage(s32 logLevel, const char* message);
extern void	       CallbackConsoleProgress(f32 progress, void* userData);
extern const char* GetFolderSeparator();
extern u32         GetFolderSeparatorLength();
extern bool        StringParseInt(s32& output, const char* string);
extern bool        StringContains_NoCase(const char* string, const char* pattern, u32* charIndex = NULL);
extern bool        StringStartsWith_NoCase(const char* string, const char* pattern);
extern bool        StringEndsWith_NoCase(const char* string, const char* pattern);
extern bool        StringEquals_NoCase(const char* a, const char* b);
extern bool        StringContains(const char* string, const char* pattern, u32* charIndex = NULL);
extern bool        StringStartsWith(const char* string, const char* pattern);
extern bool        StringEndsWith(const char* string, const char* pattern);
extern bool        StringEquals(const char* a, const char* b);
extern bool        StringFindFirstCharacterInList(u32& index, const char* string, const char* charList);
extern bool        StringFindLastCharacterInList(u32& index, const char* string, const char* charList);
extern bool        StringPathCombine(char*& combinedPath, udtVMLinearAllocator& allocator, const char* folderPath, const char* extra);
extern bool        StringHasTrailingFolderSeparator(const char* folderPath);
extern bool        StringHasValidDemoFileExtension(const char* filePath);
extern bool        StringConcatenate(char*& output, udtVMLinearAllocator& allocator, const char** strings, u32 stringCount);
extern bool        StringConcatenate(char*& output, udtVMLinearAllocator& allocator, const char* a, const char* b);
extern bool        StringConcatenate(char*& output, udtVMLinearAllocator& allocator, const char* a, const char* b, const char* c);
extern bool        StringSplitLines(udtVMArray<const char*>& lines, char* inOutText);
extern bool        StringIsNullOrEmpty(const char* string);
extern bool        GetFileName(char*& fileName, udtVMLinearAllocator& allocator, const char* filePath);
extern bool        GetFileNameWithoutExtension(char*& fileNameNoExt, udtVMLinearAllocator& allocator, const char* filePath);
extern bool        GetFolderPath(char*& folderPath, udtVMLinearAllocator& allocator, const char* filePath); // Doesn't leave a trailing separator.
extern bool        GetFileExtension(char*& fileExtension, udtVMLinearAllocator& allocator, const char* filePath);
extern bool        FormatTimeForFileName(char*& formattedTime, udtVMLinearAllocator& allocator, s32 timeMs); // Format is "mmss".
extern bool        FormatBytes(char*& formattedSize, udtVMLinearAllocator& allocator, u64 byteCount); // Will use the most appropriate unit.
extern bool        StringParseSeconds(s32& duration, const char* buffer); // Format is minutes:seconds or seconds.
extern bool        CopyFileRange(udtStream& input, udtStream& output, udtVMLinearAllocator& allocator, u32 startOffset, u32 endOffset);
extern s32         GetErrorCode(bool success, s32* cancel);
extern bool        RunParser(udtBaseParser& parser, udtStream& file, const s32* cancelOperation);
extern char*       AllocateString(udtVMLinearAllocator& allocator, const char* string, u32 stringLength = 0);
extern char*       AllocateSpaceForString(udtVMLinearAllocator& allocator, u32 stringLength);
extern s32         ConvertPowerUpFlagsToValue(s32 flags);
extern const char* GetTeamName(s32 team);
extern s32         GetUDTPlayerMODBitFromIdMod(s32 idMod, udtProtocol::Id protocol);
extern s32         GetUDTWeaponFromIdWeapon(s32 idWeapon, udtProtocol::Id protocol);
extern s32         GetUDTWeaponFromIdMod(s32 idMod, udtProtocol::Id protocol);
extern void        LogLinearAllocatorStats(u32 threadCount, u32 fileCount, udtContext& context, udtVMLinearAllocator& allocator, const udtVMLinearAllocator::Stats& stats);

extern bool        ParseConfigStringValueInt(s32& varValue, const char* varName, const char* configString); // Gets the integer value of a config string variable.
extern bool        ParseConfigStringValueString(udtString& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString); // Gets a config string variable.
extern bool        StringMatchesCutByChatRule(const udtString& string, const udtCutByChatRule& rule, udtVMLinearAllocator& allocator);
