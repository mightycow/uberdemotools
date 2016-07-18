#pragma once


#include "converter_udt_to_quake.hpp"


// Don't ever allocate an instance of this on the stack.
struct udtdEntityTimeShifterPlugIn : public udtdConverterPlugIn
{
public:
	udtdEntityTimeShifterPlugIn();
	~udtdEntityTimeShifterPlugIn();

	void ResetForNextDemo(const udtTimeShiftArg& timeShiftArg);

private:
	void InitPlugIn(udtProtocol::Id protocol) override;
	void ModifySnapshot(udtdSnapshotData& curSnap, udtdSnapshotData& oldSnap) override;
	void AnalyzeConfigString(s32 index, const char* configString, u32 /*stringLength*/) override;
	void CopySnapshot(udtdSnapshotData& dest, const udtdSnapshotData& source);
	void FixSnapshot(udtdSnapshotData& dest, const udtdSnapshotData& source);

	enum Constants
	{
		MaxSnapshotCount = 8
	};

	udtdSnapshotData _backupSnaps[MaxSnapshotCount + 1];
	udtdSnapshotData _newOldSnap;
	udtdSnapshotData _newCurSnap;
	udtVMLinearAllocator _tempAllocator { "UDTDemoEntityTimeShifterPlugIn::Temp" };
	const udtTimeShiftArg* _info;
	udtProtocol::Id _protocol;
	s32 _backupSnapIndex;
	s32 _parsedSnapIndex;
	s32 _snapshotDuration;
	s32 _delaySnapshotCount;
};
