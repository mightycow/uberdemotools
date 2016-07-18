#include "modifier_context.hpp"


udtModifierContext::udtModifierContext()
{
}

udtModifierContext::~udtModifierContext()
{
}

void udtModifierContext::ResetForNextDemo()
{
	InitIfNeeded();

	TempAllocator.Clear();
	WriteStream.Clear();
}

void udtModifierContext::InitIfNeeded()
{
}
