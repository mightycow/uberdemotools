#pragma once


#include "linear_allocator.hpp"


template<typename T>
void DestructorCall(void* ptr)
{
	static_cast<T*>(ptr)->~T();
}

struct udtVMScopedStackAllocator
{
private:
	struct Finalizer
	{
		void (*Destructor)(void*);
		Finalizer* Next;
	};

public:
	explicit udtVMScopedStackAllocator(udtVMLinearAllocator& linearAllocator);
	udtVMScopedStackAllocator();
	~udtVMScopedStackAllocator();

	void SetAllocator(udtVMLinearAllocator& linearAllocator);

	u8* Allocate(uptr byteCount);

	template<typename T>
	T* NewObject()
	{
		// Allocate and construct.
		Finalizer* const finalizer = (Finalizer*)_linearAllocator->Allocate((uptr)sizeof(Finalizer));
		T* const result = new (_linearAllocator->Allocate(sizeof(T))) T;

		// Register the finalizer.
		finalizer->Destructor = &DestructorCall<T>;
		finalizer->Next = _finalizerList;
		_finalizerList = finalizer;

		return result;
	}

	template<typename T>
	T* NewPOD()
	{
		return new (_linearAllocator->Allocate(sizeof(T))) T;
	}

private:
	UDT_NO_COPY_SEMANTICS(udtVMScopedStackAllocator);

	u8* GetObjectFromFinalizer(Finalizer* finalizer);

	udtVMLinearAllocator* _linearAllocator;
	Finalizer* _finalizerList;
	uptr _oldUsedByteCount;
};
