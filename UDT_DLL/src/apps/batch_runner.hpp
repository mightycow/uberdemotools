#pragma once


#include "uberdemotools.h"
#include "macros.hpp"
#include "file_system.hpp"
#include "array.hpp"
#include "timer.hpp"


struct CmdLineParseArg
{
	CmdLineParseArg()
	{
		CancelOperation = 0;
		
		memset(&ParseArg, 0, sizeof(ParseArg));
		ParseArg.CancelOperation = &CancelOperation;
		ParseArg.MessageCb = &CallbackConsoleMessage;
		ParseArg.ProgressCb = &CallbackConsoleProgress;
		ParseArg.MinProgressTimeMs = 250;
		ParseArg.OutputFolderPath = NULL;
	}

	void SetSinglePlugIn(udtParserPlugIn::Id plugInId)
	{
		PlugInId = (u32)plugInId;
		ParseArg.PlugIns = &PlugInId;
		ParseArg.PlugInCount = 1;
	}

	s32 CancelOperation;
	u32 PlugInId;
	udtParseArg ParseArg;
};

struct BatchRunner
{
	struct BatchInfo
	{
		u64 ByteCount;
		u32 FirstFileIndex;
		u32 FileCount;
	};

	BatchRunner(udtParseArg& parseArg, const udtFileInfo* files, u32 fileCount, u32 maxBatchSize)
	{
		_processedByteCount = 0;
		_totalByteCount = 0;
		_progressBase = 0.0f;
		_progressRange = 0.0f;
		_files = files;
		_fileCount = fileCount;
		_maxBatchSize = maxBatchSize;
		_batchIndex = -1;
		_progressCb = parseArg.ProgressCb;
		_progressUserData = parseArg.ProgressContext;

		if(fileCount <= maxBatchSize)
		{
			u64 totalByteCount = 0;
			for(u32 i = 0; i < fileCount; ++i)
			{
				const u64 byteCount = files[i].Size;
				totalByteCount += byteCount;
			}
			_totalByteCount = totalByteCount;

			BatchInfo info;
			info.ByteCount = totalByteCount;
			info.FileCount = fileCount;
			info.FirstFileIndex = 0;
			_batches.Add(info);

			return;
		}

		parseArg.ProgressCb = &BatchRunner::BatchRunnerProgressCallback;
		parseArg.ProgressContext = this;

		const u32 batchCount = (fileCount + maxBatchSize - 1) / maxBatchSize;
		const u32 filesPerBatch = fileCount / batchCount;
		_batches.Resize(batchCount);

		u64 totalByteCount = 0;
		u32 fileOffset = 0;
		for(u32 i = 0; i < batchCount; ++i)
		{
			const u32 batchFileCount = (i == batchCount - 1) ? (fileCount - fileOffset) : filesPerBatch;

			u64 batchByteCount = 0;
			for(u32 j = fileOffset; j < fileOffset + batchFileCount; ++j)
			{
				const u64 byteCount = files[j].Size;
				totalByteCount += byteCount;
				batchByteCount += byteCount;
			}

			BatchInfo info;
			info.ByteCount = batchByteCount;
			info.FileCount = batchFileCount;
			info.FirstFileIndex = fileOffset;
			_batches[i] = info;

			fileOffset += batchFileCount;
		}

		_totalByteCount = totalByteCount;
	}

	u32 GetBatchCount() const
	{
		return _batches.GetSize();
	}

	void PrepareNextBatch()
	{
		++_batchIndex;
		if(_fileCount <= _maxBatchSize ||
		   (u32)_batchIndex >= GetBatchCount())
		{
			return;
		}

		_progressBase = (f64)_processedByteCount / (f64)_totalByteCount;
		_progressRange = (f64)_batches[_batchIndex].ByteCount / (f64)_totalByteCount;
		_processedByteCount += _batches[_batchIndex].ByteCount;
	}

	const BatchInfo& GetBatchInfo(u32 batchIndex) const
	{
		return _batches[batchIndex];
	}

private:
	static void BatchRunnerProgressCallback(f32 progress, void* userData)
	{
		if(userData == NULL)
		{
			return;
		}

		BatchRunner* const runner = (BatchRunner*)userData;
		const f64 realProgress = runner->_progressBase + runner->_progressRange * f64(progress);
		(*runner->_progressCb)((f32)realProgress, runner->_progressUserData);
	}

	udtVMArray<BatchInfo> _batches { "BatchRunner::BatchArray" };
	u64 _processedByteCount;
	u64 _totalByteCount;
	f64 _progressBase;
	f64 _progressRange;
	const udtFileInfo* _files;
	udtProgressCallback _progressCb;
	void* _progressUserData;
	u32 _fileCount;
	u32 _maxBatchSize;
	s32 _batchIndex;
};
