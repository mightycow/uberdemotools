#include "thread_local_storage.hpp"


#if defined(UDT_WINDOWS)


#include <Windows.h>


udtThreadLocalStorage::udtThreadLocalStorage() 
	: _slot(TLS_OUT_OF_INDEXES)
{
}

udtThreadLocalStorage::~udtThreadLocalStorage()
{
	if(IsValid())
	{
		TlsFree((DWORD)_slot);
	}
}

bool udtThreadLocalStorage::AllocateSlot()
{
	_slot = (u32)TlsAlloc();

	return IsValid();
}

bool udtThreadLocalStorage::IsValid() const
{
	return _slot != TLS_OUT_OF_INDEXES;
}

void* udtThreadLocalStorage::GetData()
{
	return TlsGetValue((DWORD)_slot);
}

bool udtThreadLocalStorage::SetData(void* data)
{
	return TlsSetValue((DWORD)_slot, data) != 0;
}


#elif defined(UDT_LINUX)


#include <pthread.h>


udtThreadLocalStorage::udtThreadLocalStorage() 
	: _slot(PTHREAD_KEYS_MAX)
{
}

udtThreadLocalStorage::~udtThreadLocalStorage()
{
	if(IsValid())
	{
		pthread_key_delete(_slot);
	}
}

bool udtThreadLocalStorage::AllocateSlot()
{
	pthread_key_create(&_slot, NULL);

	return IsValid();
}

bool udtThreadLocalStorage::IsValid() const
{
	return _slot != PTHREAD_KEYS_MAX;
}

void* udtThreadLocalStorage::GetData()
{
	return pthread_getspecific(_slot);
}

bool udtThreadLocalStorage::SetData(void* data)
{
	return pthread_setspecific(_slot, data) == 0;
}


#endif
