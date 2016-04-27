#pragma once


#include "parser_plug_in.hpp"
#include "array.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"
#include "string.hpp"


struct udtPatternSearchPlugIn;

struct udtPatternSearchAnalyzerBase
{
public:
	friend udtPatternSearchPlugIn;

	udtPatternSearchAnalyzerBase();
	virtual ~udtPatternSearchAnalyzerBase() {}
	
	virtual void InitAllocators(u32 /*demoCount*/) {}
	virtual void StartAnalysis() {}
	virtual void FinishAnalysis() {}
	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	
	udtVMArray<udtCutSection> CutSections;

protected:
	udtPatternSearchPlugIn* PlugIn;
	const void* ExtraInfo;

	template<typename T>
	const T& GetExtraInfo() const
	{
		return *(T*)ExtraInfo;
	}

private:
	UDT_NO_COPY_SEMANTICS(udtPatternSearchAnalyzerBase);
};

struct udtPatternSearchPlugIn : udtBaseParserPlugIn
{
public:
	udtPatternSearchPlugIn();
	~udtPatternSearchPlugIn() {}

	void InitAllocators(u32 demoCount) override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser) override;

	void                         InitAnalyzerAllocators(u32 demoCount);
	udtPatternSearchAnalyzerBase* CreateAndAddAnalyzer(udtPatternType::Id patternType, const void* extraInfo);
	udtPatternSearchAnalyzerBase* GetAnalyzer(udtPatternType::Id patternType);

	void SetPatternInfo(const udtPatternSearchArg& info) { _info = &info; }

	s32 GetTrackedPlayerIndex() const;
	const udtPatternSearchArg& GetInfo() const { return *_info; }

	udtVMLinearAllocator& GetTempAllocator() { return *TempAllocator; }

	udtVMArray<udtCutSection> CutSections; // Final array.

private:
	UDT_NO_COPY_SEMANTICS(udtPatternSearchPlugIn);

	void FindPlayerInConfigStrings(udtBaseParser& parser);
	void FindPlayerInServerCommand(const udtCommandCallbackArg& info, udtBaseParser& parser);
	bool GetPlayerName(udtString& playerName, udtVMLinearAllocator& allocator, udtBaseParser& parser, s32 csIdx);

	udtVMArray<udtPatternSearchAnalyzerBase*> _analyzers;
	udtVMArray<udtPatternType::Id> _analyzerTypes;
	udtVMLinearAllocator _analyzerAllocator;
	udtVMScopedStackAllocator _analyzerAllocatorScope;

	const udtPatternSearchArg* _info;
	s32 _trackedPlayerIndex;
};
