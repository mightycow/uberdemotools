#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "analysis_cut_by_pattern.hpp"
#include "array.hpp"
#include "cut_section.hpp"


struct udtCutByChatAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByChatAnalyzer()
	{
	}

	~udtCutByChatAnalyzer() 
	{
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser);
	void FinishAnalysis();

private:
	UDT_NO_COPY_SEMANTICS(udtCutByChatAnalyzer);

	udtVMArray<udtCutSection> _cutSections;
};
