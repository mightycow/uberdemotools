#include "shared.hpp"
#include "stack_trace.hpp"
#include "path.hpp"
#include "file_system.hpp"
#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>


void CrashHandler(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, message);
	fprintf(stderr, "\n");

	PrintStackTrace(3, "UDT_json");

	exit(666);
}

static void PrintHelp()
{
	printf("???? help for UDT_json ????\n");
	printf("UDT_json demo_file   [options] [-o=output_folder]\n");
	printf("UDT_json demo_folder [options] [-o=output_folder]\n");
	printf("If the output_folder isn't provided, the .json file will be output in the same directory as the input file with the same name.\n");
	printf("Options: \n");
	printf("-c        : output to the console/terminal    (default: off)\n");
	printf("-r        : enable recursive demo file search (default: off)\n");
	printf("-t=max_t  : maximum number of threads         (default: 4)\n");
	printf("-a=<flags>: select analyzers                  (default: all enabled)\n");
	printf("            g: Game states\n");
	printf("            m: chat Messages\n");
	printf("            d: Deaths\n");
	printf("            s: Stats\n");
	printf("            r: Raw commands\n");
	printf("            c: raw Config strings\n");
}

static bool KeepOnlyDemoFiles(const char* name, u64 /*size*/)
{
	return udtPath::HasValidDemoFileExtension(name);
}

static void RegisterAnalyzer(u32* analyzers, u32& analyzerCount, udtParserPlugIn::Id analyzerId)
{
	for(u32 i = 0; i < analyzerCount; ++i)
	{
		if(analyzers[i] == (u32)analyzerId)
		{
			// We already have this registered.
			return;
		}
	}

	analyzers[analyzerCount++] = (u32)analyzerId;
}

static bool ProcessMultipleDemos(const udtFileInfo* files, u32 fileCount, const char* customOutputFolder, bool consoleOutput, u32 maxThreadCount, const u32* plugInIds, u32 plugInCount)
{
	udtVMArrayWithAlloc<const char*> filePaths(1 << 16, "ProcessMultipleDemos::FilePathsArray");
	udtVMArrayWithAlloc<s32> errorCodes(1 << 16, "ProcessMultipleDemos::ErrorCodesArray");
	filePaths.Resize(fileCount);
	errorCodes.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		filePaths[i] = files[i].Path;
	}

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	s32 cancelOperation = 0;
	info.CancelOperation = &cancelOperation;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;
	info.OutputFolderPath = customOutputFolder;
	info.PlugIns = plugInIds;
	info.PlugInCount = plugInCount;

	udtMultiParseArg threadInfo;
	memset(&threadInfo, 0, sizeof(threadInfo));
	threadInfo.FilePaths = filePaths.GetStartAddress();
	threadInfo.OutputErrorCodes = errorCodes.GetStartAddress();
	threadInfo.FileCount = fileCount;
	threadInfo.MaxThreadCount = maxThreadCount;

	udtJSONArg jsonInfo;
	memset(&jsonInfo, 0, sizeof(jsonInfo));
	jsonInfo.ConsoleOutput = consoleOutput ? 1 : 0;

	const s32 result = udtSaveDemoFilesAnalysisDataToJSON(&info, &threadInfo, &jsonInfo);

	udtVMLinearAllocator tempAllocator;
	tempAllocator.Init(1 << 16, "ProcessMultipleDemos::Temp");
	for(u32 i = 0; i < fileCount; ++i)
	{
		if(errorCodes[i] != (s32)udtErrorCode::None)
		{
			udtString fileName;
			tempAllocator.Clear();
			udtPath::GetFileName(fileName, tempAllocator, udtString::NewConstRef(filePaths[i]));

			fprintf(stderr, "Processing of file %s failed with error: %s\n", fileName.String != NULL ? fileName.String : "?", udtGetErrorCodeString(errorCodes[i]));
		}
	}

	if(result == udtErrorCode::None)
	{
		return true;
	}

	fprintf(stderr, "udtSaveDemoFilesAnalysisDataToJSON failed with error: %s\n", udtGetErrorCodeString(result));

	return false;
}

int udt_main(int argc, char** argv)
{
	if(argc < 2)
	{
		PrintHelp();
		return __LINE__;
	}

	bool fileMode = false;
	if(udtFileStream::Exists(argv[1]) && udtPath::HasValidDemoFileExtension(argv[1]))
	{
		fileMode = true;
	}
	else if(!IsValidDirectory(argv[1]))
	{
		fprintf(stderr, "Invalid file/folder path.\n");
		PrintHelp();
		return __LINE__;
	}

	const char* customOutputPath = NULL;
	u32 maxThreadCount = 4;
	u32 analyzerCount = (u32)udtParserPlugIn::Count;
	u32 analyzers[udtParserPlugIn::Count];
	bool recursive = false;
	bool consoleOutput = false;

	for(u32 i = 0; i < (u32)udtParserPlugIn::Count; ++i)
	{
		analyzers[i] = i;
	}

	for(int i = 2; i < argc; ++i)
	{
		s32 localMaxThreads = 4;

		const udtString arg = udtString::NewConstRef(argv[i]);
		if(udtString::Equals(arg, "-r"))
		{
			recursive = true;
		}
		else if(udtString::Equals(arg, "-c"))
		{
			consoleOutput = true;
		}
		else if(udtString::StartsWith(arg, "-o=") && 
				arg.Length >= 4 &&
				IsValidDirectory(argv[i] + 3))
		{
			customOutputPath = argv[i] + 3;
		}
		else if(udtString::StartsWith(arg, "-t=") && 
				arg.Length >= 4 && 
				StringParseInt(localMaxThreads, arg.String + 3) &&
				localMaxThreads >= 1 &&
				localMaxThreads <= 16)
		{
			maxThreadCount = (u32)localMaxThreads;
		}
		else if(udtString::StartsWith(arg, "-a=") &&
				arg.Length >= 4)
		{
			analyzerCount = 0;
			const char* s = argv[i] + 3;
			while(*s)
			{
				switch(*s)
				{
					case 'g': RegisterAnalyzer(analyzers, analyzerCount, udtParserPlugIn::GameState); break;
					case 'm': RegisterAnalyzer(analyzers, analyzerCount, udtParserPlugIn::Chat); break;
					case 'd': RegisterAnalyzer(analyzers, analyzerCount, udtParserPlugIn::Obituaries); break;
					case 's': RegisterAnalyzer(analyzers, analyzerCount, udtParserPlugIn::Stats); break;
					case 'r': RegisterAnalyzer(analyzers, analyzerCount, udtParserPlugIn::RawCommands); break;
					case 'c': RegisterAnalyzer(analyzers, analyzerCount, udtParserPlugIn::RawConfigStrings); break;
				}

				++s;
			}
		}
	}

	if(fileMode)
	{
		udtFileInfo fileInfo;
		fileInfo.Name = NULL;
		fileInfo.Size = 0;
		fileInfo.Path = argv[1];

		return ProcessMultipleDemos(&fileInfo, 1, customOutputPath, consoleOutput, maxThreadCount, analyzers, analyzerCount) ? 0 : __LINE__;
	}

	udtVMArrayWithAlloc<udtFileInfo> files(1 << 16, "udt_main::FilesArray");
	udtVMLinearAllocator persistAlloc;
	udtVMLinearAllocator tempAlloc;
	persistAlloc.Init(1 << 24, "udt_main::Persistent");
	tempAlloc.Init(1 << 24, "udt_main::Temp");

	udtFileListQuery query;
	memset(&query, 0, sizeof(query));
	query.FileFilter = &KeepOnlyDemoFiles;
	query.Files = &files;
	query.FolderPath = argv[1];
	query.PersistAllocator = &persistAlloc;
	query.Recursive = recursive;
	query.TempAllocator = &tempAlloc;
	GetDirectoryFileList(query);

	if(!ProcessMultipleDemos(files.GetStartAddress(), files.GetSize(), customOutputPath, false, maxThreadCount, analyzers, analyzerCount))
	{
		return __LINE__;
	}

	return 0;
}
