#pragma once


#include "common.hpp"
#include "linear_allocator.hpp"


struct udtConfigStringConversion
{
	const char* String; // Always valid.
	u32 StringLength;   // Always valid.
	s32 Index;          // Negative if needs to be dropped.
	bool NewString;     // True when a new string was created.
};

struct udtProtocolConverter
{
	udtProtocolConverter() {}
	virtual ~udtProtocolConverter() {}

	virtual void ResetForNextDemo() {}
	virtual void ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot) = 0;
	virtual void ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState) = 0;
	virtual void ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength) = 0;
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

	void ConvertSnapshot(idLargestClientSnapshot& outSnapshot, const idClientSnapshotBase& inSnapshot) override;
	void ConvertEntityState(idLargestEntityState& outEntityState, const idEntityStateBase& inEntityState) override;
	void ConvertConfigString(udtConfigStringConversion& result, udtVMLinearAllocator& allocator, s32 inIndex, const char* configString, u32 configStringLength) override;

private:
	UDT_NO_COPY_SEMANTICS(udtProtocolConverter90to68_CPMA);
};
