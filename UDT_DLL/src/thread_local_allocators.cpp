#include "thread_local_allocators.hpp"
#include "thread_local_storage.hpp"
#include "crash.hpp"

#include <new>


static u8 ThreadLocalStorageBytes[sizeof(udtThreadLocalStorage)];
static udtThreadLocalStorage* ThreadLocalStorage = NULL;


struct ThreadLocalData
{
	ThreadLocalData()
	{
		TempAllocator.Init(1 << 16, "ThreadLocal::TempAllocator");
	}

	udtVMLinearAllocator TempAllocator;
};


void udtThreadLocalAllocators::Init()
{
	if(ThreadLocalStorage != NULL)
	{
		return;
	}

	new (ThreadLocalStorageBytes) udtThreadLocalStorage();
	ThreadLocalStorage = (udtThreadLocalStorage*)ThreadLocalStorageBytes;
	if(!ThreadLocalStorage->AllocateSlot())
	{
		FatalError(__FILE__, __LINE__, __FUNCTION__, "Failed to allocate thread-local storage for allocators.");
	}
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
	if(ThreadLocalStorage == NULL)
	{
		FatalError(__FILE__, __LINE__, __FUNCTION__, "You forgot to call udtInitLibrary.");
	}

	ThreadLocalData* data = (ThreadLocalData*)ThreadLocalStorage->GetData();
	if(data != NULL)
	{
		return data->TempAllocator;
	}

	data = (ThreadLocalData*)malloc(sizeof(ThreadLocalData));
	if(data == NULL)
	{
		FatalError(__FILE__, __LINE__, __FUNCTION__, "Failed to allocate memory for thread-local allocators.");
	}

	new (data) ThreadLocalData();
	ThreadLocalStorage->SetData(data);

	return data->TempAllocator;
}

void udtThreadLocalAllocators::ReleaseThreadLocalAllocators()
{
	if(ThreadLocalStorage == NULL)
	{
		FatalError(__FILE__, __LINE__, __FUNCTION__, "You forgot to call udtInitLibrary.");
	}

	ThreadLocalData* const data = (ThreadLocalData*)ThreadLocalStorage->GetData();
	if(data == NULL)
	{
		return;
	}

	ThreadLocalStorage->SetData(NULL);
	data->~ThreadLocalData();
	free(data);
}
