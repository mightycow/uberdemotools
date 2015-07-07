#pragma once


#include "converter_udt_to_quake.hpp"
#include "api.h"


struct udtdEntityTimeShifterPlugIn : public udtdConverterPlugIn
{
public:
	void Init(const udtTimeShiftArg& timeShiftArg);

private:
	void ModifySnapshot(udtdSnapshotData& curSnap, udtdSnapshotData& oldSnap) override;
	void AnalyzeConfigString(s32 index, const char* configString, u32 /*stringLength*/) override;
	void CopySnapshot(udtdSnapshotData& dest, const udtdSnapshotData& source);
	void FixSnapshot(udtdSnapshotData& dest, const udtdSnapshotData& source);

	enum Constants
	{
		MaxSnapshotCount = 8
	};

	udtdSnapshotData _backupSnaps[MaxSnapshotCount];
	udtdSnapshotData _newOldSnap;
	udtdSnapshotData _newCurSnap;
	udtVMLinearAllocator _tempAllocator;
	const udtTimeShiftArg* _info;
	s32 _backupSnapIndex;
	s32 _parsedSnapIndex;
	s32 _snapshotDuration;
	s32 _delaySnapshotCount;
};
