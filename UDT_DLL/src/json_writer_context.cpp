#include "json_writer_context.hpp"


udtJSONWriterContext::udtJSONWriterContext()
{
	_initialized = false;
}

udtJSONWriterContext::~udtJSONWriterContext()
{
}

void udtJSONWriterContext::ResetForNextDemo()
{
	if(!_initialized)
	{
		MemoryStream.Open(1 << 24);
		Writer.SetOutputStream(&MemoryStream);
		_initialized = true;
	}
	else
	{
		MemoryStream.Clear();
	}
}
