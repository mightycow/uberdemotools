#include "thread_local_allocators.hpp"
#include "thread_local_storage.hpp"
#include "assert_or_fatal.hpp"
#include "memory.hpp"

#include <stdlib.h>
#include <new>


static u8 ThreadLocalStorageBytes[sizeof(udtThreadLocalStorage)];
static udtThreadLocalStorage* ThreadLocalStorage = NULL;


struct ThreadLocalData
{
	udtVMLinearAllocator TempAllocator { "ThreadLocal::TempAllocator" };
};


void udtThreadLocalAllocators::Init()
{
	if(ThreadLocalStorage != NULL)
	{
		return;
	}

	new (ThreadLocalStorageBytes) udtThreadLocalStorage();
	ThreadLocalStorage = (udtThreadLocalStorage*)ThreadLocalStorageBytes;
	const bool slotAllocated = ThreadLocalStorage->AllocateSlot();
	UDT_ASSERT_OR_FATAL_MSG(slotAllocated, "Failed to allocate thread-local storage for allocators.");
}

void udtThreadLocalAllocators::Destroy()
{
	if(ThreadLocalStorage == NULL)
	{
		return;
	}

	ReleaseThreadLocalAllocators();
	ThreadLocalStorage->~udtThreadLocalStorage();
	ThreadLocalStorage = NULL;
}

udtVMLinearAllocator& udtThreadLocalAllocators::GetTempAllocator()
{
	UDT_ASSERT_OR_FATAL_MSG(ThreadLocalStorage != NULL, "You forgot to call udtInitLibrary.");

	ThreadLocalData* data = (ThreadLocalData*)ThreadLocalStorage->GetData();
	if(data != NULL)
	{
		return data->TempAllocator;
	}

	data = (ThreadLocalData*)udt_malloc(sizeof(ThreadLocalData));
	new (data) ThreadLocalData();
	ThreadLocalStorage->SetData(data);

	return data->TempAllocator;
}

void udtThreadLocalAllocators::ReleaseThreadLocalAllocators()
{
	UDT_ASSERT_OR_FATAL_MSG(ThreadLocalStorage != NULL, "You forgot to call udtInitLibrary.");

	ThreadLocalData* const data = (ThreadLocalData*)ThreadLocalStorage->GetData();
	if(data == NULL)
	{
		return;
	}

	ThreadLocalStorage->SetData(NULL);
	data->~ThreadLocalData();
	free(data);
}
