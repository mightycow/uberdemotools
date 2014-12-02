#include "shared.hpp"
#include "utils.hpp"
#include "file_system.hpp"
#include "timer.hpp"
#include "api.h"

#include <stdio.h>


#define countof(array) (sizeof(array) / sizeof(array[0]))

static const u32 parseOptions[] = 
{ 
	udtParserPlugIn::Chat, 
	udtParserPlugIn::GameState 
};


static void PrintChat(udtParserContext* context, u32 demoIndex)
{
	void* buffer = NULL;
	u32 count = 0;
	if(udtGetDemoDataInfo(context, demoIndex, (u32)udtParserPlugIn::Chat, &buffer, &count) < 0)
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

static void PrintGameState(udtParserContext* context, u32 demoIndex)
{
	void* buffer = NULL;
	u32 count = 0;
	if(udtGetDemoDataInfo(context, demoIndex, (u32)udtParserPlugIn::GameState, &buffer, &count) < 0)
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
	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;
	info.PlugIns = parseOptions;
	info.PlugInCount = (u32)countof(parseOptions);

	udtParserContext* const context = udtCreateContext(NULL);
	if(context == NULL)
	{
		return;
	}

	if(udtParseDemoFile(context, &info, filePath) != udtErrorCode::None)
	{
		udtDestroyContext(context);
		return;
	}

	PrintChat(context, 0);
	PrintGameState(context, 0);

	udtDestroyContext(context);
}

static void TestAddOnsThreaded(const udtVMArray<udtFileInfo>& files)
{
	const u32 fileCount = files.GetSize();
	udtVMArray<const char*> filePaths;
	filePaths.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		filePaths[i] = files[i].Path;
	}

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;
	info.PlugIns = parseOptions;
	info.PlugInCount = (u32)countof(parseOptions);

	udtMultiParseArg extraInfo;
	memset(&extraInfo, 0, sizeof(extraInfo));
	extraInfo.FilePaths = filePaths.GetStartAddress();
	extraInfo.FileCount = fileCount;
	extraInfo.MaxThreadCount = 4;

	udtTimer timer;
	timer.Start();
	udtParserContextGroup* contextGroup = NULL;
	if(udtParseDemoFiles(&contextGroup, &info, &extraInfo) != udtErrorCode::None)
	{
		udtDestroyContextGroup(contextGroup);
		return;
	}
	timer.Stop();

	u64 totalByteCount = 0;
	for(u32 i = 0, count = files.GetSize(); i < count; ++i)
	{
		totalByteCount += files[i].Size;
	}
	printf("Batch processing time: %d ms\n", (int)timer.GetElapsedMs());
	const f64 elapsedSec = (f64)timer.GetElapsedMs() / 1000.0;
	const f64 megs = (f64)totalByteCount / (f64)(1 << 20);
	printf("Throughput: %.1f MB/s\n", (float)(megs / elapsedSec));

	u32 contextCount = 0;
	if(udtGetContextCountFromGroup(contextGroup, &contextCount) != udtErrorCode::None)
	{
		udtDestroyContextGroup(contextGroup);
		return;
	}

	for(u32 i = 0; i < contextCount; ++i)
	{
		udtParserContext* context;
		if(udtGetContextFromGroup(contextGroup, i, &context) != udtErrorCode::None)
		{
			continue;
		}

		u32 demoCount = 0;
		if(udtGetDemoCountFromContext(context, &demoCount) != udtErrorCode::None)
		{
			continue;
		}

		for(u32 j = 0; j < demoCount; ++j)
		{
			printf("\n");
			printf("Demo %d of %d (context %d of %d)\n", (int)(j + 1), (int)demoCount, (int)(i + 1), (int)contextCount);
			printf("\n");
			PauseConsoleApp();

			PrintChat(context, j);
			PrintGameState(context, j);
		}
	}

	udtDestroyContextGroup(contextGroup);
}

bool KeepOnlyDemoFiles(const char* name, u64 /*size*/)
{
	return StringHasValidDemoFileExtension(name);
}

int main(int argc, char** argv)
{
	ResetCurrentDirectory(argv[0]);

	if(argc < 2)
	{
		printf("No file path given.\n");
		return 1;
	}

	if(udtFileStream::Exists(argv[1]) && StringHasValidDemoFileExtension(argv[1]))
	{
		TestAddOns(argv[1]);
	}
	else if(IsValidDirectory(argv[1]))
	{
		udtVMArray<udtFileInfo> files;
		udtVMLinearAllocator persistAlloc;
		udtVMLinearAllocator tempAlloc;
		persistAlloc.Init(1 << 24, 4096);
		tempAlloc.Init(1 << 24, 4096);

		udtFileListQuery query;
		memset(&query, 0, sizeof(query));
		query.FileFilter = &KeepOnlyDemoFiles;
		query.Files = &files;
		query.FolderPath = argv[1];
		query.PersistAllocator = &persistAlloc;
		query.Recursive = false;
		query.TempAllocator = &tempAlloc;
		GetDirectoryFileList(query);

		TestAddOnsThreaded(files);
	}
	else
	{
		printf("Invalid file/folder path: '%s'\n", argv[1]);
		return 2;
	}

	PauseConsoleApp();

	return 0;
}
