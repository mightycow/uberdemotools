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
		Writer.SetOutputStream(&MemoryStream);
		_initialized = true;
	}
	else
	{
		MemoryStream.Clear();
	}
}
