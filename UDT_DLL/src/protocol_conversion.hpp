#pragma once


#include "common.hpp"
#include "linear_allocator.hpp"
#include "math.hpp"


struct udtConfigStringConversion
{
	const char* String; // Always valid.
	u32 StringLength;   // Always valid.
	s32 Index;          // Negative if needs to be dropped.
	bool NewString;     // True when a new string was created.
};

struct udtProtocolConverter
{
	udtProtocolConverter() { ConversionInfo = NULL; Float3::Zero(Offsets); }
	
	virtual ~udtProtocolConverter() {}

	virtual void ResetForNextDemo() {}
	virtual void StartGameState() {}
	virtual void StartSnapshot(s32 /*serverTimeMs*/) {}
	virtual void ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot) = 0;
	virtual void ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState) = 0;
	virtual void ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength) = 0;

	const udtProtocolConversionArg* ConversionInfo;
	f32 Offsets[3];
};

struct udtProtocolConverterIdentity : public udtProtocolConverter
{
	udtProtocolConverterIdentity();

	void SetProtocol(udtProtocol::Id protocol);

	void ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot) override;
	void ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState) override;
	void ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength) override;

private:
	UDT_NO_COPY_SEMANTICS(udtProtocolConverterIdentity);

	u32 _protocolSizeOfClientSnapshot;
	u32 _protocolSizeOfEntityState;
};

struct udtProtocolConverter73to90 : public udtProtocolConverter
{
	udtProtocolConverter73to90() {}

	void ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot) override;
	void ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState) override;
	void ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength) override;

private:
	UDT_NO_COPY_SEMANTICS(udtProtocolConverter73to90);
};

struct udtProtocolConverter90to68_CPMA : public udtProtocolConverter
{
	udtProtocolConverter90to68_CPMA() {}

	void StartGameState() override;
	void StartSnapshot(s32 serverTimeMs) override;
	void ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot) override;
	void ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState) override;
	void ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength) override;

private:
	UDT_NO_COPY_SEMANTICS(udtProtocolConverter90to68_CPMA);

	struct ShaftingPlayer
	{
		s32 FirstCellTimeMs;
		s32 FirstSoundTimeMs;
		u32 SnapshotSoundCounter;
	};

	struct SnapshotInfo
	{
		ShaftingPlayer Players[MAX_CLIENTS];
		s32 SnapshotTimeMs;
	};

	SnapshotInfo _snapshots[2];
	u32 _snapshotIndex;
};

struct udtProtocolConverter48to68 : public udtProtocolConverter
{
	udtProtocolConverter48to68() : _protocolNumber(0) {}

	void ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot) override;
	void ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState) override;
	void ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength) override;

private:
	UDT_NO_COPY_SEMANTICS(udtProtocolConverter48to68);

	s32 _protocolNumber;
};

struct udtProtocolConverter3to68 : public udtProtocolConverter
{
	udtProtocolConverter3to68() {}

	void ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot) override;
	void ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState) override;
	void ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength) override;

private:
	UDT_NO_COPY_SEMANTICS(udtProtocolConverter3to68);
};
