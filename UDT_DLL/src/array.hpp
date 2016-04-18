#pragma once


#include "linear_allocator.hpp"

#include <string.h> // For memmove.
#include <assert.h>


//
// A resizeable array class that only handles POD data types.
//
template<typename T>
struct udtVMArray
{
	udtVMArray(uptr reservedByteCount, const char* allocatorName)
		: _size(0)
	{
		_allocator.Init(reservedByteCount, allocatorName);
	}

	udtVMArray() : 
		_size(0)
	{
	}

	~udtVMArray()
	{
	}

	
	void Init(uptr reservedByteCount, const char* allocatorName)
	{
		_allocator.Init(reservedByteCount, allocatorName);
	}

	void InitNoOverride(uptr reservedByteCount, const char* allocatorName)
	{
		_allocator.InitNoOverride(reservedByteCount, allocatorName);
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

		_allocator.DisableReserveOverride();
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

private:
	UDT_NO_COPY_SEMANTICS(udtVMArray);

	udtVMLinearAllocator _allocator;
	u32 _size;
};
