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

#define UDT_BIT(x) (1 << x)


#define UDT_API UDT_API_DECL


struct udtParserContext;
struct udtParserContextGroup;

#define UDT_ERROR_LIST(N) \
	N(None, "no error") \
	N(InvalidArgument, "invalid argument") \
	N(OperationFailed, "operation failed") \
	N(OperationCanceled, "operation canceled") \
	N(Unprocessed, "unprocessed job")

#define UDT_ERROR_ITEM(Enum, Desc) Enum,
struct udtErrorCode
{
	enum Id
	{
		UDT_ERROR_LIST(UDT_ERROR_ITEM)
		AfterLastError
	};
};
#undef UDT_ERROR_ITEM

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

struct udtCrashType
{
	enum Id
	{
		FatalError,
		ReadAccess,
		WriteAccess,
		Count
	};
};

// Macro arguments:
// 1. enum name 
// 2. plug-in type (internal)
// 3. plug-in data type (for the user)
#define UDT_PLUG_IN_LIST(N) \
	N(Chat,       udtParserPlugInChat,       udtParseDataChat) \
	N(GameState,  udtParserPlugInGameState,  udtParseDataGameState) \
	N(Obituaries, udtParserPlugInObituaries, udtParseDataObituary)

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

#define UDT_WEAPON_LIST(N) \
	N(Gauntlet, "gauntlet", 0) \
	N(MachineGun, "machine gun", 1) \
	N(Shotgun, "shotgun", 2) \
	N(Grenade, "grenade launcher", 3) \
	N(Rocket, "rocket launcher", 4) \
	N(Plasma, "plasma gun", 5) \
	N(Railgun, "railgun", 6) \
	N(LightningGun, "lightning gun", 7) \
	N(BFG, "BFG", 8) \
	N(NailGun, "nailgun", 9) \
	N(ChainGun, "chaingun", 10) \
	N(ProximityMineLauncher, "proximity mine launcher", 11) \
	N(HeavyMachineGun, "heavy machine gun", 12)

#define UDT_WEAPON_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtWeaponBits
{
	enum Id
	{
		UDT_WEAPON_LIST(UDT_WEAPON_ITEM)
		AfterLast
	};
};
#undef UDT_WEAPON_ITEM

#define UDT_POWER_UP_LIST(N) \
	N(QuadDamage, "quad damage", 0) \
	N(BattleSuit, "battle suit", 1) \
	N(Haste, "haste", 2) \
	N(Invisibility, "invisibility", 3) \
	N(Regeneration, "regeneration", 4) \
	N(Flight, "flight", 5) \
	N(RedFlag, "red flag", 6) \
	N(BlueFlag, "blue flag", 7) \
	N(NeutralFlag, "neutral flag", 8) \
	N(Scout, "scout", 9) \
	N(Guard, "guard", 10) \
	N(Doubler, "doubler", 11) \
	N(ArmorRegeneration, "armor regeneration", 12) \
	N(Invulnerability, "invulnerability", 13)

#define UDT_POWER_UP_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtPowerUpBits
{
	enum Id
	{
		UDT_POWER_UP_LIST(UDT_POWER_UP_ITEM)
		AfterLast
	};
};
#undef UDT_POWER_UP_ITEM

#define UDT_MOD_LIST(N) \
	N(Shotgun, "shotgun", 0) \
	N(Gauntlet, "gauntlet", 1) \
	N(MachineGun, "machine gun", 2) \
	N(Grenade, "grenade", 3) \
	N(GrenadeSplash, "grenade splash", 4) \
	N(Rocket, "rocket", 5) \
	N(RocketSplash, "rocket splash", 6) \
	N(Plasma, "plasma", 7) \
	N(PlasmaSplash, "plasma splash", 8) \
	N(Railgun, "railgun", 9) \
	N(Lightning, "lightning", 10) \
	N(BFG, "BFG", 11) \
	N(BFGSplash, "BFG splash", 12) \
	N(Water, "water", 13) \
	N(Slime, "slime", 14) \
	N(Lava, "lava", 15) \
	N(Crush, "crush", 16) \
	N(TeleFrag, "telefrag", 17) \
	N(Fall, "fall", 18) \
	N(Suicide, "suicide", 19) \
	N(TargetLaser, "target laser", 20) \
	N(TriggerHurt, "trigger hurt", 21) \
	N(NailGun, "nailgun", 22) \
	N(ChainGun, "chaingun", 23) \
	N(ProximityMine, "proximity mine", 24) \
	N(Kamikaze, "kamikaze", 25) \
	N(Juiced, "juiced", 26) \
	N(Grapple, "grapple", 27) \
	N(TeamSwitch, "team switch", 28) \
	N(Thaw, "thaw", 29) \
	N(UnknownQlMod1, "unknown QL MOD #1", 30) \
	N(HeavyMachineGun, "heavy machine gun", 31)

#define UDT_MOD_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtMeansOfDeathBits
{
	enum Id
	{
		UDT_MOD_LIST(UDT_MOD_ITEM)
		AfterLast,
		MissionPackStart = NailGun,
		MissionPackEnd = Juiced,
		QLStart = TeamSwitch,
		QLEnd = HeavyMachineGun,
	};
};
#undef UDT_MOD_ITEM

#define UDT_PLAYER_MOD_LIST(N) \
	N(Shotgun, "shotgun", 0) \
	N(Gauntlet, "gauntlet", 1) \
	N(MachineGun, "machine gun", 2) \
	N(Grenade, "grenade", 3) \
	N(GrenadeSplash, "grenade splash", 4) \
	N(Rocket, "rocket", 5) \
	N(RocketSplash, "rocket splash", 6) \
	N(Plasma, "plasma", 7) \
	N(PlasmaSplash, "plasma splash", 8) \
	N(Railgun, "railgun", 9) \
	N(Lightning, "lightning", 10) \
	N(BFG, "BFG", 11) \
	N(BFGSplash, "BFG splash", 12) \
	N(TeleFrag, "telefrag", 13) \
	N(NailGun, "nailgun", 14) \
	N(ChainGun, "chaingun", 15) \
	N(ProximityMine, "proximity mine", 16) \
	N(Kamikaze, "kamikaze", 17) \
	N(Grapple, "grapple", 18) \
	N(Thaw, "thaw", 19) \
	N(HeavyMachineGun, "heavy machine gun", 20)

#define UDT_PLAYER_MOD_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtPlayerMeansOfDeathBits
{
	enum Id
	{
		UDT_PLAYER_MOD_LIST(UDT_PLAYER_MOD_ITEM)
		AfterLast
	};
};
#undef UDT_PLAYER_MOD_ITEM

#define UDT_TEAM_LIST(N) \
	N(Free, "free") \
	N(Red, "red") \
	N(Blue, "blue") \
	N(Spectators, "spectators")

#define UDT_TEAM_ITEM(Enum, Desc) Enum,
struct udtTeam
{
	enum Id
	{
		UDT_TEAM_LIST(UDT_TEAM_ITEM)
		Count
	};
};
#undef UDT_TEAM_ITEM

struct udtStringArray
{
	enum Id
	{
		Weapons,
		PowerUps,
		MeansOfDeath,
		PlayerMeansOfDeath,
		Teams,
		Count
	};
};


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

		// May be NULL.
		// Zero to proceed, non-zero to cancel the current operation.
		s32* CancelOperation;

		// Number of elements in the array pointed to by the PlugIns pointer.
		// May be 0.
		// Unused when cutting.
		u32 PlugInCount;

		// The index of the game state that will be read when starting at offset FileOffset.
		// Unused in batch operations.
		s32 GameStateIndex;

		// The offset, in bytes, at which to start reading from the file.
		// Unused in batch operations.
		u32 FileOffset;
	};

	struct udtMultiParseArg
	{
		// Pointer to an array of file paths.
		const char** FilePaths;

		// Pointer to an array of returned error codes.
		s32* OutputErrorCodes;

		// Number of elements in the arrays pointed by FilePaths and OutputErrorCodes.
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

		// The game state index for which this cut is applied
		s32 GameStateIndex;

		// Ignore this.
		s32 Reserved1;
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

	struct udtCutByFragArgFlags
	{
		enum Id
		{
			AllowSelfKills = UDT_BIT(0),
			AllowTeamKills = UDT_BIT(1),
			AllowDeaths    = UDT_BIT(2)
		};
	};

	struct udtCutByFragArg
	{
		// The minimum amount of frags in a sequence.
		u32 MinFragCount;

		// Time interval between 2 consecutive frags, in seconds.
		// See TimeMode for the interpretation of this value.
		u32 TimeBetweenFragsSec;

		// @TODO: Not supported for now.
		// If 0, TimeBetweenFragsSec is the maximum time interval between 
		// 2 consecutive frags, in seconds.
		// If 1, TimeBetweenFragsSec is the maximum average time between frags
		// for the entire frag run, in seconds.
		u32 TimeMode;

		// Negative offset from the first frag's time, in seconds.
		u32 StartOffsetSec;

		// Positive offset from the last frag's time, in seconds.
		u32 EndOffsetSec;

		// The index of the player whose frags we're looking at.
		// If valid ([0;63] range), it's used.
		// If not, will use the index of the player who recorded the demo.
		s32 PlayerIndex;

		// Boolean options.
		// See udtCutByFragArgFlags.
		u32 Flags;
		
		// All the allowed weapons.
		// See udtPlayerMeansOfDeathBits.
		u32 AllowedMeansOfDeaths;

		// @TODO:
		//u32 AllowedPowerUps;
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

		// The index of the last gamestate message after which this chat event occurred.
		// Negative if invalid or not available.
		s32 GameStateIndex;

		// Ignore this.
		s32 Reserved1;
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

	struct udtParseDataObituary
	{
		// The name of the attacker or another string.
		// Never NULL.
		const char* AttackerName;

		// The name of the attacker or another string.
		// Never NULL.
		const char* TargetName;

		// The name of the attacker or another string.
		// Never NULL.
		const char* MeanOfDeathName;

		// Ignore this.
		const char* Reserved1;

		// The index of the last gamestate message after which this death event occurred.
		// Negative if invalid or not available.
		s32 GameStateIndex;

		// The time at which the death happened.
		s32 ServerTimeMs;

		// The index of the attacking player.
		// If available, in range [0;63].
		s32 AttackerIdx;

		// The index of the player who died.
		// If available, in range [0;63].
		s32 TargetIdx;

		// The way the target died.
		// See meansOfDeath_t.
		s32 MeanOfDeath;

		// The index of the attacker's team.
		// Of type udtTeam::Id.
		// Negative if not available.
		s32 AttackerTeamIdx;

		// The index of the target's team.
		// Of type udtTeam::Id.
		// Negative if not available.
		s32 TargetTeamIdx;
		
		// Ignore this.
		s32 Reserved2;
	};

#pragma pack(pop)

	//
	// A bunch of simple stand-alone helper functions.
	//

	// Returns a null-terminated string describing the library's version.
	// Never returns NULL.
	UDT_API(const char*) udtGetVersionString();

	// Returns a null-terminated string describing the error.
	// Never returns NULL.
	UDT_API(const char*) udtGetErrorCodeString(s32 errorCode);

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

	// Raises the type of error asked for.
	UDT_API(s32) udtCrash(udtCrashType::Id crashType);

	// Retrieve the string array for the given array identifier.
	UDT_API(s32) udtGetStringArray(udtStringArray::Id arrayId, const char*** elements, u32* elementCount);

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

	// Creates a new demo cut for every matching frag sequence.
	UDT_API(s32) udtCutDemoFileByFrag(udtParserContext* context, const udtParseArg* info, const udtCutByFragArg* fragInfo, const char* demoFilePath);

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

	// Gets the input index of the specified demo.
	UDT_API(s32) udtGetDemoInputIndex(udtParserContext* context, u32 demoIdx, u32* demoInputIdx);

	// Releases all the resources associated to the context group.
	UDT_API(s32) udtDestroyContextGroup(udtParserContextGroup* contextGroup);

	// Creates sub-demos around every occurrence of a matching chat command server message.
	UDT_API(s32) udtCutDemoFilesByChat(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByChatArg* chatInfo);

	// Creates a new demo cut for every matching frag sequence for each demo.
	UDT_API(s32) udtCutDemoFilesByFrag(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByFragArg* fragInfo);

#ifdef __cplusplus
}
#endif


#undef UDT_API
