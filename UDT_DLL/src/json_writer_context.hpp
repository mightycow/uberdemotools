#pragma once


#include "json_writer.hpp"
#include "memory_stream.hpp"


struct udtJSONWriterContext
{
public:
	udtJSONWriterContext();
	~udtJSONWriterContext();

	void ResetForNextDemo(); // Called once per demo processed.

private:
	UDT_NO_COPY_SEMANTICS(udtJSONWriterContext);

public:
	udtVMMemoryStream MemoryStream;
	udtJSONWriter Writer;

private:
	bool _initialized;
};
