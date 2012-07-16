#ifndef UDT_API_H
#define UDT_API_H


#ifdef UDT_CREATE_DLL
#	define UDT_EXPORT_DLL __declspec(dllexport)
#else
#	define UDT_EXPORT_DLL __declspec(dllimport)
#endif

#define UDT_API(ReturnType) extern UDT_EXPORT_DLL ReturnType


struct UdtLibrary;
typedef UdtLibrary* UdtHandle;

struct ErrorCode
{
	enum Id
	{
		None,
		InvalidArgument
	};
};

struct Protocol
{
	enum Id
	{
		Invalid,
		Dm68,
		Dm73
	};
};


#ifdef __cplusplus
extern "C" 
{
#endif

	// Return 0 to continue, anything else to cancel.
	typedef int  (*ProgressCallback)(float progress);

	// Log levels: 0=info, 1=warning, 3=error, 4=error+crash.
	typedef void (*MessageCallback)(int logLevel, const char* message);

	// Returns a null-terminated string describing the library's version.
	UDT_API(const char*)	udtGetVersionString();

	// Allocate memory for the library and returns a handle. Returns 0 on error.
	UDT_API(UdtHandle)		udtCreate(int protocolId);

	// Destroys and deallocates everything.
	UDT_API(void)			udtDestroy(UdtHandle lib);

	// Sets the progress callback function.
	UDT_API(int)			udtSetProgressCallback(UdtHandle lib, ProgressCallback progressCb);

	// Sets the log message callback function.
	UDT_API(int)			udtSetMessageCallback(UdtHandle lib, MessageCallback messageCb);

	// Sets the offset from which to start reading the file.
	UDT_API(int)			udtSetFileStartOffset(UdtHandle lib, int fileStartOffset);

	// Set the initial server time offset.
	UDT_API(int)			udtSetServerTimeOffset(UdtHandle lib, int serverTimeOffset);

	// Add a scheduled cut to be processed when calling udtParse.
	UDT_API(int)			udtAddCut(UdtHandle lib, const char* filePath, int startTimeMs, int endTimeMs);

	// Parses the given file path.
	UDT_API(int)			udtParse(UdtHandle lib, const char* filePath, ProgressCallback progressCb, MessageCallback messageCb);

	// Get the warm-up end time, in server time (ms).
	UDT_API(int)			udtGetWarmupEndTimeMs(UdtHandle lib, int* warmupEndTimeMs);

	// Get the first snapshot's time, in server time (ms).
	UDT_API(int)			udtGetFirstSnapshotTimeMs(UdtHandle lib, int* firstSnapshotTimeMs);

	// Get the game state message count.
	UDT_API(int)			udtGetGameStateCount(UdtHandle lib, int* gameStateCount);

	// Get the file offset at which the game state was encountered.
	UDT_API(int)			udtGetGameStateFileOffset(UdtHandle lib, int gameStateIdx, int* fileOffset);

	// Get the server time offset at which the game state was encountered.
	UDT_API(int)			udtGetGameStateServerTimeOffset(UdtHandle lib, int gameStateIdx, int* serverTimeOffset);

	// Get the server time of the first snapshot encountered after the game state message was encountered.
	UDT_API(int)			udtGetGameStateFirstSnapshotTime(UdtHandle lib, int gameStateIdx, int* firstSnapshotTime);

	// Get the server time of the last snapshot encountered after the game state message was encountered.
	UDT_API(int)			udtGetGameStateLastSnapshotTime(UdtHandle lib, int gameStateIdx, int* lastSnapshotTime);

	// Get the number of server commands.
	UDT_API(int)			udtGetServerCommandCount(UdtHandle lib, int* cmdCount);

	// Get the sequence number of a server command entry.
	UDT_API(int)			udtGetServerCommandSequence(UdtHandle lib, int cmdIndex, int* seqNumber);

	// Get the message string of a server command entry.
	UDT_API(int)			udtGetServerCommandMessage(UdtHandle lib, int cmdIndex, char* valueBuffer, int bufferLength);

	// Get the number of config strings.
	UDT_API(int)			udtGetConfigStringCount(UdtHandle lib, int* udtCsCount);

	// Get the value string of a config string entry.
	UDT_API(int)			udtGetConfigStringValue(UdtHandle lib, int udtCsIndex, char* valueBuffer, int bufferLength);

	// Get the index of a config string entry.
	UDT_API(int)			udtGetConfigStringIndex(UdtHandle lib, int udtCsIndex, int* quakeCsIndex);
	
	// Get the number of chat entries.
	UDT_API(int)			udtGetChatStringCount(UdtHandle lib, int* chatCount);

	// Get the message string of a chat entry.
	UDT_API(int)			udtGetChatStringMessage(UdtHandle lib, int chatIndex, char* valueBuffer, int bufferLength);

	// Get the timestamp of a chat entry, in ms.
	UDT_API(int)			udtGetChatStringTime(UdtHandle lib, int chatIndex, int* time);

	// Get the number of obituary events.
	UDT_API(int)			udtGetObituaryCount(UdtHandle lib, int* obituaryCount);

	// Get the virtual server time of an obituary event.
	UDT_API(int)			udtGetObituaryTime(UdtHandle lib, int obituaryIndex, int* time);

	// Get the attacking player name of an obituary event.
	UDT_API(int)			udtGetObituaryAttackerName(UdtHandle lib, int obituaryIndex, char* nameBuffer, int bufferLength);

	// Get the target player name of an obituary event.
	UDT_API(int)			udtGetObituaryTargetName(UdtHandle lib, int obituaryIndex, char* nameBuffer, int bufferLength);

	// Get the mean of death of an obituary event.
	UDT_API(int)			udtGetObituaryMeanOfDeath(UdtHandle lib, int obituaryIndex, int* mod);

	// Get the number of power-up frag runs.
	UDT_API(int)			udtGetPuRunCount(UdtHandle lib, int* puRunCount);

	// Get the start time of a power-up frag run.
	UDT_API(int)			udtGetPuRunTime(UdtHandle lib, int puRunIndex, int* time);

	// Get the player name of a power-up frag run.
	UDT_API(int)			udtGetPuRunPlayerName(UdtHandle lib, int puRunIndex, char* nameBuffer, int bufferLength);

	// Get the power-up index of a power-up frag run.
	UDT_API(int)			udtGetPuRunPu(UdtHandle lib, int puRunIndex, int* pu);

	// Get the duration, in milli-seconds, of a power-up frag run.
	UDT_API(int)			udtGetPuRunDuration(UdtHandle lib, int puRunIndex, int* durationMs);

	// Get the enemy kill count of a power-up frag run.
	UDT_API(int)			udtGetPuRunKillCount(UdtHandle lib, int puRunIndex, int* kills);

	// Get the teammate kill count of a power-up frag run.
	UDT_API(int)			udtGetPuRunTeamKillCount(UdtHandle lib, int puRunIndex, int* teamKills);

	// Returns non-zero in selfKill if the power-up frag run ends with a self-kill.
	UDT_API(int)			udtGetPuRunSelfKill(UdtHandle lib, int puRunIndex, int* selfKill);

	// Cuts the demo.
	UDT_API(int)			udtCutDemoTime(const char* inFilePath, const char* outFilePath, int startTimeMs, int endTimeMs, ProgressCallback progressCb, MessageCallback messageCb);

	// Cuts the demo around any chat message that contains one of the search entry listed.
	UDT_API(int)			udtCutDemoChat(const char* inFilePath, const char* outFilePath, const char** chatEntries, int chatEntryCount, int startOffsetSecs, int endOffsetSecs);

#ifdef __cplusplus
}
#endif

#undef UDT_API


#endif