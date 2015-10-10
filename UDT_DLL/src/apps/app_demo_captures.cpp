// @TODO: Get rid of the STL code...
#define _HAS_EXCEPTIONS 0

#include "shared.hpp"
#include "stack_trace.hpp"
#include "path.hpp"
#include "file_system.hpp"
#include "utils.hpp"
#include "parser_context.hpp"
#include "json_writer.hpp"

#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>


struct CaptureInfo
{
	// @TODO: Get rid of the STL code...
	std::string FilePath;
	std::string FileName;
	std::string MapName;
	s32 GameStateIndex;
	s32 PickUpTimeMs;
	s32 CaptureTimeMs;
	f32 Distance;
	s32 DurationMs;
	f32 Speed;
	u32 Index; // Used for stable sorting.
	bool BaseToBase;
};

struct Worker
{
public:
	Worker()
	{
		_maxThreadCount = 4;
	}

	bool ProcessDemos(const udtFileInfo* files, u32 fileCount, const char* outputFilePath, u32 maxThreadCount)
	{
		_maxThreadCount = maxThreadCount;

		udtFileStream jsonFile;
		if(!jsonFile.Open(outputFilePath, udtFileOpenMode::Write))
		{
			fprintf(stderr, "Failed to open output file path for writing.\n");
			return false;
		}

		udtParserContext* const context = udtCreateContext();
		if(context == NULL)
		{
			fprintf(stderr, "Failed to create a parser context.\n");
			return false;
		}

		const u32 batchSize = 256;
		const u32 batchCount = (fileCount + batchSize - 1) / batchSize;
		u32 fileOffset = 0;
		for(u32 i = 0; i < batchCount; ++i)
		{
			u32 batchFileCount = batchSize;
			if(i == batchCount - 1)
			{
				batchFileCount = fileCount % batchSize;
			}

			ProcessBatch(files + fileOffset, batchFileCount);

			fileOffset += batchFileCount;
		}

		SortCaptures();

		context->JSONWriterContext.ResetForNextDemo();
		udtJSONWriter& writer = context->JSONWriterContext.Writer;
		writer.StartFile();
		WriteCaptures(writer);
		writer.EndFile();

		udtVMMemoryStream& memoryStream = context->JSONWriterContext.MemoryStream;
		if(jsonFile.Write(memoryStream.GetBuffer(), (u32)memoryStream.Length(), 1) != 1)
		{
			fprintf(stderr, "Failed to write to the output file.\n");
			udtDestroyContext(context);
			return false;
		}

		udtDestroyContext(context);

		return true;
	}

private:
	void ProcessBatch(const udtFileInfo* files, u32 fileCount)
	{
		udtVMArrayWithAlloc<const char*> filePaths(1 << 16, "Worker::ProcessBatch::FilePathsArray");
		udtVMArrayWithAlloc<s32> errorCodes(1 << 16, "Worker::ProcessBatch::ErrorCodesArray");
		filePaths.Resize(fileCount);
		errorCodes.Resize(fileCount);
		for(u32 i = 0; i < fileCount; ++i)
		{
			filePaths[i] = files[i].Path;
		}

		const u32 plugInId = (u32)udtParserPlugIn::Captures;

		udtParseArg info;
		memset(&info, 0, sizeof(info));
		s32 cancelOperation = 0;
		info.CancelOperation = &cancelOperation;
		info.MessageCb = &CallbackConsoleMessage;
		info.ProgressCb = &CallbackConsoleProgress;
		info.OutputFolderPath = NULL;
		info.PlugIns = &plugInId;
		info.PlugInCount = 1;

		udtMultiParseArg threadInfo;
		memset(&threadInfo, 0, sizeof(threadInfo));
		threadInfo.FilePaths = filePaths.GetStartAddress();
		threadInfo.OutputErrorCodes = errorCodes.GetStartAddress();
		threadInfo.FileCount = fileCount;
		threadInfo.MaxThreadCount = _maxThreadCount;

		udtParserContextGroup* contextGroup = NULL;
		const s32 result = udtParseDemoFiles(&contextGroup, &info, &threadInfo);
		if(result != (s32)udtErrorCode::None)
		{
			fprintf(stderr, "udtParseDemoFiles failed with error: %s\n", udtGetErrorCodeString(result));
			if(contextGroup != NULL)
			{
				udtDestroyContextGroup(contextGroup);
			}
			return;
		}

		ProcessCaptures(contextGroup, files);
		udtDestroyContextGroup(contextGroup);
	}

	void ProcessCaptures(udtParserContextGroup* contextGroup, const udtFileInfo* files)
	{
		u32 contextCount = 0;
		udtGetContextCountFromGroup(contextGroup, &contextCount);
		for(u32 contextIdx = 0; contextIdx < contextCount; ++contextIdx)
		{
			udtParserContext* context = NULL;
			u32 demoCount = 0;
			udtGetContextFromGroup(contextGroup, contextIdx, &context);
			udtGetDemoCountFromContext(context, &demoCount);
			for(u32 demoIdx = 0; demoIdx < demoCount; ++demoIdx)
			{
				u32 demoInputIdx = 0;
				udtGetDemoInputIndex(context, demoIdx, &demoInputIdx);

				void* capturesPointer = NULL;
				u32 captureCount = 0;
				if(udtGetDemoDataInfo(context, demoIdx, (u32)udtParserPlugIn::Captures, &capturesPointer, &captureCount) == (s32)udtErrorCode::None &&
				   capturesPointer != NULL)
				{
					ProcessCaptures((const udtParseDataCapture*)capturesPointer, captureCount, files[demoInputIdx].Path, files[demoInputIdx].Name);
				}
			}
		}
	}

	void ProcessCaptures(const udtParseDataCapture* captures, u32 captureCount, const char* filePath, const char* fileName)
	{
		for(u32 i = 0; i < captureCount; ++i)
		{
			const udtParseDataCapture& capture = captures[i];
			if((capture.Flags & (u32)udtParseDataCaptureFlags::DemoTaker) == 0)
			{
				continue;
			}

			const s32 durationMs = capture.CaptureTimeMs - capture.PickUpTimeMs;

			CaptureInfo cap;
			cap.BaseToBase = (capture.Flags & (u32)udtParseDataCaptureFlags::BaseToBase) != 0;
			cap.CaptureTimeMs = capture.CaptureTimeMs;
			cap.Distance = capture.Distance;
			cap.DurationMs = durationMs;
			cap.FileName = fileName;
			cap.FilePath = filePath;
			cap.GameStateIndex = capture.GameStateIndex;
			cap.Index = (u32)_captures.size();
			cap.MapName = capture.MapName;
			cap.PickUpTimeMs = capture.PickUpTimeMs;
			cap.Speed = durationMs == 0 ? FLT_MAX : capture.Distance / ((f32)durationMs / 1000.0f);
			_captures.push_back(cap);
		}
	}

	void WriteCaptures(udtJSONWriter& writer)
	{
		writer.StartArray("captures");

		// @TODO: Print by map+type
		// Example: oj5 base2base, oj5 missing2base, oj9 base2base, etc

		for(u32 i = 0, count = (u32)_captures.size(); i < count; ++i)
		{
			writer.StartObject();

			const CaptureInfo& cap = _captures[i];
			writer.WriteStringValue("filePath", cap.FilePath.c_str());

			writer.EndObject();
		}

		writer.EndArray();
	}

	void SortCaptures()
	{
		//@TODO:
	}

	u32 _maxThreadCount;
	bool _ownDemosOnly;

	std::vector<CaptureInfo> _captures;
};


void CrashHandler(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, message);
	fprintf(stderr, "\n");

	PrintStackTrace(3, "UDT_captures");

	exit(666);
}

static void PrintHelp()
{
	printf("???? help for UDT_captures ????\n");
	printf("UDT_captures [options] -o=output_file demo_folder\n");
	printf("Options:\n");
	printf("-r      : recursive demo file search             (default: off)\n");
	printf("-t=max_t: maximum number of threads              (default: 4)\n");
}

static bool KeepOnlyDemoFiles(const char* name, u64 /*size*/)
{
	return udtPath::HasValidDemoFileExtension(name);
}

int udt_main(int argc, char** argv)
{
	if(argc < 3)
	{
		PrintHelp();
		return 1;
	}

	const char* const directoryPath = argv[argc - 1];
	if(!IsValidDirectory(directoryPath))
	{
		fprintf(stderr, "Invalid directory path.\n");
		PrintHelp();
		return 1;
	}

	const char* outputFilePath = NULL;
	u32 maxThreadCount = 4;
	bool recursive = false;
	for(int i = 1; i < argc - 1; ++i)
	{
		s32 localMaxThreads = 4;

		const udtString arg = udtString::NewConstRef(argv[i]);
		if(udtString::Equals(arg, "-r"))
		{
			recursive = true;
		}
		else if(udtString::StartsWith(arg, "-o=") &&
				arg.Length >= 4)
		{
			outputFilePath = argv[i] + 3;
		}
		else if(udtString::StartsWith(arg, "-t=") &&
				arg.Length >= 4 &&
				StringParseInt(localMaxThreads, arg.String + 3) &&
				localMaxThreads >= 1 &&
				localMaxThreads <= 16)
		{
			maxThreadCount = (u32)localMaxThreads;
		}
	}

	if(outputFilePath == NULL)
	{
		fprintf(stderr, "Output file path not specified or invalid.\n");
		PrintHelp();
		return 1;
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
	query.FolderPath = directoryPath;
	query.PersistAllocator = &persistAlloc;
	query.Recursive = recursive;
	query.TempAllocator = &tempAlloc;
	GetDirectoryFileList(query);

	if(files.GetSize() == 0)
	{
		return 0;
	}

	Worker worker;
	if(!worker.ProcessDemos(files.GetStartAddress(), files.GetSize(), outputFilePath, maxThreadCount))
	{
		return 2;
	}

	return 0;
}
