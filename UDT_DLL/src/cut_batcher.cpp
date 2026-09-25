#include "cut_batcher.hpp"


// Assumes a sorted source list.
static bool HasOverlappingCuts(const udtCutArray& sortedCutList)
{
	for(u32 i = 1, count = sortedCutList.GetSize(); i < count; ++i)
	{
		const udtParserCut& prev = sortedCutList[i - 1];
		const udtParserCut& curr = sortedCutList[i];
		if(curr.GameStateIndex == prev.GameStateIndex &&
			curr.StartTimeMs < prev.EndTimeMs)
		{
			return true;
		}
	}

	return false;
}

// Assumes a sorted source list.
static void GenerateNonOverlappingLists(udtCutArrayArray& dstArray, u32& dstSize, const udtCutArray& srcList)
{
	// This is an implementation of the interval partitioning algorithm.
	dstSize = 0;
	for(u32 s = 0, srcSize = srcList.GetSize(); s < srcSize; ++s)
	{
		u32 dstIdx = UDT_U32_MAX;
		s32 dstGSIndex = UDT_S32_MAX;
		s32 dstEndTimeMs = UDT_S32_MAX;
		for(u32 d = 0; d < dstSize; ++d)
		{
			udtCutArray& cutList = dstArray[d];
			assert(cutList.GetSize() > 0);
			const udtParserCut& lastCut = cutList[cutList.GetSize() - 1];
			const s32 gsIndex = lastCut.GameStateIndex;
			const s32 endTimeMs = lastCut.EndTimeMs;
			if((gsIndex < dstGSIndex) ||
				(gsIndex == dstGSIndex && endTimeMs < dstEndTimeMs))
			{
				dstIdx = d;
				dstGSIndex = gsIndex;
				dstEndTimeMs = endTimeMs;
			}
		}

		const udtParserCut& src = srcList[s];
		const bool batchFound = dstIdx < dstSize;
		const bool validOption1 = src.GameStateIndex > dstGSIndex;
		const bool validOption2 = src.GameStateIndex == dstGSIndex && src.StartTimeMs >= dstEndTimeMs;
		if(batchFound && (validOption1 || validOption2))
		{
			udtCutArray& cutList = dstArray[dstIdx];
			cutList.Add(src);
		}
		else if(dstSize < UDT_ARRAY_LENGTH(dstArray))
		{
			udtCutArray& cutList = dstArray[dstSize++];
			cutList.Add(src);
		}
	}
}

static void RemoveInvalidCuts(udtCutArray& cuts)
{
	if(cuts.IsEmpty())
	{
		return;
	}

	// @TODO:
}

static int CompareCuts(const void* aPtr, const void* bPtr)
{
	const udtParserCut& a = *(udtParserCut*)aPtr;
	const udtParserCut& b = *(udtParserCut*)bPtr;

	const s32 g = a.GameStateIndex - b.GameStateIndex;
	if(g < 0) return -1;
	if(g > 0) return 1;

	const s32 s = a.StartTimeMs - b.StartTimeMs;
	if(s < 0) return -1;
	if(s > 0) return 1;

	const s32 e = a.EndTimeMs - b.EndTimeMs;
	if(e < 0) return -1;
	if(e > 0) return 1;

	return 0;
}

udtCutBatcher::udtCutBatcher()
{
	Clear();
}

void udtCutBatcher::Clear()
{
	Cuts.Clear();
	BatchCount = 0;
}

void udtCutBatcher::Process()
{
	RemoveInvalidCuts(Cuts);
	if(Cuts.IsEmpty())
	{
		udtCutArray& batch = Batches[0];
		batch.Resize(0);
		BatchCount = 1;
		return;
	}

	qsort(Cuts.GetStartAddress(), (size_t)Cuts.GetSize(), sizeof(decltype(Cuts)::Type), &CompareCuts);
	if(HasOverlappingCuts(Cuts))
	{
		GenerateNonOverlappingLists(Batches, BatchCount, Cuts);
	}
	else
	{
		udtCutArray& batch = Batches[0];
		batch.Resize(Cuts.GetSize());
		memcpy(batch.GetStartAddress(), Cuts.GetStartAddress(), Cuts.GetUsedByteCount());
		BatchCount = 1;
	}
}
