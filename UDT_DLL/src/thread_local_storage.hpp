#pragma once


#include "uberdemotools.h"
#include "macros.hpp"


#if defined(UDT_LINUX)
#include <sys/types.h> // For pthread_key_t.
#endif


struct udtThreadLocalStorage
{
	udtThreadLocalStorage();
	~udtThreadLocalStorage();

	bool  AllocateSlot();
	bool  IsValid() const;
	void* GetData();
	bool  SetData(void* data);

private:
	UDT_NO_COPY_SEMANTICS(udtThreadLocalStorage);

#if defined(UDT_WINDOWS)
	u32 _slot; // DWORD
#elif defined(UDT_LINUX)
	pthread_key_t _slot;
	bool _isValid;
#endif
};
