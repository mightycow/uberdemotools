#include "parser.hpp"
#include "shared.hpp"
#include "stack_trace.hpp"
#include "utils.hpp"
#include "memory_stream.hpp"
#include "file_stream.hpp"
#include "json_writer.hpp"

#include <stdio.h>
#include <stdlib.h>


#define UDT_PLAYER_STATS_ITEM(Enum, Desc) Desc,
static const char* PlayerStatsFieldNames[udtPlayerStatsField::Count + 1]
{
	UDT_PLAYER_STATS_LIST(UDT_PLAYER_STATS_ITEM)
	""
};
#undef UDT_PLAYER_STATS_ITEM

#define UDT_TEAM_STATS_ITEM(Enum, Desc) Desc,
static const char* TeamStatsFieldNames[udtPlayerStatsField::Count + 1]
{
	UDT_TEAM_STATS_LIST(UDT_TEAM_STATS_ITEM)
	""
};
#undef UDT_TEAM_STATS_ITEM


static void CrashHandler(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, message);
	fprintf(stderr, "\n");

	PrintStackTrace(3, "UDT_json");

	exit(666);
}

static void PrintHelp()
{
	printf("\n");
	printf("UDT_json demo_path json_path\n");
}

static void WriteStats(udtJSONWriter& writer, const udtTeamStats& stats, const char* team)
{
	writer.StartObject();

	writer.WriteStringValue("team", team);
	for(s32 i = 0; i < (s32)udtTeamStatsField::Count; ++i)
	{
		if((stats.Flags & ((s64)1 << (s64)i)) != 0)
		{
			writer.WriteIntValue(TeamStatsFieldNames[i], stats.Fields[i]);
		}
	}

	writer.EndObject();
}

static void WriteStats(udtJSONWriter& writer, const udtPlayerStats& stats, const char* name, const char* cleanName)
{
	writer.StartObject();

	if(name != NULL)
	{
		writer.WriteStringValue("name", name);
	}

	if(cleanName != NULL)
	{
		writer.WriteStringValue("clean name", cleanName);
	}

	for(s32 i = 0; i < (s32)udtPlayerStatsField::Count; ++i)
	{
		if((stats.Flags & ((s64)1 << (s64)i)) != 0)
		{
			writer.WriteIntValue(PlayerStatsFieldNames[i], stats.Fields[i]);
		}
	}

	writer.EndObject();
}

static void WriteStats(udtJSONWriter& writer, const udtParseDataStats& stats)
{
	writer.StartObject("team stats");
	for(s32 i = 0; i < 2; ++i)
	{
		const udtTeamStats& teamStats = stats.TeamStats[i];
		if(teamStats.Flags != 0)
		{
			WriteStats(writer, teamStats, i == 0 ? "red" : "blue");
		}
	}
	writer.EndObject();
	
	writer.StartObject("player stats");
	for(s32 i = 0; i < 64; ++i)
	{
		const udtPlayerStats& playerStats = stats.PlayerStats[i];
		if(playerStats.Flags != 0)
		{
			WriteStats(writer, playerStats, playerStats.Name, playerStats.CleanName);
		}
	}
	writer.EndObject();
}

static int ProcessDemo(const char* demoPath, const char* jsonPath)
{
	s32 cancel = 0;
	s32 errorCode = 0;
	const u32 statsPlugIn = (u32)udtParserPlugIn::Stats;

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.CancelOperation = &cancel;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;
	info.PlugIns = &statsPlugIn;
	info.PlugInCount = 1;

	udtMultiParseArg extraInfo;
	memset(&extraInfo, 0, sizeof(extraInfo));
	extraInfo.FileCount = 1;
	extraInfo.FilePaths = &demoPath;
	extraInfo.MaxThreadCount = 1;
	extraInfo.OutputErrorCodes = &errorCode;

	udtParserContextGroup* contextGroup = NULL;
	if(udtParseDemoFiles(&contextGroup, &info, &extraInfo) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	u32 contextCount = 0;
	if(udtGetContextCountFromGroup(contextGroup, &contextCount) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	if(contextCount != 1)
	{
		return __LINE__;
	}

	udtParserContext* context = NULL;
	if(udtGetContextFromGroup(contextGroup, 0, &context) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	u32 demoCount = 0;
	if(udtGetDemoCountFromContext(context, &demoCount) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	if(demoCount != 1)
	{
		return __LINE__;
	}

	void* statsPointer = NULL;
	u32 statsCount = 0;
	if(udtGetDemoDataInfo(context, 0, (u32)udtParserPlugIn::Stats, &statsPointer, &statsCount) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	if(statsPointer == NULL)
	{
		return __LINE__;
	}

	udtVMMemoryStream memoryStream;
	if(!memoryStream.Open(1 << 20))
	{
		return __LINE__;
	}

	const udtParseDataStats* const stats = (const udtParseDataStats*)statsPointer;
	udtJSONWriter jsonWriter;
	jsonWriter.SetOutputStream(&memoryStream);
	jsonWriter.StartFile();
	for(u32 i = 0; i < statsCount; ++i)
	{
		jsonWriter.StartObject("game stats");
		WriteStats(jsonWriter, stats[i]);
		jsonWriter.EndObject();
	}
	jsonWriter.EndFile();

	udtFileStream jsonFile;
	if(!jsonFile.Open(jsonPath, udtFileOpenMode::Write))
	{
		return __LINE__;
	}

	if(jsonFile.Write(memoryStream.GetBuffer(), (u32)memoryStream.Length(), 1) != 1)
	{
		return __LINE__;
	}

	return 0;
}

int main(int argc, char** argv)
{
	if(argc < 3)
	{
		PrintHelp();
		return 1;
	}

	printf("UDT library version: %s\n", udtGetVersionString());

	ResetCurrentDirectory(argv[0]);
	udtSetCrashHandler(&CrashHandler);

	return ProcessDemo(argv[1], argv[2]);
}
