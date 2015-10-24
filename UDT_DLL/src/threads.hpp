#pragma once


#include "uberdemotools.h"


struct udtThread
{
	typedef void (*ThreadEntryPoint)(void* userData);

	udtThread();
	~udtThread();

	bool CreateAndStart(ThreadEntryPoint entryPoint, void* userData);
	bool Join();
	bool TimedJoin(u32 timeoutMs);
	void Release();

	// Do not use directly.
	void InvokeUserFunction();

private:
	void* _threadhandle;
	void* _userData;
	ThreadEntryPoint _entryPoint;
};
