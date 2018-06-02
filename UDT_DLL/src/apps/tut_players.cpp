#include "uberdemotools.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <string>


/*
ABOUT THIS EXAMPLE APPLICATION
==============================

This little application shows how to use UDT's custom parsing API (functions whose name start with "udtCu") to
1. list the players connected at a given game state, and
2. print player connection, disconnection and rename events.

Even though this is sample code, it does extensive error testing to show off how to build
software using the library at the cost of some verbosity.
*/


static const char* LogLevelPrefixes[4] =
{
	"",
	"Warning: ",
	"Error: ",
	"Fatal: "
};

static FILE* LogLevelFiles[4] =
{
	stdout,
	stdout,
	stderr,
	stderr
};


static void PrintHelp()
{
	printf("For a given demo file, lists all the players and join/disconnect/rename events.\n");
	printf("\n");
	printf("tut_players inputfile\n");
}

// Not necessarily the best way to perform this for your specific platform.
static bool IsValidFilePath(const char* filePath)
{
	FILE* const file = fopen(filePath, "rb");
	if(file == NULL)
	{
		return false;
	}

	fclose(file);

	return true;
}

static void CrashCallback(const char* message)
{
	fprintf(stderr, "Fatal error: %s\n", message);
	exit(666);
}

static void MessageCallback(s32 logLevel, const char* message)
{
	if(logLevel < 0 || logLevel >= 4)
	{
		return;
	}

	fprintf(LogLevelFiles[logLevel], "%s%s\n", LogLevelPrefixes[logLevel], message);
}

static void PrintInfo(const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stdout, format, argptr);
	va_end(argptr);

	fprintf(stdout, "\n");
}

static void PrintWarning(const char* format, ...)
{
	fprintf(stdout, "Warning: ");

	va_list argptr;
	va_start(argptr, format);
	vfprintf(stdout, format, argptr);
	va_end(argptr);

	fprintf(stdout, "\n");
}

static void PrintError(const char* format, ...)
{
	fprintf(stderr, "Error: ");

	va_list argptr;
	va_start(argptr, format);
	vfprintf(stderr, format, argptr);
	va_end(argptr);

	fprintf(stderr, "\n");
}

static void FormatServerTime(std::string& serverTime, s32 serverTimeMs)
{
	char timeString[64];
	const s32 totalSeconds = serverTimeMs / 1000;
	const s32 minutes = totalSeconds / 60;
	const s32 seconds = totalSeconds % 60;
	sprintf(timeString, "%3d:%02d", minutes, seconds);
	serverTime = timeString;
}

static bool ExtractRawPlayerName(std::string& playerName, const std::string& configString)
{
	char name[256];
	char temp[4];
	const s32 errorCode = udtParseConfigStringValueAsString(name, (u32)sizeof(name), temp, (u32)sizeof(temp), "n", configString.c_str());
	assert(errorCode != (s32)udtErrorCode::InsufficientBufferSize);
	if(errorCode != (s32)udtErrorCode::None)
	{
		return false;
	}

	playerName = name;

	return true;
}

static bool ExtractPlayerName(std::string& playerName, const std::string& configString, udtProtocol::Id protocol)
{
	if(!ExtractRawPlayerName(playerName, configString))
	{
		return false;
	}

	if(playerName.empty())
	{
		return true;
	}

	std::vector<char> cleanedUpName(playerName.length() + 1);
	strcpy(&cleanedUpName[0], playerName.c_str());
	udtCleanUpString(&cleanedUpName[0], (u32)protocol);
	playerName = &cleanedUpName[0];

	return true;
}

struct FileReader
{
	FileReader()
	{
		_file = NULL;
	}

	~FileReader()
	{
		if(_file != NULL)
		{
			fclose(_file);
		}
	}

	bool Open(const char* filePath)
	{
		FILE* const file = fopen(filePath, "rb");
		if(file == NULL)
		{
			return false;
		}

		_file = file;

		return true;
	}

	bool Read(void* buffer, u32 byteCount)
	{
		return fread(buffer, (size_t)byteCount, 1, _file) == 1;
	}

	void Close()
	{
		if(_file != NULL)
		{
			fclose(_file);
			_file = NULL;
		}
	}

private:
	FileReader(const FileReader&);
	void operator=(const FileReader&);

	FILE* _file;
};

struct PlayerPrinter
{
	PlayerPrinter()
	{
		_cuContext = NULL;
		_inMsgData = NULL;
		_protocol = udtProtocol::Invalid;
		_demoTakerClientNumber = -1;
		_protocolFirstPlayerCsIdx = -1;
	}

	~PlayerPrinter()
	{
		if(_cuContext != NULL)
		{
			udtCuDestroyContext(_cuContext);
		}

		free(_inMsgData);
	}

	bool Init()
	{
		udtCuContext* const cuContext = udtCuCreateContext();
		if(cuContext == NULL)
		{
			PrintError("udtCuCreateContext failed");
			return false;
		}
		_cuContext = cuContext;
		
		u8* const inMsgData = (u8*)malloc(ID_MAX_MSG_LENGTH);
		if(inMsgData == NULL)
		{
			PrintError("Failed to allocated %d bytes", (int)ID_MAX_MSG_LENGTH);
			return false;
		}
		_inMsgData = inMsgData;

		udtCuSetMessageCallback(cuContext, &MessageCallback);

		return true;
	}

	bool ProcessDemoFile(const char* filePath)
	{
		for(u32 i = 0; i < ID_MAX_CLIENTS; ++i)
		{
			_players[i].Name.clear();
			_players[i].Valid = false;
		}

		FileReader reader;
		if(!reader.Open(filePath))
		{
			return false;
		}

		udtCuContext* const cuContext = _cuContext;
		const u32 protocol = udtGetProtocolByFilePath(filePath);

		s32 firstPlayerIdx;
		if(udtGetIdMagicNumber(&firstPlayerIdx, (u32)udtMagicNumberType::ConfigStringIndex, (s32)udtConfigStringIndex::FirstPlayer, protocol, (u32)udtMod::None) != 
		   udtErrorCode::None)
		{
			PrintError("Failed to get the index of the first player config string");
			return false;
		}

		_protocol = (udtProtocol::Id)protocol;
		_protocolFirstPlayerCsIdx = firstPlayerIdx;

		s32 errorCode = udtCuStartParsing(cuContext, protocol);
		if(errorCode != udtErrorCode::None)
		{
			PrintError("udtCuStartParsing failed: %s", udtGetErrorCodeString(errorCode));
			return false;
		}

		udtCuMessageInput input;
		udtCuMessageOutput output;
		u32 continueParsing = 0;
		for(;;)
		{
			if(!reader.Read(&input.MessageSequence, 4))
			{
				PrintWarning("Demo is truncated");
				return true;
			}

			if(!reader.Read(&input.BufferByteCount, 4))
			{
				PrintWarning("Demo is truncated");
				return true;
			}

			if(input.MessageSequence == -1 &&
			   input.BufferByteCount == u32(-1))
			{
				// End of demo file.
				break;
			}

			if(input.BufferByteCount > ID_MAX_MSG_LENGTH)
			{
				PrintError("Corrupt input: the buffer length exceeds the maximum allowed");
				return false;
			}

			if(!reader.Read(_inMsgData, input.BufferByteCount))
			{
				PrintWarning("Demo is truncated");
				return true;
			}
			input.Buffer = _inMsgData;

			errorCode = udtCuParseMessage(cuContext, &output, &continueParsing, &input);
			if(errorCode != udtErrorCode::None)
			{
				PrintError("udtCuParseMessage failed: %s", udtGetErrorCodeString(errorCode));
				return false;
			}

			if(continueParsing == 0)
			{
				break;
			}

			AnalyzeMessage(output);
		}

		return true;
	}

private:
	PlayerPrinter(const PlayerPrinter&);
	void operator=(const PlayerPrinter&);

	void AnalyzeMessage(udtCuMessageOutput& message)
	{
		if(message.IsGameState && 
		   message.GameStateOrSnapshot.GameState != NULL)
		{
			AnalyzeGameState(*message.GameStateOrSnapshot.GameState);
		}
		else if(message.CommandCount > 0 && 
				message.Commands != NULL && 
				message.GameStateOrSnapshot.Snapshot != NULL)
		{
			const s32 serverTimeMs = message.GameStateOrSnapshot.Snapshot->ServerTimeMs;
			for(u32 i = 0; i < message.CommandCount; ++i)
			{
				AnalyzeCommand(message.Commands[i], serverTimeMs);
			}
		}
	}
	
	void AnalyzeGameState(const udtCuGamestateMessage& gameState)
	{
		_demoTakerClientNumber = gameState.ClientNumber;

		PrintInfo("");
		PrintInfo("================================");
		PrintInfo("New Game State");
		PrintInfo("================================");

		std::string playerName;
		udtCuConfigString cs;
		for(u32 i = 0; i < ID_MAX_CLIENTS; ++i)
		{
			udtCuGetConfigString(_cuContext, &cs, (u32)_protocolFirstPlayerCsIdx + i);
			if(cs.ConfigString != NULL && cs.ConfigStringLength > 0)
			{
				if(ExtractPlayerName(playerName, cs.ConfigString, _protocol))
				{
					_players[i].Name = playerName;
					_players[i].Valid = true;
					PrintInfo("Player: %s (client %u)%s", playerName.c_str(), i, (s32)i == _demoTakerClientNumber ? " <== demo taker" : "");
				}
			}
		}

		PrintInfo("================================");
	}

	void AnalyzeCommand(const udtCuCommandMessage& command, s32 serverTimeMs)
	{
		if(!command.IsConfigString)
		{
			return;
		}

		const s32 clientNumber = command.ConfigStringIndex - _protocolFirstPlayerCsIdx;
		if(clientNumber < 0 || clientNumber >= ID_MAX_CLIENTS)
		{
			// Not a player config string.
			return;
		}

		// Config string update commands always have 3 tokens:
		// 1. "cs"
		// 2. The index of the config string to update.
		// 3. The actual content, in quotes.
		if(command.TokenCount != 3)
		{
			// Not a proper config string command.
			return;
		}

		std::string serverTime;
		FormatServerTime(serverTime, serverTimeMs);

		const std::string configString = command.CommandTokens[2];
		std::string playerName;
		Player& player = _players[clientNumber];
		if(configString.empty() || !ExtractPlayerName(playerName, configString, _protocol))
		{
			PrintInfo("%s Player %s left (client %u)", serverTime.c_str(), player.Name.c_str(), (u32)clientNumber);
			player.Name = "";
			player.Valid = false;
			return;
		}
		
		const bool wasValid = player.Valid;
		player.Valid = true;
		if(!wasValid)
		{
			PrintInfo("%s Player %s joined (client %u)", serverTime.c_str(), playerName.c_str(), (u32)clientNumber);
			player.Name = playerName;
		}
		else if(playerName != player.Name)
		{
			PrintInfo("%s Player %s renamed to %s (client %u)", serverTime.c_str(), player.Name.c_str(), playerName.c_str(), (u32)clientNumber);
			player.Name = playerName;
		}
	}

	struct Player
	{
		std::string Name;
		bool Valid;
	};
	
	udtCuContext* _cuContext;
	u8* _inMsgData;
	udtProtocol::Id _protocol;
	s32 _demoTakerClientNumber;
	s32 _protocolFirstPlayerCsIdx;
	Player _players[ID_MAX_CLIENTS];
};


int main(int argc, char** argv)
{
	if(!udtSameVersion())
	{
		PrintError("Compiled with header for version %s, but linked against version %s", UDT_VERSION_STRING, udtGetVersionString());
		return 1;
	}

	if(argc < 2)
	{
		PrintHelp();
		return 2;
	}

	if(!IsValidFilePath(argv[1]))
	{
		PrintHelp();
		return 3;
	}

	udtSetCrashHandler(&CrashCallback);
	udtInitLibrary();

	PlayerPrinter printer;
	if(printer.Init())
	{
		printer.ProcessDemoFile(argv[1]);
	}

	udtShutDownLibrary();

	return 0;
}
