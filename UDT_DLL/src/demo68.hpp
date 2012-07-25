#include "demo.hpp"


struct Demo68 : Demo
{
public:
	static const netField_t EntityStateFields[];
	static const int EntityStateFieldCount;

public:
	Demo68();
	~Demo68();

	// Overrides.
	void ProtocolInit();
	void ProtocolParseBaseline(msg_t* msg, msg_t* msgOut);
	void ProtocolParsePacketEntities(msg_t* msg, msg_t* msgOut, clSnapshot_t* oldframe, clSnapshot_t* newframe);
	void ProtocolEmitPacketEntities(clSnapshot_t* from, clSnapshot_t* to);
	void ProtocolAnalyzeConfigString(int csIndex, const std::string& input);
	void ProtocolFixConfigString(int csIndex, const std::string& input, std::string& output);
	void ProtocolAnalyzeAndFixCommandString(const char* command, std::string& output);
	void ProtocolAnalyzeSnapshot(const clSnapshot_t* oldSnap, const clSnapshot_t* newSnap);
	void ProtocolGetScores(int& score1, int& score2);

	void AnalyzeConfigStringCpmaGameInfo(const std::string& input);
	void AnalyzeConfigStringCpmaRoundInfo(const std::string& input);

public:
	void AnalyzePlayerInfo(int clientNum, const std::string& configString);

	EntityVector68 _inParseEntities; // Fixed-size array of size MAX_PARSE_ENTITIES.
	EntityVector68 _inEntityBaselines; // Fixed-size array of size MAX_PARSE_ENTITIES. Must be zeroed initially.

	int _inTw;
	int _inTs;
	int _inSb;
	int _inSr;
	int _inTl;
	bool _writeStats;
};