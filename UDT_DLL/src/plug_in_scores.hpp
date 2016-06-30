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
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessMessageBundleEnd(const udtMessageBundleCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInScores);

	void ProcessPlayerConfigString(u32 index, const char* cs);
	void ProcessServerInfo();
	void ProcessCPMAScores();
	void ProcessCPMARoundScores();
	void DetectGameType();
	void DetectMod();
	void AddScore();
	void ParseScoresCommand(const idTokenizer& tokenizer);
	void ParseScoresCommandImpl(const idTokenizer& tokenizer, u32 statsPerPlayer);
	void GetGameStateNumbers();

	struct Player
	{
		udtString Name;
		u8 Present;
		u8 Team;
	};

	Player _players[64];
	udtVMLinearAllocator _stringAllocator;
	udtVMArray<udtParseDataScore> _scores;
	udtParseDataScoreBuffers _buffers;
	udtBaseParser* _parser;
	s32 _gameStateIndex;
	s32 _score1;
	s32 _score2;
	s32 _clientNumber1; // CPMA: first place client number
	s32 _clientNumber2; // CPMA: second place client number
	s32 _csIndexScore1;
	s32 _csIndexScore2;
	s32 _csIndexClient1;
	s32 _csIndexClient2;
	udtGameType::Id _gameType;
	udtProtocol::Id _protocol;
	udtMod::Id _mod;
	bool _scoreChanged;
};
