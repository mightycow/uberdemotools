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
static void GenerateNonOverlappingLists(udtCutArrayArray& dst, u32& dstSize, const udtCutArray& src)
{
	// This is an implementation of the interval partitioning algorithm.
	dstSize = 0;
	for(u32 s = 0, srcSize = src.GetSize(); s < srcSize; ++s)
	{
		u32 dstIdx = UDT_U32_MAX;
		s32 dstEndTimeMs = UDT_S32_MAX;
		for(u32 d = 0; d < dstSize; ++d)
		{
			udtCutArray& cutList = dst[d];
			assert(cutList.GetSize() > 0);
			const s32 endTimeMs = cutList[cutList.GetSize() - 1].EndTimeMs;
			if(endTimeMs < dstEndTimeMs)
			{
				dstIdx = d;
				dstEndTimeMs = endTimeMs;
			}
		}

		if(dstIdx < dstSize && src[s].StartTimeMs >= dstEndTimeMs)
		{
			udtCutArray& cutList = dst[dstIdx];
			cutList.Add(src[s]);
		}
		else if(dstSize < UDT_ARRAY_LENGTH(dst))
		{
			udtCutArray& cutList = dst[dstSize++];
			cutList.Add(src[s]);
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
