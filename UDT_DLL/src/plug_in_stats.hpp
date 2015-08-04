#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"
#include "analysis_general.hpp"


struct udtParserPlugInStats : udtBaseParserPlugIn
{
public:
	udtParserPlugInStats();
	~udtParserPlugInStats();

	void InitAllocators(u32 demoCount) override;
	u32  GetElementSize() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInStats);

	struct udtStatsField
	{
		s32 Index;
		s32 TokenIndex;
	};

	s32  GetValue(s32 index);
	void ParseFields(u8* destMask, s32* destFields, const udtStatsField* fields, s32 fieldCount, s32 tokenOffset = 0);
	s64  CreateBitMask(const udtStatsField* fields, s32 fieldCount);
	void AddCurrentStats();
	void ClearStats();
	void ProcessConfigString(s32 csIndex, const udtString& configString);
	void ProcessPlayerConfigString(const char* configString, s32 playerIndex);
	bool GetClientNumberFromScoreIndex(s32& clientNumber, s32 fieldIndex);
	bool AreStatsValid();
	void ParseQLScoresTDM();
	void ParseQLStatsTDM();
	void ParseQLScoresDuel();
	void ParseQLScoresCTF();
	void ParseQLStatsCTF();
	void ParseQLScoresOld();
	void ParseQLScoresDuelOld();

	u8 _playerIndices[64];
	udtGeneralAnalyzer _analyzer;
	udtVMArray<udtParseDataStats> _statsArray; // The final array.
	udtParseDataStats _stats;
	udtVMLinearAllocator _namesAllocator;
	CommandLineTokenizer* _tokenizer;
	udtProtocol::Id _protocol;
	bool _gameEnded;
};
