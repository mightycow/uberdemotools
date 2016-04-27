#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "analysis_cut_by_pattern.hpp"
#include "array.hpp"
#include "cut_section.hpp"


struct udtChatPatternAnalyzer : public udtPatternSearchAnalyzerBase
{
public:
	udtChatPatternAnalyzer();
	~udtChatPatternAnalyzer();

	void StartAnalysis() override;
	void FinishAnalysis() override;
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtChatPatternAnalyzer);

	udtVMArray<udtCutSection> _cutSections; // Local copy, write back to the final array as merged.
};
