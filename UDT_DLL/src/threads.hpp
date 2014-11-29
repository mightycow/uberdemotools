#pragma once


#include "types.hpp"


struct udtThread
{
	typedef void (*ThreadEntryPoint)(void* userData);

	udtThread();
	~udtThread();

	bool CreateAndStart(ThreadEntryPoint entryPoint, void* userData);
	bool Wait();

	// Do not use directly.
	void InvokeUserFunction();

private:
	void* _threadhandle;
	void* _userData;
	ThreadEntryPoint _entryPoint;
};
