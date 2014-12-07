#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


//
// This class only does analysis.
// It receive chat commands as input and build a list of 
// merged cut sections to apply for a new parsing pass.
//
// That way, we can either analyze for cutting by chat on first pass or after loading the demo in the GUI.
//
struct udtCutByChatAnalyzer
{
public:
	struct CutSection
	{
		s32 GameStateIndex;
		s32 StartTimeMs;
		s32 EndTimeMs;
	};

	typedef udtVMArray<CutSection> CutSectionVector;

public:
	udtCutByChatAnalyzer(const udtCutByChatArg& info) 
		: _info(info)
	{
	}

	~udtCutByChatAnalyzer() 
	{
	}

	void ProcessOriginalCommandMessage(udtContext& context, const char* commandMessage, s32 serverTimeMs, s32 gsIndex);
	void MergeCutSections();

	CutSectionVector MergedCutSections;

private:
	UDT_NO_COPY_SEMANTICS(udtCutByChatAnalyzer);

	const udtCutByChatArg& _info;
	CutSectionVector _cutSections;
};

struct udtParserPlugInCutByChat : udtBaseParserPlugIn
{
public:
	udtParserPlugInCutByChat(const udtCutByChatArg& info)
		: Analyzer(info)
	{
	}

	~udtParserPlugInCutByChat()
	{
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
	{
		Analyzer.ProcessOriginalCommandMessage(*parser._context, info.String, parser._inServerTime, parser._inGameStateIndex);
	}

	void FinishAnalysis()
	{
		Analyzer.MergeCutSections();
	}

	u32 GetElementCount() const 
	{ 
		return Analyzer.MergedCutSections.GetSize();
	}

	u32 GetElementSize() const 
	{
		return (u32)sizeof(udtCutByChatAnalyzer::CutSection);
	};

	void* GetFirstElementAddress() 
	{
		return GetElementCount() > 0 ? Analyzer.MergedCutSections.GetStartAddress() : NULL;
	}

	udtCutByChatAnalyzer Analyzer;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInCutByChat);
};
