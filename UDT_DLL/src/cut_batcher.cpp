#include "cut_batcher.hpp"


bool CutList_HasOverlappingCuts(const udtCutArray& sortedCutList)
{
	for(u32 i = 1, count = sortedCutList.GetSize(); i < count; ++i)
	{
		if(sortedCutList[i].StartTimeMs < sortedCutList[i - 1].EndTimeMs)
		{
			return true;
		}
	}

	return false;
}

void CutList_GenerateNonOverlappingLists(udtCutArrayArray& dst, u32& dstSize, const udtCutArray& src)
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
		else if(dstSize < 64) // @TODO: how to extract that constant?
		{
			udtCutArray& cutList = dst[dstSize++];
			cutList.Add(src[s]);
		}
	}
}
