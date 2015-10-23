#include "shared.hpp"
#include "stack_trace.hpp"
#include "path.hpp"
#include "file_system.hpp"
#include "utils.hpp"
#include "batch_runner.hpp"

#include <stdio.h>
#include <stdlib.h>


#define    UDT_JSON_BATCH_SIZE    256


void PrintHelp()
{
	printf("For each input demo, outputs JSON data with analysis results to one file per demo or optionally to the terminal.\n");
	printf("\n");
	printf("UDT_json [-c] [-r] [-q] [-t=maxthreads] [-a=analyzers] [-o=outputfolder] inputfile|inputfolder\n");
	printf("\n");
	printf("-q    quiet mode: no logging to stdout        (default: off)\n");
	printf("-o=p  set the output folder path to p         (default: the input's folder)\n");
	printf("-c    output to the console/terminal          (default: off)\n");
	printf("-r    enable recursive demo file search       (default: off)\n");
	printf("-t=N  set the maximum number of threads to N  (default: 4)\n");
	printf("-a=   select analyzers                        (default: all enabled)\n");
	printf("        g: Game states         s: Stats\n");
	printf("        m: chat Messages       r: Raw commands\n");
	printf("        c: raw Config strings  f: Flag captures\n");
	printf("        d: Deaths\n");
	printf("\n");
	printf("The terminal output option -c will only be in effect when you specify the input as a file path.\n");
	printf("When active, the option will disable all stdout output that isn't the JSON data itself but stderr output will ");
	printf("still be active, so make sure you only read the stdout output from your programs and scripts.\n");
	printf("\n");
	printf("Example for selecting analyzers: -a=sd will select stats and deaths.\n");
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

static void CallbackConsoleMessageNoStdOut(s32 logLevel, const char* message)
{
	if(logLevel != 2 && logLevel != 3)
	{
		return;
	}

	fprintf(stderr, logLevel == 2 ? "Error: " : "Fatal: ");
	fprintf(stderr, message);
	fprintf(stderr, "\n");
}

static bool ProcessBatch(udtParseArg& parseArg, const udtFileInfo* files, u32 fileCount, bool consoleOutput, u32 maxThreadCount)
{
	udtVMArrayWithAlloc<const char*> filePaths(1 << 16, "ProcessMultipleDemos::FilePathsArray");
	udtVMArrayWithAlloc<s32> errorCodes(1 << 16, "ProcessMultipleDemos::ErrorCodesArray");
	filePaths.Resize(fileCount);
	errorCodes.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		filePaths[i] = files[i].Path;
	}

	if(consoleOutput)
	{
		parseArg.MessageCb = &CallbackConsoleMessageNoStdOut;
	}

	udtMultiParseArg threadInfo;
	memset(&threadInfo, 0, sizeof(threadInfo));
	threadInfo.FilePaths = filePaths.GetStartAddress();
	threadInfo.OutputErrorCodes = errorCodes.GetStartAddress();
	threadInfo.FileCount = fileCount;
	threadInfo.MaxThreadCount = maxThreadCount;

	udtJSONArg jsonInfo;
	memset(&jsonInfo, 0, sizeof(jsonInfo));
	jsonInfo.ConsoleOutput = consoleOutput ? 1 : 0;

	const s32 result = udtSaveDemoFilesAnalysisDataToJSON(&parseArg, &threadInfo, &jsonInfo);

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

static bool ProcessMultipleDemos(const udtFileInfo* files, u32 fileCount, const char* customOutputFolder, bool consoleOutput, u32 maxThreadCount, const u32* plugInIds, u32 plugInCount)
{
	CmdLineParseArg cmdLineParseArg;
	udtParseArg& parseArg = cmdLineParseArg.ParseArg;
	parseArg.PlugIns = plugInIds;
	parseArg.PlugInCount = plugInCount;
	parseArg.OutputFolderPath = customOutputFolder;

	BatchRunner runner(parseArg, files, fileCount, UDT_JSON_BATCH_SIZE);
	const u32 batchCount = runner.GetBatchCount();
	for(u32 i = 0; i < batchCount; ++i)
	{
		runner.PrepareNextBatch();
		const BatchRunner::BatchInfo& info = runner.GetBatchInfo(i);
		if(!ProcessBatch(parseArg, files + info.FirstFileIndex, info.FileCount, consoleOutput, maxThreadCount))
		{
			return false;
		}
	}

	return true;
}

int udt_main(int argc, char** argv)
{
	if(argc < 2)
	{
		PrintHelp();
		return 0;
	}

	bool fileMode = false;
	const char* const inputPath = argv[argc - 1];
	if(udtFileStream::Exists(inputPath) && udtPath::HasValidDemoFileExtension(inputPath))
	{
		fileMode = true;
	}
	else if(!IsValidDirectory(inputPath))
	{
		fprintf(stderr, "Invalid file/folder path.\n");
		return 1;
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

	for(int i = 1; i < argc - 1; ++i)
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
					case 'f': RegisterAnalyzer(analyzers, analyzerCount, udtParserPlugIn::Captures); break;
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
		fileInfo.Path = inputPath;

		return ProcessMultipleDemos(&fileInfo, 1, customOutputPath, consoleOutput, maxThreadCount, analyzers, analyzerCount) ? 0 : 1;
	}

	udtVMArrayWithAlloc<udtFileInfo> files(1 << 16, "udt_main::FilesArray");
	udtVMLinearAllocator persistAlloc;
	udtVMLinearAllocator folderArrayAlloc;
	udtVMLinearAllocator tempAlloc;
	persistAlloc.Init(1 << 24, "udt_main::Persistent");
	folderArrayAlloc.Init(1 << 24, "udt_main::FolderArray");
	tempAlloc.Init(1 << 20, "udt_main::Temp");

	udtFileListQuery query;
	memset(&query, 0, sizeof(query));
	query.FileFilter = &KeepOnlyDemoFiles;
	query.Files = &files;
	query.FolderArrayAllocator = &folderArrayAlloc;
	query.FolderPath = inputPath;
	query.PersistAllocator = &persistAlloc;
	query.Recursive = recursive;
	query.TempAllocator = &tempAlloc;
	GetDirectoryFileList(query);
	if(files.IsEmpty())
	{
		fprintf(stderr, "No demo file found.\n");
		return 1;
	}

	if(!ProcessMultipleDemos(files.GetStartAddress(), files.GetSize(), customOutputPath, false, maxThreadCount, analyzers, analyzerCount))
	{
		return 1;
	}

	return 0;
}
