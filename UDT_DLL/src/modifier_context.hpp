#pragma once


#include "converter_entity_timer_shifter.hpp"
#include "memory_stream.hpp"


struct udtModifierContext
{
public:
	udtModifierContext();
	~udtModifierContext();

	void ResetForNextDemo(); // Called once per demo processed.

private:
	UDT_NO_COPY_SEMANTICS(udtModifierContext);

private:
	void InitIfNeeded();

public:
	udtdConverter Converter;
	udtdEntityTimeShifterPlugIn TimeShifterPlugIn;
	udtVMMemoryStream WriteStream;
	udtVMLinearAllocator TempAllocator { "ModifierContext::Temp" };
};
