#pragma once


#include "linear_allocator.hpp"

#include <string.h> // For memmove.
#include <assert.h>


//
// A resizeable array class that handles all data types as POD.
//
template<typename T>
struct udtVMArray
{
	udtVMArray(const char* allocatorName)
		: _allocator(allocatorName)
		, _size(0)
	{
		HandleAlignment();
	}
	
	udtVMArray() : 
		_size(0)
	{
		HandleAlignment();
	}

	~udtVMArray()
	{
	}

	T& operator[](u32 index)
	{
		return *((T*)_allocator.GetStartAddress() + index);
	}

	const T& operator[](u32 index) const
	{
		return *((const T*)_allocator.GetStartAddress() + index);
	}

	T* GetStartAddress()
	{
		return (T*)_allocator.GetStartAddress();
	}

	const T* GetStartAddress() const
	{
		return (const T*)_allocator.GetStartAddress();
	}

	T* GetEndAddress()
	{
		return (T*)_allocator.GetStartAddress() + _size;
	}

	const T* GetEndAddress() const
	{
		return (const T*)_allocator.GetStartAddress() + _size;
	}

	void Add(const T& object)
	{
		T* const objectPtr = (T*)_allocator.AllocateAndGetAddress((uptr)sizeof(T));
		*objectPtr = object;
		++_size;
	}

	void Clear()
	{
		_allocator.Clear();
		_size = 0;
	}

	void Resize(u32 newSize)
	{
		const u32 oldSize = _size;
		if(newSize == oldSize)
		{
			return;
		}

		if(newSize < oldSize)
		{
			_allocator.SetCurrentByteCount((uptr)sizeof(T) * (uptr)newSize);
			_size = newSize;
			return;
		}

		_allocator.Allocate((uptr)sizeof(T) * (uptr)(newSize - oldSize));
		_size = newSize;
	}

	T* Extend(u32 itemsToAdd)
	{
		const u32 oldSize = GetSize();
		Resize(oldSize + itemsToAdd);

		return GetStartAddress() + oldSize;
	}

	T* ExtendAndMemset(u32 itemsToAdd, u8 value)
	{
		const u32 oldSize = GetSize();
		Resize(oldSize + itemsToAdd);
		T* const firstNewItem = GetStartAddress() + oldSize;
		memset(firstNewItem, (int)value, (size_t)itemsToAdd * sizeof(T));

		return firstNewItem;
	}

	void ExtendAndSet(u32 itemsToAdd, T value)
	{
		const u32 oldSize = GetSize();
		const u32 newSize = oldSize + itemsToAdd;
		Resize(newSize);
		T* const items = GetStartAddress();
		for(u32 i = oldSize; i < newSize; ++i)
		{
			items[i] = value;
		}

		return items + oldSize;
	}

	void RemoveUnordered(u32 index)
	{
		if(index >= _size)
		{
			return;
		}

		T* const objects = (T*)_allocator.GetStartAddress();
		objects[index] = objects[_size - 1];
		--_size;
		_allocator.Pop((uptr)sizeof(T));
	}

	void Remove(u32 index)
	{
		if(index >= _size)
		{
			return;
		}

		if(index == _size - 1)
		{
			--_size;
			_allocator.Pop((uptr)sizeof(T));
			return;
		}

		T* const objects = (T*)_allocator.GetStartAddress();
		memmove(objects + index, objects + index + 1, (size_t)(_size - 1 - index) * sizeof(T));
		--_size;
		_allocator.Pop((uptr)sizeof(T));
	}

	// The array's size, i.e. the number of elements.
	u32 GetSize() const
	{
		return _size;
	}

	uptr GetReservedByteCount() const
	{
		return _allocator.GetCommittedByteCount();
	}

	bool IsEmpty() const
	{
		return _size == 0;
	}

	void SetName(const char* name)
	{
		_allocator.SetName(name);
	}

private:
	UDT_NO_COPY_SEMANTICS(udtVMArray);

	// Very stupid trick to fool the compiler into not emitting a
	// "conditional expression is constant" warning.
	static size_t Identity(size_t x) { return x; }

	void HandleAlignment()
	{
		if(Identity(sizeof(T)) % sizeof(void*) != 0)
		{
			_allocator.SetAlignment(1);
		}
	}

	udtVMLinearAllocator _allocator;
	u32 _size;
};
