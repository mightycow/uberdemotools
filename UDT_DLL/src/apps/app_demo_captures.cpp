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
	udtString FilePath;
	udtString FileName;
	udtString MapName;
	s32 GameStateIndex;
	s32 PickUpTimeMs;
	s32 CaptureTimeMs;
	f32 Distance;
	s32 DurationMs;
	f32 Speed;
	s32 SortIndex; // Used for stable sorting.
	u32 DemoIndex; // Used for having the same results no matter the number of threads.
	bool BaseToBase;
	bool DistanceAndSpeedValid;
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
	return (s32)strcmp(a.MapName.GetPtr(), b.MapName.GetPtr());
}

s32 SortByBaseToBaseTrueToFalse(const CaptureInfo& a, const CaptureInfo& b)
{
	return (s32)(!!b.BaseToBase - !!a.BaseToBase);
}

s32 SortBySpeedDescending(const CaptureInfo& a, const CaptureInfo& b)
{
	return (s32)(b.Speed - a.Speed);
}

s32 SortByDemoIndexAscending(const CaptureInfo& a, const CaptureInfo& b)
{
	return (s32)(a.DemoIndex - b.DemoIndex);
}


struct Worker
{
public:
	Worker()
	{
		_maxThreadCount = 1;
		_topBaseToBaseTimeCount = 3;

		_parseArg.SetSinglePlugIn(udtParserPlugIn::Captures);
	}

	bool ProcessDemos(const udtFileInfo* files, u32 fileCount, const char* outputFilePath, u32 maxThreadCount, u32 topBaseToBaseTimeCount)
	{
		_maxThreadCount = maxThreadCount;
		_topBaseToBaseTimeCount = topBaseToBaseTimeCount;

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
		if(topBaseToBaseTimeCount > 0)
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
		udtVMArray<const char*> filePaths("Worker::ProcessBatch::FilePathsArray");
		udtVMArray<s32> errorCodes("Worker::ProcessBatch::ErrorCodesArray");
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

			udtParseDataCaptureBuffers buffers;
			udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::Captures, &buffers);

			for(u32 demoIdx = 0; demoIdx < demoCount; ++demoIdx)
			{
				u32 demoInputIdx = 0;
				udtGetDemoInputIndex(context, demoIdx, &demoInputIdx);

				const udtParseDataBufferRange range = buffers.CaptureRanges[demoIdx];
				const u32 firstCapIdx = range.FirstIndex;
				const u32 capCount = range.Count;
				ProcessCaptures(buffers, buffers.Captures + firstCapIdx, capCount, demoInputIdx, files[demoInputIdx].Path, files[demoInputIdx].Name);
			}
		}
	}

	void ProcessCaptures(const udtParseDataCaptureBuffers& buffers, const udtParseDataCapture* captures, u32 captureCount, u32 fileIndex, udtString filePath, udtString fileName)
	{
		for(u32 i = 0; i < captureCount; ++i)
		{
			const udtParseDataCapture& capture = captures[i];
			if((capture.Flags & (u32)udtParseDataCaptureMask::DemoTaker) == 0)
			{
				continue;
			}

			const s32 durationMs = capture.CaptureTimeMs - capture.PickUpTimeMs;

			udtString mapName = udtString::NewClone(_stringAllocator, (const char*)buffers.StringBuffer + capture.MapName);
			udtString::MakeLowerCase(mapName);

			const bool distanceValid = (capture.Flags & (u32)udtParseDataCaptureMask::DistanceValid) != 0;

			CaptureInfo cap;
			cap.BaseToBase = (capture.Flags & (u32)udtParseDataCaptureMask::BaseToBase) != 0;
			cap.CaptureTimeMs = capture.CaptureTimeMs;
			cap.Distance = capture.Distance;
			cap.DistanceAndSpeedValid = distanceValid && durationMs > 0;
			cap.DurationMs = durationMs;
			cap.FileName = udtString::NewCloneFromRef(_stringAllocator, fileName);
			cap.FilePath = udtString::NewCloneFromRef(_stringAllocator, filePath);
			cap.GameStateIndex = capture.GameStateIndex;
			cap.SortIndex = (s32)i;
			cap.DemoIndex = fileIndex;
			cap.MapName = mapName;
			cap.PickUpTimeMs = capture.PickUpTimeMs;
			cap.Speed = durationMs == 0 ? 0.0f : (capture.Distance / ((f32)durationMs / 1000.0f));
			_captures.Add(cap);
		}
	}

	void WriteAllCaptures(udtJSONWriter& writer)
	{
		writer.StartArray("allCaptures");

		udtString previousMap = udtString::NewConstRef("__invalid__");
		bool previousBaseToBase = false;

		const u32 captureCount = _captures.GetSize();
		for(u32 i = 0; i < captureCount; ++i)
		{
			const CaptureInfo& cap = _captures[i];
			if(i == 0 || 
			   cap.BaseToBase != previousBaseToBase ||
			   !udtString::EqualsNoCase(cap.MapName, previousMap))
			{
				if(i > 0)
				{
					writer.EndArray();
					writer.EndObject();
				}

				writer.StartObject();
				writer.WriteStringValue("map", cap.MapName.GetPtr());
				writer.WriteBoolValue("baseToBase", cap.BaseToBase);
				writer.StartArray("captures");

				previousMap = cap.MapName;
				previousBaseToBase = cap.BaseToBase;

			}

			writer.StartObject();
			if(!cap.BaseToBase && cap.DistanceAndSpeedValid)
			{
				writer.WriteIntValue("distance", (s32)cap.Distance);
				writer.WriteIntValue("speed", (s32)cap.Speed);
			}
			writer.WriteIntValue("duration", cap.DurationMs);
			writer.WriteStringValue("fileName", cap.FileName.GetPtr());
			writer.WriteStringValue("filePath", cap.FilePath.GetPtr());
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

		const u32 topBaseToBaseTimeCount = _topBaseToBaseTimeCount;
		udtString previousMap = udtString::NewConstRef("__invalid__");
		udtString currentSectionMap = udtString::NewConstRef("__invalid__");
		bool previousBaseToBase = false;
		u32 mapTimeIndex = 0;
		s32 previousMapTimeMs = UDT_S32_MIN;
		bool hasAtLeastOneObject = false;

		const u32 captureCount = _captures.GetSize();
		for(u32 i = 0; i < captureCount; ++i)
		{
			const CaptureInfo& cap = _captures[i];

			const bool mapChanged = !udtString::EqualsNoCase(cap.MapName, previousMap);
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
					mapTimeIndex = 0;
					previousMapTimeMs = UDT_S32_MIN;
					writer.StartObject();
					writer.WriteStringValue("map", cap.MapName.GetPtr());
					writer.StartArray("captures");
					currentSectionMap = cap.MapName;
				}
			}
			previousMap = cap.MapName;
			previousBaseToBase = cap.BaseToBase;

			const bool correctMap = udtString::EqualsNoCase(cap.MapName, currentSectionMap);
			if(!cap.BaseToBase || !correctMap)
			{
				continue;
			}

			if(cap.DurationMs > previousMapTimeMs)
			{
				++mapTimeIndex;
				previousMapTimeMs = cap.DurationMs;
			}

			if(mapTimeIndex > topBaseToBaseTimeCount)
			{
				continue;
			}

			writer.StartObject();
			writer.WriteIntValue("duration", cap.DurationMs);
			writer.WriteStringValue("fileName", cap.FileName.GetPtr());
			writer.WriteStringValue("filePath", cap.FilePath.GetPtr());
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
		SortCapturesPass<&SortByDemoIndexAscending>();
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

	udtVMArray<CaptureInfo> _captures { "Worker::CapturesArray" };
	udtVMLinearAllocator _stringAllocator { "Worker::Strings" };
	CmdLineParseArg _parseArg;
	u32 _maxThreadCount;
	u32 _topBaseToBaseTimeCount;
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
	printf("-t=N  set the maximum number of threads to N  (default: 1)\n");
	printf("-b=N  top N base2base capture times per map   (default: 3)\n");
	printf("-o=p  output path p of the JSON file with the sorted results\n");
	printf("\n");
	printf("The top base2base capture times are a subset of the entire captures collection\n");
	printf("that are stored in the separate JSON array 'fastestBaseToBaseCaptures'.\n");
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
	u32 maxThreadCount = 1;
	u32 topBaseToBaseTimeCount = 3;
	bool recursive = false;
	for(int i = 1; i < argc - 1; ++i)
	{
		s32 localMaxThreads = 1;
		s32 localTimeCount = 3;

		const udtString arg = udtString::NewConstRef(argv[i]);
		if(udtString::Equals(arg, "-r"))
		{
			recursive = true;
		}
		else if(udtString::StartsWith(arg, "-o=") &&
				arg.GetLength() >= 4)
		{
			outputFilePath = argv[i] + 3;
		}
		else if(udtString::StartsWith(arg, "-t=") &&
				arg.GetLength() >= 4 &&
				StringParseInt(localMaxThreads, arg.GetPtr() + 3) &&
				localMaxThreads >= 1 &&
				localMaxThreads <= 16)
		{
			maxThreadCount = (u32)localMaxThreads;
		}
		else if(udtString::StartsWith(arg, "-b=") &&
				arg.GetLength() >= 4 &&
				StringParseInt(localTimeCount, arg.GetPtr() + 3))
		{
			topBaseToBaseTimeCount = (u32)localTimeCount;
		}
	}

	if(outputFilePath == NULL)
	{
		fprintf(stderr, "Output file path not specified or invalid.\n");
		PrintHelp();
		return 1;
	}

	udtFileListQuery query;
	query.FileFilter = &KeepOnlyDemoFiles;
	query.FolderPath = udtString::NewConstRef(directoryPath);
	query.Recursive = recursive;
	GetDirectoryFileList(query);

	if(query.Files.GetSize() == 0)
	{
		return 0;
	}

	Worker worker;
	if(!worker.ProcessDemos(query.Files.GetStartAddress(), query.Files.GetSize(), outputFilePath, maxThreadCount, topBaseToBaseTimeCount))
	{
		return 2;
	}

	return 0;
}
