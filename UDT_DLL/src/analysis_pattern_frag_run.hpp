#pragma once


#include "analysis_obituaries.hpp"
#include "analysis_pattern_base.hpp"


struct udtFragRunPatternAnalyzer : public udtPatternSearchAnalyzerBase
{
public:
	udtFragRunPatternAnalyzer();
	~udtFragRunPatternAnalyzer();

	void InitAllocators(u32 demoCount) override;
	void StartAnalysis() override;
	void FinishAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtFragRunPatternAnalyzer);

	void AddCurrentSectionIfValid();
	void AddMatch(const udtParseDataObituary& data);

	struct Frag
	{
		s32 GameStateIndex;
		s32 ServerTimeMs;
	};

	udtVMArray<Frag> _frags { "CutByFragAnalyzer::FragsArray" };
	udtObituariesAnalyzer _analyzer;
};
