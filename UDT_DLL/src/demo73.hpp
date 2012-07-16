#include "demo.hpp"


struct Demo73 : Demo
{
public:
	static const netField_t EntityStateFields[];
	static const int EntityStateFieldCount;

public:
	Demo73();
	~Demo73();

	// Overrides.
	void ProtocolInit();
	void ProtocolParseBaseline(msg_t* msg, msg_t* msgOut);
	void ProtocolParsePacketEntities(msg_t* msg, msg_t* msgOut, clSnapshot_t* oldframe, clSnapshot_t* newframe);
	void ProtocolEmitPacketEntities(clSnapshot_t* from, clSnapshot_t* to);
	void ProtocolAnalyzeConfigString(int csIndex, const std::string& input);
	void ProtocolFixConfigString(int csIndex, const std::string& input, std::string& output);
	void ProtocolAnalyzeAndFixCommandString(const char* command, std::string& output);
	void ProtocolAnalyzeSnapshot(const clSnapshot_t* oldSnap, const clSnapshot_t* newSnap);

public:
	void AnalyzePlayerInfo(int clientNum, const std::string& configString);

	EntityVector73 _inParseEntities; // Fixed-size array of size MAX_PARSE_ENTITIES.
	EntityVector73 _inEntityBaselines; // Fixed-size array of size MAX_PARSE_ENTITIES. Must be zeroed initially.
};