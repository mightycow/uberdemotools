#pragma once


#include "parser_plug_in.hpp"
#include "array.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"


struct udtCutByPatternAnalyzerBase
{
public:
	udtCutByPatternAnalyzerBase() {}
	virtual ~udtCutByPatternAnalyzerBase() {}

	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void FinishAnalysis() {}

	udtVMArray<udtCutSection> CutSections;

private:
	UDT_NO_COPY_SEMANTICS(udtCutByPatternAnalyzerBase);
};

struct udtCutByPatternPlugIn : udtBaseParserPlugIn
{
public:
	udtCutByPatternPlugIn();

	~udtCutByPatternPlugIn()
	{
	}

	udtCutByPatternAnalyzerBase* CreateAndAddAnalyzer(udtPatternType::Id patternType, const udtCutByPatternArg* info, const void* extraInfo);
	udtCutByPatternAnalyzerBase* GetAnalyzer(udtPatternType::Id patternType);

	void ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser);
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser);
	void FinishAnalysis();

	u32 GetElementCount() const
	{
		return CutSections.GetSize();
	}

	u32 GetElementSize() const
	{
		return (u32)sizeof(udtCutSection);
	};

	void* GetFirstElementAddress()
	{
		return GetElementCount() > 0 ? CutSections.GetStartAddress() : NULL;
	}

	udtVMArray<udtCutSection> CutSections;

private:
	UDT_NO_COPY_SEMANTICS(udtCutByPatternPlugIn);

	udtVMArray<udtCutByPatternAnalyzerBase*> _analyzers;
	udtVMArray<udtPatternType::Id> _analyzerTypes;
	udtVMLinearAllocator _analyzerAllocator;
	udtVMScopedStackAllocator _analyzerAllocatorScope;
};
