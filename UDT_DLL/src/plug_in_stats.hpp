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

	struct udtStatsFieldValue
	{
		s32 Index;
		s32 Value;
	};

	s32  GetValue(s32 index);
	void ParseFields(u8* destMask, s32* destFields, const udtStatsField* fields, s32 fieldCount, s32 tokenOffset = 0);
	void SetFields(u8* destMask, s32* destFields, const udtStatsFieldValue* fields, s32 fieldCount);
	s64  CreateBitMask(const udtStatsField* fields, s32 fieldCount);
	void AddCurrentStats();
	void ClearStats();
	void ProcessConfigString(s32 csIndex, const udtString& configString);
	void ProcessPlayerConfigString(const char* configString, s32 playerIndex);
	bool GetClientNumberFromScoreIndex(s32& clientNumber, s32 fieldIndex);
	bool AreStatsValid();
	void ParseScores();
	void ParseQLScoresTDM();
	void ParseQLStatsTDM();
	void ParseQLScoresDuel();
	void ParseQLScoresCTF();
	void ParseQLStatsCTF();
	void ParseQLScoresOld();
	void ParseQLScoresDuelOld();
	void ParseQ3Scores();
	void ParseQ3ScoresDM3();
	void ParseCPMAXStats2();
	void ParseCPMAXScores();
	void ParseQLScoresTDMVeryOld();
	void ParseQLScoresTDMOld();
	void ParseOSPStatsInfo();
	void ComputePlayerAccuracies(s32 clientNumber);
	void ComputePlayerAccuracy(s32 clientNumber, s32 acc, s32 hits, s32 shots);

	u8*  GetTeamFlags(s32 idx) { return _teamFlags[idx]; }
	u8*  GetPlayerFlags(s32 idx) { return _playerFlags[idx]; }
	s32* GetTeamFields(s32 idx) { return _teamFields[idx]; }
	s32* GetPlayerFields(s32 idx) { return _playerFields[idx]; }

	void ParseTeamFields(s32 teamIndex, const udtStatsField* fields, s32 fieldCount, s32 tokenOffset = 0)
	{
		ParseFields(GetTeamFlags(teamIndex), GetTeamFields(teamIndex), fields, fieldCount, tokenOffset);
	}

	void ParsePlayerFields(s32 clientNumber, const udtStatsField* fields, s32 fieldCount, s32 tokenOffset = 0)
	{
		ParseFields(GetPlayerFlags(clientNumber), GetPlayerFields(clientNumber), fields, fieldCount, tokenOffset);
	}

	void SetTeamFields(s32 teamIndex, const udtStatsFieldValue* fields, s32 fieldCount)
	{
		SetFields(GetTeamFlags(teamIndex), GetTeamFields(teamIndex), fields, fieldCount);
	}

	void SetPlayerFields(s32 clientNumber, const udtStatsFieldValue* fields, s32 fieldCount)
	{
		SetFields(GetPlayerFlags(clientNumber), GetPlayerFields(clientNumber), fields, fieldCount);
	}

	u8 _playerIndices[64];
	udtGeneralAnalyzer _analyzer;
	udtVMArray<udtParseDataStats> _statsArray; // The final array.
	udtParseDataStats _stats;
	udtPlayerStats _playerStats[64];
	s32 _playerFields[64][udtPlayerStatsField::Count];
	s32 _teamFields[2][udtTeamStatsField::Count];
	u8 _playerFlags[64][UDT_PLAYER_STATS_MASK_BYTE_COUNT];
	u8 _teamFlags[2][UDT_TEAM_STATS_MASK_BYTE_COUNT];
	udtVMLinearAllocator _allocator;
	CommandLineTokenizer* _tokenizer;
	udtProtocol::Id _protocol;
	s32 _followedClientNumber;
	bool _gameEnded;
};
