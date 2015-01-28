#pragma once


#include "common.hpp"
#include "linear_allocator.hpp"


struct udtConfigStringConversion
{
	const char* String; // Always valid.
	u32 StringLength; // Always valid.
	s32 Index; // Negative if needs to be dropped.
	bool NewString; // True when a new string was created.
};

struct udtProtocolConverter
{
	void(*ConvertSnapshot)(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot);
	void(*ConvertEntityState)(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState);
	void(*ConvertConfigString)(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength);
};

extern void GetProtocolConverter(udtProtocolConverter& converter, udtProtocol::Id outProtocol, udtProtocol::Id inProtocol);
