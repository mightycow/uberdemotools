#include "allocator_tracking.hpp"
#include "memory.hpp"

#include <stdlib.h>
#include <new>


udtAllocatorTracker::udtAllocatorTracker()
{
	_allocatorList.AllocateSlot();
}

udtAllocatorTracker::~udtAllocatorTracker()
{
	free(_allocatorList.GetData());
}

void udtAllocatorTracker::RegisterAllocator(udtIntrusiveListNode& node)
{
	udtIntrusiveList* allocators = (udtIntrusiveList*)_allocatorList.GetData();
	if(allocators == NULL)
	{
		allocators = (udtIntrusiveList*)udt_malloc(sizeof(udtIntrusiveList));
		if(!_allocatorList.SetData(allocators))
		{
			free(allocators);
			allocators = NULL;
		}
		else
		{
			new (allocators)udtIntrusiveList;
		}
	}

	if(allocators != NULL)
	{
		InsertNodeAfter(&node, &allocators->Root);
	}
}

void udtAllocatorTracker::UnregisterAllocator(udtIntrusiveListNode& node)
{
	if(node.Next != NULL && node.Previous != NULL)
	{
		RemoveNode(&node);
	}
}

void udtAllocatorTracker::GetAllocatorList(udtIntrusiveList*& list)
{
	list = (udtIntrusiveList*)_allocatorList.GetData();
}
