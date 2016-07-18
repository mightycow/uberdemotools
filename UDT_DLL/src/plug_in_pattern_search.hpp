#pragma once


#include "parser_plug_in.hpp"
#include "analysis_pattern_base.hpp"
#include "array.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"
#include "string.hpp"


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

	void                          InitAnalyzerAllocators(u32 demoCount);
	udtPatternSearchAnalyzerBase* CreateAndAddAnalyzer(udtPatternType::Id patternType, const void* extraInfo);
	udtPatternSearchAnalyzerBase* GetAnalyzer(udtPatternType::Id patternType);

	void SetPatternInfo(const udtPatternSearchArg& info) { _info = &info; }

	s32 GetTrackedPlayerIndex() const;
	const udtPatternSearchArg& GetInfo() const { return *_info; }

	udtVMLinearAllocator& GetTempAllocator() { return *TempAllocator; }

	udtVMArray<udtCutSection> CutSections { "CutByPatternPlugIn::CutSectionsArray" }; // Final array.

private:
	UDT_NO_COPY_SEMANTICS(udtPatternSearchPlugIn);

	void FindPlayerInConfigStrings(udtBaseParser& parser);
	void FindPlayerInServerCommand(const udtCommandCallbackArg& info, udtBaseParser& parser);
	bool GetPlayerName(udtString& playerName, udtVMLinearAllocator& allocator, udtBaseParser& parser, s32 csIdx);

	udtVMArray<udtPatternSearchAnalyzerBase*> _analyzers { "CutByPatternPlugIn::AnalyzersArray" };
	udtVMArray<udtPatternType::Id> _analyzerTypes { "CutByPatternPlugIn::AnalyzerTypesArray" };
	udtVMLinearAllocator _analyzerAllocator { "CutByPatternPlugIn::AnalyzerData" };
	udtVMScopedStackAllocator _analyzerAllocatorScope;

	const udtPatternSearchArg* _info;
	s32 _trackedPlayerIndex;
};
