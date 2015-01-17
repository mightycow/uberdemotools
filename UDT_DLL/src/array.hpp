#pragma once


#include "linear_allocator.hpp"

#include <string.h> // For memmove.
#include <assert.h>


//
// A resizeable array class that:
// - only handles POD data types
// - never deallocates memory
// - never does array copies
//
template<typename T>
struct udtVMArray
{
	explicit udtVMArray(udtVMLinearAllocator& allocator)
		: _allocator(&allocator)
		, _objects((T*)allocator.GetStartAddress())
		, _size(0)
	{
	}

	udtVMArray()
		: _allocator(NULL)
		, _objects(NULL)
		, _size(0)
	{
	}

	virtual ~udtVMArray()
	{
	}

	void SetAllocator(udtVMLinearAllocator& allocator)
	{
		assert(_allocator == NULL);
		assert(_objects == NULL);
		assert(allocator.GetStartAddress() != NULL); // The allocator is not initialized yet?

		_allocator = &allocator;
		_objects = (T*)allocator.GetStartAddress();
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

	T* GetEndAddress()
	{
		return _objects + _size;
	}

	const T* GetEndAddress() const
	{
		return _objects + _size;
	}

	void Add(const T& object)
	{
		T* const objectPtr = (T*)_allocator->Allocate((uptr)sizeof(T));
		*objectPtr = object;
		++_size;
	}

	void Clear()
	{
		_allocator->Clear();
		_size = 0;
	}

	void Resize(u32 newObjectCount)
	{
		_allocator->Clear();
		_allocator->Allocate((uptr)sizeof(T) * (uptr)newObjectCount);
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
		_allocator->Pop((uptr)sizeof(T));
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
			_allocator->Pop((uptr)sizeof(T));
			return;
		}

		memmove(_objects + index, _objects + index + 1, (size_t)(_size - 1 - index) * sizeof(T));
		--_size;
		_allocator->Pop((uptr)sizeof(T));
	}

	// The array's size, i.e. the number of elements.
	u32 GetSize() const
	{
		return _size;
	}

	uptr GetReservedByteCount() const
	{
		return _allocator->GetCommittedByteCount();
	}

	bool IsEmpty() const
	{
		return _size == 0;
	}

private:
	UDT_NO_COPY_SEMANTICS(udtVMArray);

	udtVMLinearAllocator* _allocator;
	T* _objects;
	u32 _size;
};

template<typename T>
struct udtVMArrayWithAlloc : public udtVMArray<T>
{
public:
	explicit udtVMArrayWithAlloc(uptr reservedByteCount)
	{
		_allocator.Init(reservedByteCount);
		udtVMArray<T>::SetAllocator(_allocator);
	}

	udtVMArrayWithAlloc()
	{
	}

	void Init(uptr reservedByteCount)
	{
		_allocator.Init(reservedByteCount);
		udtVMArray<T>::SetAllocator(_allocator);
	}

private:
	UDT_NO_COPY_SEMANTICS(udtVMArrayWithAlloc);

	udtVMLinearAllocator _allocator;
};
