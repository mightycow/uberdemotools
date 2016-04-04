#pragma once


#include "parser_plug_in.hpp"
#include "array.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"
#include "string.hpp"


struct udtCutByPatternPlugIn;

struct udtCutByPatternAnalyzerBase
{
public:
	friend udtCutByPatternPlugIn;

	udtCutByPatternAnalyzerBase();
	virtual ~udtCutByPatternAnalyzerBase() {}
	
	virtual void InitAllocators(u32 /*demoCount*/) {}
	virtual void StartAnalysis() {}
	virtual void FinishAnalysis() {}
	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	
	udtVMArray<udtCutSection> CutSections;

protected:
	udtCutByPatternPlugIn* PlugIn;
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

	void InitAllocators(u32 demoCount) override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser) override;

	void                         InitAnalyzerAllocators(u32 demoCount);
	udtCutByPatternAnalyzerBase* CreateAndAddAnalyzer(udtPatternType::Id patternType, const void* extraInfo);
	udtCutByPatternAnalyzerBase* GetAnalyzer(udtPatternType::Id patternType);

	void SetPatternInfo(const udtCutByPatternArg& info) { _info = &info; }

	s32 GetTrackedPlayerIndex() const;
	const udtCutByPatternArg& GetInfo() const { return *_info; }

	udtVMLinearAllocator& GetTempAllocator() { return *TempAllocator; }

	udtVMArray<udtCutSection> CutSections; // Final array.

private:
	UDT_NO_COPY_SEMANTICS(udtCutByPatternPlugIn);

	void TrackPlayerFromCommandMessage(udtBaseParser& parser);
	bool GetTempPlayerName(udtString& playerName, udtBaseParser& parser, s32 csIdx); // Valid until TempAllocator calls Allocate again.

	udtVMArray<udtCutByPatternAnalyzerBase*> _analyzers;
	udtVMArray<udtPatternType::Id> _analyzerTypes;
	udtVMLinearAllocator _analyzerAllocator;
	udtVMScopedStackAllocator _analyzerAllocatorScope;

	const udtCutByPatternArg* _info;
	s32 _trackedPlayerIndex;
};
