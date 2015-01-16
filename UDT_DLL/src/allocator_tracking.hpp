#pragma once


#include "intrusive_list.hpp"
#include "thread_local_storage.hpp"


struct udtAllocatorTracker
{
	udtAllocatorTracker();
	~udtAllocatorTracker();

	void RegisterAllocator(udtIntrusiveListNode& node);
	void UnregisterAllocator(udtIntrusiveListNode& node);
	void GetAllocatorList(udtIntrusiveList*& list);

private:
	UDT_NO_COPY_SEMANTICS(udtAllocatorTracker);

	udtThreadLocalStorage _allocatorList;
};
