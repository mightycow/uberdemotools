#include "allocator_tracking.hpp"

#include <stdlib.h>
#include <new>


udtAllocatorTracker::udtAllocatorTracker()
{
	_allocatorList.AllocateSlot();
}

udtAllocatorTracker::~udtAllocatorTracker()
{
	void* const allocators = _allocatorList.GetData();
	if(allocators != NULL)
	{
		free(allocators);
	}
}

void udtAllocatorTracker::RegisterAllocator(udtIntrusiveListNode& node)
{
	udtIntrusiveList* allocators = (udtIntrusiveList*)_allocatorList.GetData();
	if(allocators == NULL)
	{
		allocators = (udtIntrusiveList*)malloc(sizeof(udtIntrusiveList));
		if(allocators != NULL)
		{
			if(!_allocatorList.SetData(allocators))
			{
				free(allocators);
				allocators = NULL;
			}
			else
			{
				new (allocators) udtIntrusiveList;
			}
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
