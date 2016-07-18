#pragma once


#include "parser_plug_in.hpp"
#include "cut_section.hpp"


struct udtPatternSearchPlugIn;

struct udtPatternSearchAnalyzerBase
{
public:
	friend udtPatternSearchPlugIn;

	udtPatternSearchAnalyzerBase() {}
	virtual ~udtPatternSearchAnalyzerBase() {}

	virtual void InitAllocators(u32 /*demoCount*/) {}
	virtual void StartAnalysis() {}
	virtual void FinishAnalysis() {}
	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}

	udtVMArray<udtCutSection> CutSections { "PatternSearchAnalyzerBase::CutSectionsArray" };

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
