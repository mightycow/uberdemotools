#pragma once


#include "parser_plug_in.hpp"
#include "array.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"


struct udtCutByPatternPlugIn;

struct udtCutByPatternAnalyzerBase
{
public:
	friend udtCutByPatternPlugIn;

	udtCutByPatternAnalyzerBase() {}
	virtual ~udtCutByPatternAnalyzerBase() {}

	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void FinishAnalysis() {}

	udtVMArray<udtCutSection> CutSections;

protected:
	const udtCutByPatternPlugIn* PlugIn;
	const void* ExtraInfo;

	template<typename T>
	const T& GetExtraInfo() const
	{
		return *(T*)ExtraInfo;
	}

private:
	UDT_NO_COPY_SEMANTICS(udtCutByPatternAnalyzerBase);
};

struct udtCutByPatternPlugIn : udtBaseParserPlugIn
{
public:
	udtCutByPatternPlugIn(udtVMLinearAllocator& analyzerAllocator, const udtCutByPatternArg& info);

	~udtCutByPatternPlugIn()
	{
	}

	udtCutByPatternAnalyzerBase* CreateAndAddAnalyzer(udtPatternType::Id patternType, const void* extraInfo);
	udtCutByPatternAnalyzerBase* GetAnalyzer(udtPatternType::Id patternType);

	s32 GetTrackedPlayerIndex() const;
	const udtCutByPatternArg& GetInfo() const { return _info; }

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

	void        TrackPlayerFromCommandMessage(udtBaseParser& parser);
	const char* GetPlayerName(udtBaseParser& parser, s32 csIndex);

	udtVMArray<udtCutByPatternAnalyzerBase*> _analyzers;
	udtVMArray<udtPatternType::Id> _analyzerTypes;
	udtVMScopedStackAllocator _analyzerAllocatorScope;
	udtVMLinearAllocator _tempAllocator;

	const udtCutByPatternArg& _info;
	s32 _trackedPlayerIndex;
};
