#pragma once


#include "linear_allocator.hpp"

#include <string.h> // For memmove.


//
// A resizeable array class that:
// - only handles POD data types
// - never deallocates memory
// - never does array copies
//
template<typename T>
struct udtVMArray
{
	explicit udtVMArray(u32 reservedByteCount = 1 << 24, u32 commitByteCountGranularity = 4096)
		: _objects(NULL)
		, _size(0)
	{
		_allocator.Init(reservedByteCount, commitByteCountGranularity);
		_objects = (T*)_allocator.GetStartAddress();
		_size = 0;
	}

	~udtVMArray()
	{
	}

	T& operator[](u32 index)
	{
		return _objects[index];
	}

	const T& operator[](u32 index) const
	{
		return _objects[index];
	}

	T* GetStartAddress()
	{
		return _objects;
	}

	const T* GetStartAddress() const
	{
		return _objects;
	}

	void Add(const T& object)
	{
		T* const objectPtr = (T*)_allocator.Allocate((u32)sizeof(T));
		*objectPtr = object;
		++_size;
	}

	void Clear()
	{
		_allocator.Clear();
		_size = 0;
	}

	void Resize(u32 newObjectCount)
	{
		_allocator.Clear();
		_allocator.Allocate((u32)sizeof(T) * newObjectCount);
		_size = newObjectCount;
	}

	void RemoveUnordered(u32 index)
	{
		if(index >= _size)
		{
			return;
		}

		_objects[index] = _objects[_size - 1];
		--_size;
		_allocator.Pop((u32)sizeof(T));
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
			_allocator.Pop((u32)sizeof(T));
			return;
		}

		memmove(_objects + index, _objects + index + 1, (size_t)(_size - 1 - index) * sizeof(T));
		--_size;
		_allocator.Pop((u32)sizeof(T));
	}

	// The array's size, i.e. the number of elements.
	u32 GetSize() const
	{
		return _size;
	}

	u32 GetReservedByteCount() const
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
	T* _objects;
	u32 _size;
};
