#include "shared.hpp"
#include "stack_trace.hpp"
#include "path.hpp"
#include "file_system.hpp"
#include "utils.hpp"
#include "parser_context.hpp"
#include "json_writer.hpp"
#include "batch_runner.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <float.h>


#define    UDT_CAPTURES_BATCH_SIZE    256


struct CaptureInfo
{
	const char* FilePath;
	const char* FileName;
	const char* MapName;
	s32 GameStateIndex;
	s32 PickUpTimeMs;
	s32 CaptureTimeMs;
	f32 Distance;
	s32 DurationMs;
	f32 Speed;
	s32 SortIndex; // Used for stable sorting.
	bool BaseToBase;
};


typedef s32 (*CaptureCompareFunc)(const CaptureInfo& a, const CaptureInfo& b);

template<CaptureCompareFunc CompareFunc>
static int GenericStableCaptureSort(const void* aPtr, const void* bPtr)
{
	const CaptureInfo& a = *(CaptureInfo*)aPtr;
	const CaptureInfo& b = *(CaptureInfo*)bPtr;

	const s32 result = (*CompareFunc)(a, b);
	if(result != 0)
	{
		return (int)result;
	}

	return (int)(a.SortIndex - b.SortIndex);
}

s32 SortByDurationAscending(const CaptureInfo& a, const CaptureInfo& b)
{
	return a.DurationMs - b.DurationMs;
}

s32 SortByMapNameAscending(const CaptureInfo& a, const CaptureInfo& b)
{
	return (s32)strcmp(a.MapName, b.MapName);
}

s32 SortByBaseToBaseTrueToFalse(const CaptureInfo& a, const CaptureInfo& b)
{
	return (s32)(!!b.BaseToBase - !!a.BaseToBase);
}

s32 SortBySpeedDescending(const CaptureInfo& a, const CaptureInfo& b)
{
	return (s32)(b.Speed - a.Speed);
}


struct Worker
{
public:
	Worker()
	{
		_maxThreadCount = 4;
		_topBaseToBaseCapCount = 3;

		_parseArg.SetSinglePlugIn(udtParserPlugIn::Captures);
	}

	bool ProcessDemos(const udtFileInfo* files, u32 fileCount, const char* outputFilePath, u32 maxThreadCount, u32 topBaseToBaseCapCount)
	{
		_maxThreadCount = maxThreadCount;
		_topBaseToBaseCapCount = topBaseToBaseCapCount;

		// Max demo count: 64k
		// Captures per demo: 64
		// String data per demo: file path + file name + map name = 640 bytes max
		const uptr maxDemoCount = uptr(1 << 16);
		_captures.Init((uptr)(maxDemoCount * (64 * sizeof(CaptureInfo))), "Worker::CapturesArray");
		_stringAllocator.Init((uptr)(maxDemoCount * 640), "Worker::String");

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

		BatchRunner runner(_parseArg.ParseArg, files, fileCount, UDT_CAPTURES_BATCH_SIZE);
		const u32 batchCount = runner.GetBatchCount();
		for(u32 i = 0; i < batchCount; ++i)
		{
			runner.PrepareNextBatch();
			const BatchRunner::BatchInfo& info = runner.GetBatchInfo(i);
			ProcessBatch(files + info.FirstFileIndex, info.FileCount);
		}

		if(_captures.IsEmpty())
		{
			fprintf(stderr, "Not a single capture was found.\n");
			return false;
		}

		SortCaptures();

		// @TODO: custom memory buffer size?
		context->JSONWriterContext.ResetForNextDemo();
		udtJSONWriter& writer = context->JSONWriterContext.Writer;
		writer.StartFile();
		if(topBaseToBaseCapCount > 0)
		{
			WriteFastestBaseToBaseCaptures(writer);
		}
		WriteAllCaptures(writer);
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

		udtMultiParseArg threadInfo;
		memset(&threadInfo, 0, sizeof(threadInfo));
		threadInfo.FilePaths = filePaths.GetStartAddress();
		threadInfo.OutputErrorCodes = errorCodes.GetStartAddress();
		threadInfo.FileCount = fileCount;
		threadInfo.MaxThreadCount = _maxThreadCount;

		udtParserContextGroup* contextGroup = NULL;
		const s32 result = udtParseDemoFiles(&contextGroup, &_parseArg.ParseArg, &threadInfo);
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

			udtString mapName = udtString::NewClone(_stringAllocator, capture.MapName);
			udtString::MakeLowerCase(mapName);

			CaptureInfo cap;
			cap.BaseToBase = (capture.Flags & (u32)udtParseDataCaptureFlags::BaseToBase) != 0;
			cap.CaptureTimeMs = capture.CaptureTimeMs;
			cap.Distance = capture.Distance;
			cap.DurationMs = durationMs;
			cap.FileName = udtString::NewClone(_stringAllocator, fileName).String;
			cap.FilePath = udtString::NewClone(_stringAllocator, filePath).String;
			cap.GameStateIndex = capture.GameStateIndex;
			cap.SortIndex = (s32)i;
			cap.MapName = mapName.String;
			cap.PickUpTimeMs = capture.PickUpTimeMs;
			cap.Speed = durationMs == 0 ? FLT_MAX : capture.Distance / ((f32)durationMs / 1000.0f);
			_captures.Add(cap);
		}
	}

	void WriteAllCaptures(udtJSONWriter& writer)
	{
		writer.StartArray("allCaptures");

		const char* previousMap = "__invalid__";
		bool previousBaseToBase = false;

		const u32 captureCount = _captures.GetSize();
		for(u32 i = 0; i < captureCount; ++i)
		{
			const CaptureInfo& cap = _captures[i];

			if(i == 0 || 
			   cap.BaseToBase != previousBaseToBase ||
			   strcmp(cap.MapName, previousMap) != 0)
			{
				if(i > 0)
				{
					writer.EndArray();
					writer.EndObject();
				}

				writer.StartObject();
				writer.WriteStringValue("map", cap.MapName);
				writer.WriteBoolValue("baseToBase", cap.BaseToBase);
				writer.StartArray("captures");

				previousMap = cap.MapName;
				previousBaseToBase = cap.BaseToBase;

			}

			writer.StartObject();
			if(!cap.BaseToBase && cap.Speed != FLT_MAX)
			{
				writer.WriteIntValue("speed", (s32)cap.Speed);
			}
			writer.WriteIntValue("duration", cap.DurationMs);
			writer.WriteStringValue("fileName", cap.FileName);
			writer.WriteStringValue("filePath", cap.FilePath);
			writer.WriteIntValue("gameStateIndex", cap.GameStateIndex);
			writer.WriteIntValue("startTime", cap.PickUpTimeMs);
			writer.WriteIntValue("endTime", cap.CaptureTimeMs);
			writer.EndObject();
		}

		writer.EndArray();
		writer.EndObject();

		writer.EndArray();
	}

	void WriteFastestBaseToBaseCaptures(udtJSONWriter& writer)
	{
		writer.StartArray("fastestBaseToBaseCaptures");

		const u32 topBaseToBaseCapCount = _topBaseToBaseCapCount;
		const char* previousMap = "__invalid__";
		const char* currentSectionMap = "__invalid__";
		bool previousBaseToBase = false;
		u32 mapCaptureIndex = 0;
		bool hasAtLeastOneObject = false;

		const u32 captureCount = _captures.GetSize();
		for(u32 i = 0; i < captureCount; ++i)
		{
			const CaptureInfo& cap = _captures[i];

			const bool mapChanged = strcmp(cap.MapName, previousMap) != 0;
			if(i == 0 ||
			   (cap.BaseToBase && !previousBaseToBase) ||
			   mapChanged)
			{
				if(cap.BaseToBase)
				{
					if(hasAtLeastOneObject)
					{
						writer.EndArray();
						writer.EndObject();
					}

					hasAtLeastOneObject = true;
					mapCaptureIndex = 0;
					writer.StartObject();
					writer.WriteStringValue("map", cap.MapName);
					writer.StartArray("captures");
					currentSectionMap = cap.MapName;
				}
			}
			previousMap = cap.MapName;
			previousBaseToBase = cap.BaseToBase;

			const bool correctMap = strcmp(cap.MapName, currentSectionMap) == 0;
			if(!cap.BaseToBase || !correctMap)
			{
				continue;
			}

			if(mapCaptureIndex++ >= topBaseToBaseCapCount)
			{
				continue;
			}

			writer.StartObject();
			writer.WriteIntValue("duration", cap.DurationMs);
			writer.WriteStringValue("fileName", cap.FileName);
			writer.WriteStringValue("filePath", cap.FilePath);
			writer.WriteIntValue("gameStateIndex", cap.GameStateIndex);
			writer.WriteIntValue("startTime", cap.PickUpTimeMs);
			writer.WriteIntValue("endTime", cap.CaptureTimeMs);
			writer.EndObject();
		}

		writer.EndArray();
		writer.EndObject();

		writer.EndArray();
	}

	void SortCaptures()
	{
		SortCapturesPass<&SortBySpeedDescending>();
		SortCapturesPass<&SortByDurationAscending>();
		SortCapturesPass<&SortByBaseToBaseTrueToFalse>();
		SortCapturesPass<&SortByMapNameAscending>();
	}

	template<CaptureCompareFunc compareFunc>
	void SortCapturesPass()
	{
		const u32 captureCount = _captures.GetSize();
		for(u32 i = 0; i < captureCount; ++i)
		{
			_captures[i].SortIndex = (s32)i;
		}

		qsort(_captures.GetStartAddress(), (size_t)captureCount, sizeof(CaptureInfo), &GenericStableCaptureSort<compareFunc>);
	}

	udtVMArrayWithAlloc<CaptureInfo> _captures;
	udtVMLinearAllocator _stringAllocator;
	CmdLineParseArg _parseArg;
	u32 _maxThreadCount;
	u32 _topBaseToBaseCapCount;
};


void PrintHelp()
{
	printf("For a collection of demo files, extract and sort all the flag captures from\n");
	printf("the demo taker and store the data to a single JSON output file.\n");
	printf("\n");
	printf("UDT_captures [-r] [-q] [-t=maxthreads] [-b=maxb2bcaps] -o=outputfile inputfolder\n");
	printf("\n");
	printf("-q    quiet mode: no logging to stdout        (default: off)\n");
	printf("-r    enable recursive demo file search       (default: off)\n");
	printf("-t=N  set the maximum number of threads to N  (default: 4)\n");
	printf("-b=N  top N base2base captures per map        (default: 3)\n");
	printf("-o=p  output path p of the JSON file with the sorted results\n");
	printf("\n");
	printf("The top base2base captures are a subset of the entire captures collection\n");
	printf("that is stored in the separate JSON array 'fastestBaseToBaseCaptures'.\n");
	printf("Setting -b=0 will disable the writing of that extra JSON array entirely.\n");
}

static bool KeepOnlyDemoFiles(const char* name, u64 /*size*/, void* /*userData*/)
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
	u32 topBaseToBaseCapCount = 3;
	bool recursive = false;
	for(int i = 1; i < argc - 1; ++i)
	{
		s32 localMaxThreads = 4;
		s32 localCapCount = 3;

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
		else if(udtString::StartsWith(arg, "-b=") &&
				arg.Length >= 4 &&
				StringParseInt(localCapCount, arg.String + 3))
		{
			topBaseToBaseCapCount = (u32)localCapCount;
		}
	}

	if(outputFilePath == NULL)
	{
		fprintf(stderr, "Output file path not specified or invalid.\n");
		PrintHelp();
		return 1;
	}

	// Should be able to handle over 10k demos.
	udtVMArrayWithAlloc<udtFileInfo> files(1 << 24, "udt_main::FilesArray");
	udtVMLinearAllocator persistAlloc;
	udtVMLinearAllocator folderArrayAlloc;
	udtVMLinearAllocator tempAlloc;
	persistAlloc.Init(1 << 26, "udt_main::Persistent");
	folderArrayAlloc.Init(1 << 26, "udt_main::FolderArray");
	tempAlloc.Init(1 << 22, "udt_main::Temp");

	udtFileListQuery query;
	memset(&query, 0, sizeof(query));
	query.FileFilter = &KeepOnlyDemoFiles;
	query.Files = &files;
	query.FolderArrayAllocator = &folderArrayAlloc;
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
	if(!worker.ProcessDemos(files.GetStartAddress(), files.GetSize(), outputFilePath, maxThreadCount, topBaseToBaseCapCount))
	{
		return 2;
	}

	return 0;
}
