#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "file_stream.hpp"


struct udtParserPlugInQuakeToUDT : udtBaseParserPlugIn
{
public:
	udtParserPlugInQuakeToUDT();
	~udtParserPlugInQuakeToUDT();

	bool ResetForNextDemo(udtProtocol::Id protocol);
	void SetOutputStream(udtStream* output);

	void InitAllocators(u32 demoCount) override;

	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInQuakeToUDT);

private:
	void WriteSnapshot(udtBaseParser& parser, idClientSnapshotBase& snapshot);

	struct udtdClientEntity
	{
		idLargestEntityState EntityState;
		bool Valid;
	};

	struct udtdSnapshot
	{
		udtdClientEntity Entities[MAX_GENTITIES];
		s32 ServerTime;
	};

	struct udtdData
	{
		udtdSnapshot Snapshots[2];
		s32 SnapshotReadIndex;
		s32 LastSnapshotTimeMs;
	};

	udtVMLinearAllocator _allocator { "ParserPlugInQuakeToUDT::Data" };
	udtStream* _outputFile;
	udtdData* _data;
	udtProtocol::Id _protocol;
	u32 _protocolSizeOfEntityState;
	u32 _protocolSizeOfPlayerState;
	bool _firstSnapshot;
};
