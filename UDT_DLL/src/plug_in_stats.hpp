#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtParserPlugInStats : udtBaseParserPlugIn
{
public:
	udtParserPlugInStats()
	{
		_tokenizer = NULL;
		_protocol = udtProtocol::Invalid;
		_gameEnded = false;
		memset(&_stats, 0, sizeof(_stats));
	}

	~udtParserPlugInStats()
	{
	}

	void InitAllocators(u32 demoCount) override
	{
		FinalAllocator.Init((uptr)4 * (uptr)sizeof(udtParseDataStats) * (uptr)demoCount);
		_namesAllocator.Init((uptr)(1 << 14) * (uptr)demoCount);
		_statsArray.SetAllocator(FinalAllocator);
	}

	u32 GetElementSize() const override
	{
		return (u32)sizeof(udtParseDataStats);
	};

	void StartDemoAnalysis()
	{
		_tokenizer = NULL;
		_protocol = udtProtocol::Invalid;
		_gameEnded = false;
		memset(&_stats, 0, sizeof(_stats));
	}

	void FinishDemoAnalysis()
	{
		AddCurrentStats();
	}

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;

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
	void ParseQLScoresTDM();
	void ParseQLStatsTDM();
	void ParseQLScoresDuel();
	void ParseQLScoresCTF();
	void ParseQLStatsCTF();

	u8 _playerIndices[64];
	udtVMArray<udtParseDataStats> _statsArray; // The final array.
	udtParseDataStats _stats;
	udtVMLinearAllocator _namesAllocator;
	CommandLineTokenizer* _tokenizer;
	udtProtocol::Id _protocol;
	bool _gameEnded;
};
