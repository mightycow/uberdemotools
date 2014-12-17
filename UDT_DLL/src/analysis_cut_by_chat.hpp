#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "analysis_cut_base.hpp"
#include "array.hpp"


struct udtCutByChatAnalyzer : public udtCutAnalyzerBase
{
public:
	udtCutByChatAnalyzer(const udtCutByChatArg& info) 
		: _info(info)
	{
	}

	~udtCutByChatAnalyzer() 
	{
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser);
	void FinishAnalysis();

private:
	UDT_NO_COPY_SEMANTICS(udtCutByChatAnalyzer);

	const udtCutByChatArg& _info;
	CutSectionVector _cutSections;
};
