#include "shared.hpp"
#include "utils.hpp"
#include "api.h"

#include <stdio.h>


#define countof(array) (sizeof(array) / sizeof(array[0]))

static const u32 parseOptions[] = 
{ 
	udtParserPlugIn::Chat, 
	udtParserPlugIn::GameState 
};


static void PrintChat(udtParserContext* context)
{
	void* buffer = NULL;
	u32 count = 0;
	if(udtGetDemoDataInfo(context, (u32)udtParserPlugIn::Chat, &buffer, &count) < 0)
	{
		return;
	}

	if(buffer == NULL || count == 0)
	{
		printf("No chat messages!\n");
		return;
	}

	const udtParseDataChat* const chatEvents = (const udtParseDataChat*)buffer;
	bool formatQL = true;
	if(chatEvents[0].PlayerIndex < 0 || chatEvents[0].Strings[0].ClanName == NULL)
	{
		formatQL = false;
	}

	if(formatQL)
	{
		for(u32 i = 0; i < count; ++i)
		{
			//printf("ORIGINAL: %s\n", chatEvents[i].Strings[1].OriginalCommand);
			printf("%d: <%d> [%s] %s: %s\n",
				   chatEvents[i].ServerTimeMs / 1000,
				   chatEvents[i].PlayerIndex,
				   chatEvents[i].Strings[1].ClanName,
				   chatEvents[i].Strings[1].PlayerName,
				   chatEvents[i].Strings[1].Message);
		}
	}
	else
	{
		for(u32 i = 0; i < count; ++i)
		{
			//printf("ORIGINAL: %s\n", chatEvents[i].Strings[1].OriginalCommand);
			printf("%d: %s: %s\n",
				   chatEvents[i].ServerTimeMs / 1000,
				   chatEvents[i].Strings[1].PlayerName,
				   chatEvents[i].Strings[1].Message);
		}
	}
}

static void PrintGameState(udtParserContext* context)
{
	void* buffer = NULL;
	u32 count = 0;
	if(udtGetDemoDataInfo(context, (u32)udtParserPlugIn::GameState, &buffer, &count) < 0)
	{
		return;
	}

	if(buffer == NULL || count == 0)
	{
		return;
	}

	const udtParseDataGameState* const gameStates = (const udtParseDataGameState*)buffer;
	for(u32 i = 0; i < count; ++i)
	{
		printf("\n");
		printf("Game state %u\n", i + 1);
		printf("First snapshot: %d\n", gameStates[i].FirstSnapshotTimeMs / 1000);
		printf("Last snapshot: %d\n", gameStates[i].LastSnapshotTimeMs / 1000);
		for(u32 j = 0; j < gameStates[i].MatchCount; ++j)
		{
			printf("\n");
			printf("Match %u\n", j + 1);
			printf("Warm up end: %d\n", gameStates[i].Matches[j].WarmUpEndTimeMs / 1000);
			printf("Start: %d\n", gameStates[i].Matches[j].MatchStartTimeMs / 1000);
			printf("End: %d\n", gameStates[i].Matches[j].MatchEndTimeMs / 1000);
		}
	}
}

static void TestAddOns(const char* filePath)
{
	udtFileParseArg info;
	memset(&info, 0, sizeof(info));
	info.FilePath = filePath;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = NULL;

	PauseConsoleApp();

	udtParserContext* const context = udtCreateContext();
	if(context == NULL)
	{
		return;
	}

	udtParseDemo(context, &info, parseOptions, (u32)countof(parseOptions));

	PrintChat(context);
	PrintGameState(context);

	udtDestroyContext(context);
}

int main(int argc, char** argv)
{
	ResetCurrentDirectory(argv[0]);

	if(argc < 2)
	{
		printf("No file path given.\n");
		return 1;
	}

	if(!udtFileStream::Exists(argv[1]) || !StringHasValidDemoFileExtension(argv[1]))
	{
		printf("Invalid file path: '%s'\n", argv[1]);
		return 2;
	}

	TestAddOns(argv[1]);

	PauseConsoleApp();

	return 0;
}
