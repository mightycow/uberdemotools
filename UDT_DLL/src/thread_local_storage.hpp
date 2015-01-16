#pragma once


#include "types.hpp"


struct ThreadLocalStorage
{
	ThreadLocalStorage();
	~ThreadLocalStorage();

	bool  AllocateSlot();
	bool  IsValid() const;
	void* GetData();
	bool  SetData(void* data);

private:
	UDT_NO_COPY_SEMANTICS(ThreadLocalStorage);

#if defined(UDT_WINDOWS)
	u32 _slot;
#endif
};
