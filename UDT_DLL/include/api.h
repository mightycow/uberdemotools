#pragma once


#include "types.hpp"


#if defined(UDT_MSVC)
	#if defined(UDT_CREATE_DLL)
	#	define UDT_EXPORT_DLL __declspec(dllexport)
	#else
	#	define UDT_EXPORT_DLL __declspec(dllimport)
	#endif
#	define UDT_API_DECL(ReturnType) extern UDT_EXPORT_DLL ReturnType
#	define UDT_API_DEF(ReturnType)  UDT_EXPORT_DLL ReturnType
#elif defined(UDT_GCC)
	#if defined(UDT_CREATE_DLL)
	#	define UDT_EXPORT_DLL
	#else
	#	define UDT_EXPORT_DLL
	#endif
#	define UDT_API_DECL(ReturnType) extern ReturnType UDT_EXPORT_DLL
#	define UDT_API_DEF(ReturnType)  ReturnType UDT_EXPORT_DLL
#endif


#define UDT_API UDT_API_DECL


struct udtParserContext;

struct udtErrorCode
{
	enum Id
	{
		None,
		InvalidArgument,
		OperationFailed
	};
};

#define UDT_PROTOCOL_LIST(N) \
	N(Invalid, NULL) \
	N(Dm68, ".dm_68") \
	N(Dm73, ".dm_73") \
	N(Dm90, ".dm_90")

#define UDT_PROTOCOL_ITEM(Enum, Ext) Enum,
struct udtProtocol
{
	enum Id
	{
		UDT_PROTOCOL_LIST(UDT_PROTOCOL_ITEM)
		AfterLastProtocol,
		FirstProtocol = Dm68
	};
};
#undef UDT_PROTOCOL_ITEM

struct udtChatOperator
{
	enum Id
	{
		Contains,
		StartsWith,
		EndsWith,
		Count
	};
};

// Macro arguments:
// 1. enum name 
// 2. plug-in type (internal)
// 3. plug-in data type (for the user)
#define UDT_PLUG_IN_LIST(N) \
	N(Chat, udtParserPlugInChat, udtParseDataChat) \
	N(GameState, udtParserPlugInGameState, udtParseDataGameState)

#define UDT_PLUG_IN_ITEM(Enum, Type, ApiType) Enum,
struct udtParserPlugIn
{
	enum Id
	{
		UDT_PLUG_IN_LIST(UDT_PLUG_IN_ITEM)
		Count
	};
};
#undef UDT_PLUG_IN_ITEM


#ifdef __cplusplus
extern "C" 
{
#endif

	// Return 0 to continue, anything else to cancel.
	typedef s32(*udtProgressCallback)(f32 progress);

	// Log levels: 0=info, 1=warning, 2=error, 3=error+crash.
	typedef void (*udtMessageCallback)(s32 logLevel, const char* message);

	// Called when UDT can't recover from the error.
	// Implement to throw an exception, generate a stack trace, etc.
	// Default behavior: calls exit.
	typedef void (*udtCrashCallback)(const char* message);

#pragma pack(push, 1)
	struct udtCutByTimeArg
	{
		const char* FilePath;
		const char* OutputFolderPath; // May be NULL.
		udtMessageCallback MessageCb; // May be NULL.
		udtProgressCallback ProgressCb; // May be NULL.
		s32 StartTimeMs;
		s32 EndTimeMs;
	};

	struct udtCutByChatRule
	{
		const char* Pattern;
		u32 ChatOperator; // udtChatOperator::Id
		u32 CaseSensitive; // Non-zero means case-sensitive.
		u32 IgnoreColorCodes; // Non-zero means color codes are ignored.
	};

	struct udtCutByChatArg
	{
		const char* FilePath;
		const char* OutputFolderPath; // May be NULL.
		udtMessageCallback MessageCb; // May be NULL.
		udtProgressCallback ProgressCb; // May be NULL.
		const udtCutByChatRule* Rules;
		u32 RuleCount;
		u32 StartOffsetSec;
		u32 EndOffsetSec;
	};

	struct udtFileParseArg
	{
		const char* FilePath;
		udtMessageCallback MessageCb; // May be NULL.
		udtProgressCallback ProgressCb; // May be NULL.
	};

	struct udtChatEventData
	{
		// Values can be NULL if extraction failed.
		const char* OriginalCommand;
		const char* ClanName; // QL only. Points to a null string if not available.
		const char* PlayerName;
		const char* Message;
	};

	struct udtParseDataChat
	{
		udtChatEventData Strings[2]; // Index 0 with color codes, 1 without.
		s32 ServerTimeMs;
		s32 PlayerIndex; // QL only. Negative if not available. Range: [0;63].
	};

	struct udtMatchInfo
	{
		// The time between the warm-up end and the match start is the countdown phase, 
		// whose length might vary depending on the game, mod, mode, etc.
		s32 WarmUpEndTimeMs; // S32_MIN if the demo doesn't have the warm-up start.
		s32 MatchStartTimeMs; // S32_MIN if the demo doesn't have the match start.
		s32 MatchEndTimeMs; // S32_MIN if the demo doesn't have the match end.
	};

	struct udtParseDataGameState
	{
		const udtMatchInfo* Matches;
		u32 MatchCount;
		u32 FileOffset; // In bytes.
		s32 FirstSnapshotTimeMs; // Server time. 
		s32 LastSnapshotTimeMs; // Server time.
	};

#pragma pack(pop)

	//
	// A bunch of simple stand-alone helper functions.
	//

	// Returns a null-terminated string describing the library's version.
	UDT_API(const char*) udtGetVersionString();

	// Returns zero if not a valid protocol.
	UDT_API(s32) udtIsValidProtocol(udtProtocol::Id protocol);

	// Returns zero if not a valid protocol.
	UDT_API(u32) udtGetSizeOfIdEntityState(udtProtocol::Id protocol);

	// Returns zero if not a valid protocol.
	UDT_API(u32) udtGetSizeOfidClientSnapshot(udtProtocol::Id protocol);

	// Returns NULL if invalid.
	UDT_API(const char*) udtGetFileExtensionByProtocol(udtProtocol::Id protocol);

	// Returns Protocol::Invalid if invalid.
	UDT_API(udtProtocol::Id) udtGetProtocolByFilePath(const char* filePath);

	//
	// The configurable API for fine-grained task selection.
	// All functions returning a s32 value return an error code of type udtErrorCode::Id.
	//

	// Create a context that can be used by multiple parsers.
	UDT_API(udtParserContext*) udtCreateContext();

	// Release all the resources associated to the context.
	UDT_API(s32) udtDestroyContext(udtParserContext* context);

	// Splits a demo into multiple sub-demos if the input demo has more than 1 gamestate server message.
	UDT_API(s32) udtSplitDemo(udtParserContext* context, const udtFileParseArg* info, const char* outputFolderPath);

	// Creates a sub-demo starting and ending at the specified times.
	UDT_API(s32) udtCutDemoByTime(udtParserContext* context, const udtCutByTimeArg* info);

	// Creates sub-demos around every occurrence of a matching chat command server message.
	UDT_API(s32) udtCutDemoByChat(udtParserContext* context, const udtCutByChatArg* info);

	// Read through an entire demo file.
	// Can be configured for various analysis and data extraction tasks.
	// The "plugIns" arguments are of type udtParserPlugIn::Id.
	UDT_API(s32) udtParseDemo(udtParserContext* context, const udtFileParseArg* info, const u32* plugIns, u32 plugInCount);

	// Get the address and element count for the requested parse data type.
	// The "plugInId" argument is of type udtParserPlugIn::Id.
	UDT_API(s32) udtGetDemoDataInfo(udtParserContext* context, u32 plugInId, void** buffer, u32* count);

#ifdef __cplusplus
}
#endif


#undef UDT_API
