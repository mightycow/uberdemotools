#pragma once


#include "analysis_obituaries.hpp"
#include "analysis_cut_by_pattern.hpp"


struct udtCutByFragAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByFragAnalyzer();
	~udtCutByFragAnalyzer();

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);
	void FinishAnalysis();

private:
	UDT_NO_COPY_SEMANTICS(udtCutByFragAnalyzer);

	void AddCurrentSectionIfValid();
	void AddMatch(const udtParseDataObituary& data);

	struct Frag
	{
		s32 GameStateIndex;
		s32 ServerTimeMs;
	};

	udtVMArray<Frag> _frags;
	udtObituariesAnalyzer _analyzer;
};
