#pragma once


#include "macros.hpp"


struct udtIntrusiveListNode
{
	udtIntrusiveListNode()
	{
		Next = NULL;
		Previous = NULL;
	}

	udtIntrusiveListNode* Next;
	udtIntrusiveListNode* Previous;
};

struct udtIntrusiveList 
{
	udtIntrusiveList()
	{
		Root.Next = &Root;
		Root.Previous = &Root;
	}

	udtIntrusiveListNode Root;
};

inline void InsertNodeAfter(udtIntrusiveListNode* node, udtIntrusiveListNode* prev)
{
	udtIntrusiveListNode* const next = prev->Next;
	node->Next = next;
	node->Previous = prev;
	next->Previous = node;
	prev->Next = node;
}

inline void RemoveNode(udtIntrusiveListNode* node)
{
	node->Next->Previous = node->Previous;
	node->Previous->Next = node->Next;
	node->Next = NULL;
	node->Previous = NULL;
}
