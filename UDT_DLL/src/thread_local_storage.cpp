#include "thread_local_storage.hpp"


#if defined(UDT_WINDOWS)


#include <Windows.h>


ThreadLocalStorage::ThreadLocalStorage() 
	: _slot(TLS_OUT_OF_INDEXES)
{
}

ThreadLocalStorage::~ThreadLocalStorage()
{
	if(IsValid())
	{
		TlsFree(_slot);
	}
}

bool ThreadLocalStorage::AllocateSlot()
{
	_slot = TlsAlloc();

	return IsValid();
}

bool ThreadLocalStorage::IsValid() const
{
	return _slot != TLS_OUT_OF_INDEXES;
}

void* ThreadLocalStorage::GetData()
{
	return TlsGetValue(_slot);
}

bool ThreadLocalStorage::SetData(void* data)
{
	return TlsSetValue(_slot, data) != 0;
}


#endif