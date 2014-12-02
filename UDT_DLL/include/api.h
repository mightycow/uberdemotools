#pragma once


#include "types.hpp"


#if defined(UDT_MSVC)
#	if defined(UDT_CREATE_DLL)
#		define UDT_EXPORT_DLL          __declspec(dllexport)
#	else
#		define UDT_EXPORT_DLL          __declspec(dllimport)
#	endif
#	define UDT_API_DECL(ReturnType)    extern UDT_EXPORT_DLL ReturnType
#	define UDT_API_DEF(ReturnType)     UDT_EXPORT_DLL ReturnType
#elif defined(UDT_GCC)
#	if defined(UDT_CREATE_DLL)
#		define UDT_EXPORT_DLL
#	else
#		define UDT_EXPORT_DLL
#	endif
#	define UDT_API_DECL(ReturnType)    extern ReturnType UDT_EXPORT_DLL
#	define UDT_API_DEF(ReturnType)     ReturnType UDT_EXPORT_DLL
#endif


#define UDT_API UDT_API_DECL


struct udtParserContext;
struct udtParserContextGroup;

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
		FirstProtocol = Dm68,
		Count = AfterLastProtocol - 1
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
	N(Chat,      udtParserPlugInChat,      udtParseDataChat) \
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
	
	// "userData" is the member variable udtParseArg::ProgressContext that you pass to API functions.
	typedef void (*udtProgressCallback)(f32 progress, void* userData);

	// Log levels: 0=info, 1=warning, 2=error, 3=error+crash.
	typedef void (*udtMessageCallback)(s32 logLevel, const char* message);

	// Called when UDT can't recover from the error.
	// Implement to throw an exception, generate a stack trace, etc.
	// Default behavior: calls the C function exit.
	typedef void (*udtCrashCallback)(const char* message);

#pragma pack(push, 1)
	
	struct udtParseArg
	{
		// Pointer to an array of plug-ins IDs.
		// Of type udtParserPlugIn::Id.
		// May be NULL.
		// Unused when cutting.
		const u32* PlugIns;

		// May be NULL.
		// Unused when not cutting.
		const char* OutputFolderPath;

		// May be NULL.
		// Used for info, warning and non-fatal error messages.
		// Fatal errors are routed through a udtCrashCallback callback.
		udtMessageCallback MessageCb;

		// May be NULL.
		udtProgressCallback ProgressCb;

		// May be NULL.
		// This is passed as "userData" to "ProgressCb".
		void* ProgressContext;

		// Number of elements in the array pointed to by the PlugIns pointer.
		// May be 0.
		// Unused when cutting.
		u32 PlugInCount;

		// Zero to proceed, non-zero to cancel the current operation.
		s32 CancelOperation;
	};

	struct udtMultiParseArg
	{
		// Pointer to an array of file paths.
		const char** FilePaths;

		// Number of elements in the array pointed by FilePaths.
		u32 FileCount;

		// The maximum amount of threads that should be used to process the demos.
		u32 MaxThreadCount;
	};

	
	struct udtCut
	{
		// Cut start time in milli-seconds.
		s32 StartTimeMs;

		// Cut end time in milli-seconds.
		s32 EndTimeMs;
	};

	struct udtCutByTimeArg
	{
		// Pointer to an array of cut times.
		// May not be NULL.
		const udtCut* Cuts;

		// Number of elements in the array pointed by Cuts.
		u32 CutCount;
	};

	struct udtCutByChatRule
	{
		// May not be NULL.
		const char* Pattern;

		// Of type udtChatOperator::Id.
		u32 ChatOperator;

		// Non-zero means case-sensitive.
		u32 CaseSensitive;

		// Non-zero means color codes are ignored.
		u32 IgnoreColorCodes;
	};

	struct udtCutByChatArg
	{
		// Pointer to an array of chat cutting rules.
		// Rules are OR'd together.
		// May not be NULL.
		const udtCutByChatRule* Rules;

		// Number of elements in the array pointed to by the Rules pointer.
		// May not be 0.
		u32 RuleCount;

		// Negative offset from the matching time, in seconds.
		u32 StartOffsetSec;

		// Positive offset from the matching time, in seconds.
		u32 EndOffsetSec;
	};

	struct udtChatEventData
	{
		// All C string pointers can be NULL if extraction failed.

		// The original, unmodified command string.
		const char* OriginalCommand;

		// The player's active clan name at the time the demo was recorded.
		// Not available in protocol version 68.
		// Points to a null string if not available.
		const char* ClanName;

		// The player's name.
		const char* PlayerName;

		// The message itself.
		const char* Message;
	};

	struct udtParseDataChat
	{
		// String data for this chat message.
		// Index 0 with color codes, 1 without.
		udtChatEventData Strings[2];

		// The time at which the chat message was sent from the client.
		s32 ServerTimeMs;

		// The index of the player who sent the message.
		// Not available in protocol version 68.
		// Negative if not available.
		// If available, in range [0;63].
		s32 PlayerIndex;
	};

	struct udtMatchInfo
	{
		// The time between the warm-up end and the match start is the countdown phase, 
		// whose length might vary depending on the game, mod, mode, etc.

		// The time the warm-up ends (countdown start), in milli-seconds.
		// S32_MIN if not available.
		s32 WarmUpEndTimeMs; 

		// The time the match starts (countdown end), in milli-seconds.
		// S32_MIN if not available.
		s32 MatchStartTimeMs;

		// The time the match ends, in milli-seconds.
		// S32_MIN if not available.
		s32 MatchEndTimeMs;
	};

	struct udtParseDataGameState
	{
		// Pointer to an array of match informations.
		const udtMatchInfo* Matches;

		// Number of elements in the array pointed to by the Matches pointer.
		u32 MatchCount;

		// File offset, in bytes, where the "gamestate" message is.
		u32 FileOffset;

		// Time of the first snapshot, in milli-seconds.
		s32 FirstSnapshotTimeMs;

		// Time of the last snapshot, in milli-seconds.
		s32 LastSnapshotTimeMs;
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

	// Sets the global fatal error handler.
	// If you pass NULL, will set it back to the default handler.
	UDT_API(s32) udtSetCrashHandler(udtCrashCallback crashHandler);

	// Creates a context that can be used by multiple parsers.
	UDT_API(udtParserContext*) udtCreateContext();

	// Releases all the resources associated to the context.
	UDT_API(s32) udtDestroyContext(udtParserContext* context);

	// Splits a demo into multiple sub-demos if the input demo has more than 1 gamestate server message.
	UDT_API(s32) udtSplitDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath);

	// Creates a sub-demo starting and ending at the specified times.
	UDT_API(s32) udtCutDemoFileByTime(udtParserContext* context, const udtParseArg* info, const udtCutByTimeArg* cutInfo, const char* demoFilePath);

	// Creates sub-demos around every occurrence of a matching chat command server message.
	UDT_API(s32) udtCutDemoFileByChat(udtParserContext* context, const udtParseArg* info, const udtCutByChatArg* chatInfo, const char* demoFilePath);

	// Reads through an entire demo file.
	// Can be configured for various analysis and data extraction tasks.
	UDT_API(s32) udtParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath);

	// Gets the address and element count for the requested parse data type.
	// The "plugInId" argument is of type udtParserPlugIn::Id.
	UDT_API(s32) udtGetDemoDataInfo(udtParserContext* context, u32 demoIdx, u32 plugInId, void** buffer, u32* count);

	//
	// The configurable API for fine-grained task selection.
	// All functions returning a s32 value return an error code of type udtErrorCode::Id.
	// Batch processing functions.
	//

	// Reads through a group of demo files.
	// Can be configured for various analysis and data extraction tasks.
	UDT_API(s32) udtParseDemoFiles(udtParserContextGroup** contextGroup, const udtParseArg* info, const udtMultiParseArg* extraInfo);

	// Gets the amount of contexts stored in the context group.
	UDT_API(s32) udtGetContextCountFromGroup(udtParserContextGroup* contextGroup, u32* count);

	// Gets a context in a context group.
	UDT_API(s32) udtGetContextFromGroup(udtParserContextGroup* contextGroup, u32 contextIdx, udtParserContext** context);

	// Gets the total demo count for which plug-in data is stored in a context group.
	UDT_API(s32) udtGetDemoCountFromGroup(udtParserContextGroup* contextGroup, u32* count);

	// Gets the demo count for which plug-in data is stored in a context.
	UDT_API(s32) udtGetDemoCountFromContext(udtParserContext* context, u32* count);

	// Releases all the resources associated to the context group.
	UDT_API(s32) udtDestroyContextGroup(udtParserContextGroup* contextGroup);

	// Creates sub-demos around every occurrence of a matching chat command server message.
	UDT_API(s32) udtCutDemoFilesByChat(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByChatArg* chatInfo);

#ifdef __cplusplus
}
#endif


#undef UDT_API
