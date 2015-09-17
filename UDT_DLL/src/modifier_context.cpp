#include "modifier_context.hpp"


udtModifierContext::udtModifierContext()
{
	_initialized = false;
}

udtModifierContext::~udtModifierContext()
{
}

void udtModifierContext::ResetForNextDemo()
{
	InitIfNeeded();

	TempAllocator.Clear();
}

void udtModifierContext::InitIfNeeded()
{
	if(_initialized)
	{
		return;
	}
	_initialized = true;

	TempAllocator.Init(1 << 16, "ModifierContext::Temp");
	WriteStream.Open(1 << 20);
}
