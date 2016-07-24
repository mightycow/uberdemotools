#include "parser.hpp"
#include "shared.hpp"
#include "stack_trace.hpp"
#include "utils.hpp"
#include "file_system.hpp"
#include "path.hpp"
#include "batch_runner.hpp"

#include <stdio.h>
#include <stdlib.h>


#define    UDT_CONVERTER_BATCH_SIZE    256


void PrintHelp()
{
	printf("Converts demos from one protocol to another.\n");
	printf("\n");
	printf("UDT_converter [-o=outputfolder] [-q] [-t=maxthreads] [-r] -p=protocol inputfile|inputfolder\n");
	printf("\n");
	printf("-q    quiet mode: no logging to stdout        (default: off)\n");
	printf("-o=p  set the output folder path to p         (default: the input's folder)\n");
	printf("-r    enable recursive demo file search       (default: off)\n");
	printf("-t=N  set the maximum number of threads to N  (default: 1)\n");
	printf("-p=N  set the output protocol version to N\n");
	printf("        N=68  output to .dm_68 files\n");
	printf("              supported input: .dm3 and .dm_48\n");
	printf("        N=91  output to .dm_91 files\n");
	printf("              supported input: .dm_73 and .dm_90\n");
}

static bool IsValidConversion(udtProtocol::Id input, udtProtocol::Id output)
{
	if((output == udtProtocol::Dm91 && input == udtProtocol::Dm73) ||
	   (output == udtProtocol::Dm91 && input == udtProtocol::Dm90) ||
	   (output == udtProtocol::Dm68 && input == udtProtocol::Dm3) ||
	   (output == udtProtocol::Dm68 && input == udtProtocol::Dm48))
	{
		return true;
	}

	return false;
}

struct Config
{
	Config()
	{
		CustomOutputFolder = NULL;
		MaxThreadCount = 1;
		OutputProtocol = udtProtocol::Invalid;
	}

	const char* CustomOutputFolder;
	u32 MaxThreadCount;
	udtProtocol::Id OutputProtocol;
};

static bool ConvertDemoBatch(udtParseArg& parseArg, const udtFileInfo* files, u32 fileCount, const Config& config)
{
	udtVMArray<const char*> filePaths("ConvertDemoBatch::FilePathsArray");
	udtVMArray<s32> errorCodes("ConvertDemoBatch::ErrorCodesArray");
	filePaths.Resize(fileCount);
	errorCodes.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		filePaths[i] = files[i].Path.GetPtr();
	}

	udtMultiParseArg threadInfo;
	memset(&threadInfo, 0, sizeof(threadInfo));
	threadInfo.FilePaths = filePaths.GetStartAddress();
	threadInfo.OutputErrorCodes = errorCodes.GetStartAddress();
	threadInfo.FileCount = fileCount;
	threadInfo.MaxThreadCount = config.MaxThreadCount;

	udtProtocolConversionArg conversionArg;
	memset(&conversionArg, 0, sizeof(conversionArg));
	conversionArg.OutputProtocol = (u32)config.OutputProtocol;

	const s32 result = udtConvertDemoFiles(&parseArg, &threadInfo, &conversionArg);

	udtVMLinearAllocator tempAllocator("ConvertDemoBatch::Temp");
	for(u32 i = 0; i < fileCount; ++i)
	{
		if(errorCodes[i] != (s32)udtErrorCode::None)
		{
			udtString fileName;
			tempAllocator.Clear();
			udtPath::GetFileName(fileName, tempAllocator, udtString::NewConstRef(filePaths[i]));

			fprintf(stderr, "Processing of file %s failed with error: %s\n", fileName.GetPtrSafe("?"), udtGetErrorCodeString(errorCodes[i]));
		}
	}

	if(result == udtErrorCode::None)
	{
		return true;
	}

	fprintf(stderr, "udtConvertDemoFiles failed with error: %s\n", udtGetErrorCodeString(result));

	return false;
}

static bool ConvertMultipleDemos(const udtFileInfo* files, u32 fileCount, const Config& config)
{
	CmdLineParseArg cmdLineParseArg;
	udtParseArg& parseArg = cmdLineParseArg.ParseArg;
	parseArg.OutputFolderPath = config.CustomOutputFolder;

	BatchRunner runner(parseArg, files, fileCount, UDT_CONVERTER_BATCH_SIZE);
	const u32 batchCount = runner.GetBatchCount();
	for(u32 i = 0; i < batchCount; ++i)
	{
		runner.PrepareNextBatch();
		const BatchRunner::BatchInfo& info = runner.GetBatchInfo(i);
		if(!ConvertDemoBatch(parseArg, files + info.FirstFileIndex, info.FileCount, config))
		{
			return false;
		}
	}

	return true;
}

static bool KeepOnlyCompatibleDemoFiles(const char* name, u64 /*size*/, void* userData)
{
	const Config& config = *(const Config*)userData;

	return IsValidConversion((udtProtocol::Id)udtGetProtocolByFilePath(name), config.OutputProtocol);
}

int udt_main(int argc, char** argv)
{
	if(argc == 1)
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

	bool recursive = false;
	Config config;
	for(int i = 1; i < argc - 1; ++i)
	{
		s32 localMaxThreads = 1;
		s32 localProtocol = (s32)udtProtocol::Invalid;
		const udtString arg = udtString::NewConstRef(argv[i]);
		if(udtString::StartsWith(arg, "-p=") &&
		   arg.GetLength() >= 4 &&
		   StringParseInt(localProtocol, arg.GetPtr() + 3))
		{
			if(localProtocol == 68)
			{
				config.OutputProtocol = udtProtocol::Dm68;
			}
			else if(localProtocol == 91)
			{
				config.OutputProtocol = udtProtocol::Dm91;
			}
		}
		else if(udtString::Equals(arg, "-r"))
		{
			recursive = true;
		}
		else if(udtString::StartsWith(arg, "-o=") &&
				arg.GetLength() >= 4 &&
				IsValidDirectory(argv[i] + 3))
		{
			config.CustomOutputFolder = argv[i] + 3;
		}
		else if(udtString::StartsWith(arg, "-t=") &&
				arg.GetLength() >= 4 &&
				StringParseInt(localMaxThreads, arg.GetPtr() + 3) &&
				localMaxThreads >= 1 &&
				localMaxThreads <= 16)
		{
			config.MaxThreadCount = (u32)localMaxThreads;
		}
	}

	if(config.OutputProtocol == udtProtocol::Invalid)
	{
		fprintf(stderr, "Invalid or unspecified output protocol number.\n");
		return 1;
	}

	if(fileMode)
	{
		if(!IsValidConversion((udtProtocol::Id)udtGetProtocolByFilePath(inputPath), config.OutputProtocol))
		{
			fprintf(stderr, "Unsupported conversion.\n");
			return 1;
		}

		udtFileInfo fileInfo;
		fileInfo.Name = udtString::NewNull();
		fileInfo.Path = udtString::NewConstRef(inputPath);
		fileInfo.Size = 0;

		return ConvertMultipleDemos(&fileInfo, 1, config) ? 0 : 1;
	}

	udtFileListQuery query;
	query.FileFilter = &KeepOnlyCompatibleDemoFiles;
	query.FolderPath = udtString::NewConstRef(inputPath);
	query.Recursive = recursive;
	query.UserData = &config;
	GetDirectoryFileList(query);
	if(query.Files.IsEmpty())
	{
		fprintf(stderr, "No compatible demo file found.\n");
		return 1;
	}

	return ConvertMultipleDemos(query.Files.GetStartAddress(), query.Files.GetSize(), config) ? 0 : 1;
}
