#include "uberdemotools.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <vector>
#include <string>


/*
ABOUT THIS EXAMPLE APPLICATION
==============================

This little application shows how to use UDT's custom parsing API (functions whose name start with "udtCu") to
1. detect multi-frag rail kills (1 shot with 2+ kills) from the player who recorded the demo, then
2. create cut demos for each such kill.

Even though this is sample code, it does extensive error testing to show off how to build 
software using the library at the cost of some verbosity.
*/


#define CUT_START_OFFSET_SEC (10)
#define CUT_END_OFFSET_SEC   (10)


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
	printf("For a given demo file, finds all the multi-frag rail shots\n");
	printf("from the player who recorded the demo.\n");
	printf("\n");
	printf("tut_multi_rail inputfile\n");
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

static const char* TupleNames[] =
{
	"none",
	"single",
	"double",
	"triple",
	"quadruple",
	"quintuple"
};

static const size_t TupleNameCount = sizeof(TupleNames) / sizeof(TupleNames[0]);

static const char* GetTupleNameByLength(u32 length)
{
	if(length >= (u32)TupleNameCount)
	{
		return "unknown";
	}

	return TupleNames[length];
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

struct Obituary
{
	s32 AttackerIndex;
	s32 TargetIndex;
	s32 MeanOfDeath;
};

static bool IsObituaryEvent(Obituary& obituary, const idEntityStateBase& entity, udtProtocol::Id protocol)
{	
	// Should get these values once before parsing instead of over and over during parsing.
	s32 obituaryEvtId;
	s32 eventTypeId;
	if(udtGetIdMagicNumber(&obituaryEvtId, udtMagicNumberType::EntityEvent, (u32)udtEntityEvent::Obituary, protocol, udtMod::None) != udtErrorCode::None ||
	   udtGetIdMagicNumber(&eventTypeId, udtMagicNumberType::EntityType, (u32)udtEntityType::Event, protocol, udtMod::None) != udtErrorCode::None)
	{
		return false;
	}

	// Make sure it's an obituary event.
	if(entity.eType != eventTypeId ||
	   entity.event != obituaryEvtId)
	{
		return false;
	}

	// The target must always be a player.
	const s32 targetIdx = entity.otherEntityNum;
	if(targetIdx < 0 || targetIdx >= ID_MAX_CLIENTS)
	{
		return false;
	}

	// The attacker can be the world, though.
	s32 attackerIdx = entity.otherEntityNum2;
	if(attackerIdx < 0 || attackerIdx >= ID_MAX_CLIENTS)
	{
		attackerIdx = -1;
	}

	// Should get this value once before parsing instead of over and over during parsing.
	s32 udtMod;
	if(udtGetUDTMagicNumber(&udtMod, (u32)udtMagicNumberType::MeanOfDeath, entity.eventParm, (u32)protocol, (u32)udtMod::None) != udtErrorCode::None)
	{
		return false;
	}

	obituary.AttackerIndex = attackerIdx;
	obituary.TargetIndex = targetIdx;
	obituary.MeanOfDeath = udtMod;
	
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

struct MultiFragRailCutter
{
	MultiFragRailCutter()
	{
		_cuContext = NULL;
		_context = NULL;
		_inMsgData = NULL;
		_gameStateIndex = -1;
		_trackedClientNumber = -1;
		_protocol = udtProtocol::Invalid;
		_protocolWriteEnabled = false;
	}

	~MultiFragRailCutter()
	{
		if(_cuContext != NULL)
		{
			udtCuDestroyContext(_cuContext);
		}

		if(_context != NULL)
		{
			udtDestroyContext(_context);
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

		udtParserContext* const context = udtCreateContext();
		if(context == NULL)
		{
			PrintError("udtCreateContext failed");
			return false;
		}
		_context = context;
		
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
		_inputFilePath = filePath;
		_cutSections.clear();
		_cutFilePaths.clear();

		if(!AnalyzeDemoFile(filePath))
		{
			return false;
		}

		if(_cutSections.empty())
		{
			return true;
		}

		const size_t cutCount = _cutSections.size();
		if(!_protocolWriteEnabled)
		{
			PrintInfo("%u multi-frag rail kill%s were found, here's the list:", cutCount, cutCount > 1 ? "s" : "");
			for(size_t i = 0; i < cutCount; ++i)
			{
				PrintInfo("- game state #%d, server time %d ms", 
						  (int)_cutSections[i].GameStateIndex + 1, 
						  (int)(_cutSections[i].StartTimeMs + CUT_START_OFFSET_SEC * 1000));
			}

			return true;
		}

		PrintInfo("%u multi-frag rail kill%s were found, attempting to cut now...", cutCount, cutCount > 1 ? "s" : "");

		// Fix up the file paths pointers.
		for(size_t i = 0; i < cutCount; ++i)
		{
			_cutSections[i].FilePath = _cutFilePaths[i].c_str();
		}

		udtCutByTimeArg arg;
		memset(&arg, 0, sizeof(arg));
		arg.CutCount = (u32)_cutSections.size();
		arg.Cuts = &_cutSections[0];

		udtParseArg info;
		memset(&info, 0, sizeof(info));
		info.MessageCb = &MessageCallback;

		const s32 errorCode = udtCutDemoFileByTime(_context, &info, &arg, filePath);
		if(errorCode != udtErrorCode::None)
		{
			PrintError("udtCutDemoFileByTime failed: %s", udtGetErrorCodeString(errorCode));
			return false;
		}

		return true;
	}

private:
	MultiFragRailCutter(const MultiFragRailCutter&);
	void operator=(const MultiFragRailCutter&);

	bool AnalyzeDemoFile(const char* filePath)
	{
		FileReader reader;
		if(!reader.Open(filePath))
		{
			PrintError("Failed to open the file for reading: %s", filePath);
			return false;
		}

		udtCuContext* const cuContext = _cuContext;
		const u32 protocol = udtGetProtocolByFilePath(filePath);
		const bool writeEnabled = udtIsProtocolWriteSupported(protocol) != 0;
		if(!writeEnabled)
		{
			PrintWarning("UDT can't cut demos of this protocol, but the application will proceed with the analysis.");
		}

		_protocol = (udtProtocol::Id)protocol;
		_protocolWriteEnabled = writeEnabled;
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

	void AnalyzeMessage(udtCuMessageOutput& message)
	{
		if(message.IsGameState)
		{
			++_gameStateIndex;
			_trackedClientNumber = message.GameStateOrSnapshot.GameState->ClientNumber;
		}
		else if(message.GameStateOrSnapshot.Snapshot != NULL)
		{
			AnalyzeSnapshot(*message.GameStateOrSnapshot.Snapshot);
		}
	}
	
	void AnalyzeSnapshot(const udtCuSnapshotMessage& snapshot)
	{
		u32 railKillCount = 0;
		for(u32 i = 0; i < snapshot.ChangedEntityCount; ++i)
		{
			Obituary obituary;
			if(!IsObituaryEvent(obituary, *snapshot.ChangedEntities[i], _protocol))
			{
				continue;
			}

			if(obituary.AttackerIndex != _trackedClientNumber ||
			   obituary.TargetIndex == _trackedClientNumber)
			{
				continue;
			}

			if(obituary.MeanOfDeath == (s32)udtMeanOfDeath::Railgun)
			{
				++railKillCount;
			}
		}

		if(railKillCount >= 2)
		{
			const u32 cutIndex = (u32)_cutSections.size();

			// Create a 20 seconds long demo cut to keep that sweet frag.
			udtCut cut;
			cut.GameStateIndex = _gameStateIndex;
			cut.StartTimeMs = snapshot.ServerTimeMs - (s32)CUT_START_OFFSET_SEC * (s32)1000;
			cut.EndTimeMs = snapshot.ServerTimeMs + (s32)CUT_END_OFFSET_SEC * (s32)1000;
			cut.FilePath = NULL; // Will be fixed up later.
			_cutSections.push_back(cut);

			std::string outputFilePath;
			CreateOutputFilePath(outputFilePath, cutIndex, railKillCount);
			_cutFilePaths.push_back(outputFilePath);
		}
	}

	void CreateOutputFilePath(std::string& outputFilePath, u32 cutIndex, u32 railKillCount)
	{
		const char* const extension = udtGetFileExtensionByProtocol((u32)_protocol);

		// Default value in case something goes wrong...
		char output[512];
		sprintf(output, "cut_#%u_%s_rail_%s", cutIndex + 1, GetTupleNameByLength(railKillCount), extension);
		outputFilePath = output;

		const size_t extIdx = _inputFilePath.rfind('.');
		if(extIdx == std::string::npos)
		{
			// We somehow couldn't find the extension of this demo file's path.
			return;
		}

		const std::string filePathNoExt = _inputFilePath.substr(0, extIdx);
		sprintf(output, "%s_cut_#%u_%s_rail_%s", filePathNoExt.c_str(), cutIndex + 1, GetTupleNameByLength(railKillCount), extension);
		outputFilePath = output;
	}
	
	std::vector<udtCut> _cutSections;
	std::vector<std::string> _cutFilePaths;
	std::string _inputFilePath;
	udtCuContext* _cuContext;
	udtParserContext* _context;
	u8* _inMsgData;
	s32 _gameStateIndex;
	s32 _trackedClientNumber;
	udtProtocol::Id _protocol;
	bool _protocolWriteEnabled;
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

	MultiFragRailCutter cutter;
	if(cutter.Init())
	{
		cutter.ProcessDemoFile(argv[1]);
	}

	udtShutDownLibrary();

	return 0;
}
