#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "analysis_cut_by_pattern.hpp"
#include "array.hpp"
#include "cut_section.hpp"


struct udtCutByChatAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByChatAnalyzer();
	~udtCutByChatAnalyzer();

	void StartAnalysis() override;
	void FinishAnalysis() override;
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtCutByChatAnalyzer);

	udtVMArrayWithAlloc<udtCutSection> _cutSections; // Local copy, write back to the final array as merged.
};
