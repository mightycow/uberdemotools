#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"
#include "parser.hpp"


struct udtParserPlugInScores : udtBaseParserPlugIn
{
public:
	udtParserPlugInScores();
	~udtParserPlugInScores();

	void InitAllocators(u32 demoCount) override;
	void CopyBuffersStruct(void* buffersStruct) const override;
	void UpdateBufferStruct() override;
	u32  GetItemCount() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessMessageBundleEnd(const udtMessageBundleCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInScores);

	struct Player
	{
		udtString Clan; // For old Quake Live demos only.
		udtString Name;
		u8 Present;
		u8 Team;
	};

	void ProcessPlayerConfigString(u32 index, const char* cs);
	void ProcessServerInfo();
	void ProcessCPMAScores(s32 csIndex);
	void DetectGameType();
	void DetectMod();
	void AddScore();
	void GetScoresCPMA(udtParseDataScore& scores);
	void GetScoresQ3(udtParseDataScore& scores);
	void GetScoresQL(udtParseDataScore& scores);
	void GetScoreName(udtString& name, udtVMLinearAllocator& alloc, const Player& player);

	Player _players[64];
	udtVMLinearAllocator _stringAllocator { "ParserPlugInScores::Strings" };
	udtVMLinearAllocator _tempAllocator { "ParserPlugInScores::Temp" }; // For temporary storage of Quake Live score names.
	udtVMArray<udtParseDataScore> _scores { "ParserPlugInScores::ScoresArray" };
	udtParseDataScoreBuffers _buffers;
	udtBaseParser* _parser;
	s32 _gameStateIndex;
	s32 _score1;
	s32 _score2;
	s32 _clientNumber1; // For CPMA demos only: first place client number.
	s32 _clientNumber2; // For CPMA demos only: second place client number.
	s32 _followedNumber; // For non-CPMA Quake 3 demos only.
	s32 _followedScore;  // For non-CPMA Quake 3 demos only.
	s32 _firstSnapshotTimeMs;
	udtString _name1; // For Quake Live demos only.
	udtString _name2; // For Quake Live demos only.
	udtGameType::Id _gameType;
	udtProtocol::Id _protocol;
	udtMod::Id _mod;
	bool _scoreChanged;
};
