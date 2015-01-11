#pragma once


#include "parser_plug_in.hpp"
#include "array.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"


struct udtCutByPatternPlugIn;

struct udtCutByPatternAnalyzerBase
{
public:
	friend udtCutByPatternPlugIn;

	udtCutByPatternAnalyzerBase();
	virtual ~udtCutByPatternAnalyzerBase() {}

	void ResetForNextDemo();
	
	virtual void FinishAnalysis() {}
	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	
	udtVMArrayWithAlloc<udtCutSection> CutSections;

protected:
	virtual void OnResetForNextDemo() {}

	const udtCutByPatternPlugIn* PlugIn;
	const void* ExtraInfo;

	template<typename T>
	const T& GetExtraInfo() const
	{
		return *(T*)ExtraInfo;
	}

private:
	UDT_NO_COPY_SEMANTICS(udtCutByPatternAnalyzerBase);
};

struct udtCutByPatternPlugIn : udtBaseParserPlugIn
{
public:
	udtCutByPatternPlugIn();
	~udtCutByPatternPlugIn() {}

	void InitAllocators(u32 demoCount);
	u32  GetElementSize() const { return (u32)sizeof(udtCutSection); };

	void ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser);
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser);
	void FinishDemoAnalysis();

	udtCutByPatternAnalyzerBase* CreateAndAddAnalyzer(udtPatternType::Id patternType, const void* extraInfo);
	udtCutByPatternAnalyzerBase* GetAnalyzer(udtPatternType::Id patternType);

	void SetPatternInfo(const udtCutByPatternArg& info) { _info = &info; }

	s32 GetTrackedPlayerIndex() const;
	const udtCutByPatternArg& GetInfo() const { return *_info; }

	udtVMArray<udtCutSection> CutSections; // Final array.

private:
	UDT_NO_COPY_SEMANTICS(udtCutByPatternPlugIn);

	void        TrackPlayerFromCommandMessage(udtBaseParser& parser);
	const char* GetPlayerName(udtBaseParser& parser, s32 csIndex);

	udtVMArrayWithAlloc<udtCutByPatternAnalyzerBase*> _analyzers;
	udtVMArrayWithAlloc<udtPatternType::Id> _analyzerTypes;
	udtVMLinearAllocator _analyzerAllocator;
	udtVMScopedStackAllocator _analyzerAllocatorScope;

	const udtCutByPatternArg* _info;
	s32 _trackedPlayerIndex;
};
