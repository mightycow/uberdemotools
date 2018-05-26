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
	void CopyBuffersStruct(void* buffersStruct) const override;
	void UpdateBufferStruct() override;
	u32  GetItemCount() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;
	void ClearMatchList();

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
	void ClearStats(bool newGameState = false);
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
	void ParseCPMAStats(bool endGameStats, bool newEncoding);
	void ParseCPMAXStats2();
	void ParseCPMAMStats();
	void ParseCPMAXStats2a();
	void ParseCPMAMStatsa();
	void ParseCPMADuelEndScores();
	void ParseCPMAXScores();
	void ParseCPMADMScores();
	void ParseQLScoresTDMVeryOld();
	void ParseQLScoresTDMOld();
	void ParseOSPStatsInfo();
	void ParseOSPXStats1();
	void ParseQLScoresCA();
	void ParseQLScoresCTFOld();
	void ParseQLScoresCAOld();
	void ParseQLStatsCA();
	void ParseQLScoresAD();
	void ParseQLScoresFT();
	void ParseQLScoresRROld();
	void ParseQLScoresRR();
	void ParsePrint();
	void ParseCPMAPrint();
	void ParseCPMAPrintHeader(const udtString& message);
	void ParseCPMAPrintStats(const udtString& message);
	void ParseCPMAPrintStatsPlayer(const udtString& message);
	void ParseCPMAPrintStatsTeam(const udtString& message);
	void ResetCPMAPrintStats();
	void ComputePlayerAccuracies(s32 clientNumber);
	void ComputePlayerAccuracy(s32 clientNumber, s32 acc, s32 hits, s32 shots);
	void ComputePlayerRocketSkill(s32 clientNumber);

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

	void SetPlayerField(s32 clientNumber, udtPlayerStatsField::Id fieldId, s32 value)
	{
		const udtStatsFieldValue field = { (s32)fieldId, value };
		SetFields(GetPlayerFlags(clientNumber), GetPlayerFields(clientNumber), &field, 1);
	}

	void SetTeamField(s32 teamIndex, udtTeamStatsField::Id fieldId, s32 value)
	{
		const udtStatsFieldValue field = { (s32)fieldId, value };
		SetFields(GetTeamFlags(teamIndex), GetTeamFields(teamIndex), &field, 1);
	}

	struct udtCPMAPrintStats
	{
		struct Header
		{
			u16 StringStart;
			u16 StringLength;
			s16 Field1;
			s16 Field2;
		};

		Header Headers[64];
		u32 HeaderCount;
		bool TeamStats;
	};

	u8 _playerIndices[64];
	udtGeneralAnalyzer _analyzer;
	udtVMArray<udtParseDataStats> _statsArray { "ParserPlugInStats::StatsArray" };
	udtParseDataStatsBuffers _buffers;
	udtParseDataStats _stats;
	udtCPMAPrintStats _cpmaPrintStats;
	udtPlayerStats _playerStats[64];
	s32 _playerTeamIndices[64];
	s32 _playerFields[64][udtPlayerStatsField::Count];
	s32 _teamFields[2][udtTeamStatsField::Count];
	u8 _playerFlags[64][UDT_PLAYER_STATS_MASK_BYTE_COUNT];
	u8 _teamFlags[2][UDT_TEAM_STATS_MASK_BYTE_COUNT];
	udtVMLinearAllocator _stringAllocator { "ParserPlugInStats::Strings" };
	udtString _redString;  // Allocated with _stringAllocator.
	udtString _blueString; // Allocated with _stringAllocator.
	udtVMArray<u8> _teamFlagsArray { "ParserPlugInStats::TeamFlagsArray" };
	udtVMArray<u8> _playerFlagsArray { "ParserPlugInStats::PlayerFlagsArray" };
	udtVMArray<s32> _teamFieldsArray { "ParserPlugInStats::TeamFieldsArray" };
	udtVMArray<s32> _playerFieldsArray { "ParserPlugInStats::PlayerFieldsArray" };
	udtVMArray<udtPlayerStats> _playerStatsArray {  "ParserPlugInStats::PlayerStatsArray" };
	udtVMArray<s32> _timeOutTimes { "ParserPlugInStats::TimeOutTimesArray" };
	const idTokenizer* _tokenizer;
	idTokenizer* _plugInTokenizer;
	udtProtocol::Id _protocol;
	s32 _followedClientNumber;
	s32 _firstPlaceClientNumber;
	s32 _secondPlaceClientNumber;
	s32 _cpmaScoreRed;
	s32 _cpmaScoreBlue;
	s32 _cpmaRoundScoreRed;
	s32 _cpmaRoundScoreBlue;
	s32 _cpma150DuelEndScoresState;
	s32 _firstPlaceScore;
	s32 _secondPlaceScore;
	s32 _lastMatchEndTime;
	bool _disableStatsOverrides;
	bool _cpma150ValidDuelEndScores;
	bool _cpma150Forfeit;
};
