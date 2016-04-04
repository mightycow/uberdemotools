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
		uptr Next;
	};

public:
	explicit udtVMScopedStackAllocator(udtVMLinearAllocator& linearAllocator);
	udtVMScopedStackAllocator();
	~udtVMScopedStackAllocator();

	void SetAllocator(udtVMLinearAllocator& linearAllocator);

	template<typename T>
	uptr NewObject()
	{
		// Allocate and construct.
		const uptr offset = _linearAllocator->Allocate((uptr)sizeof(Finalizer) + (uptr)sizeof(T));
		Finalizer* const finalizer = (Finalizer*)_linearAllocator->GetAddressAt(offset);
		new (finalizer + 1) T;

		// Register the finalizer.
		finalizer->Destructor = &DestructorCall<T>;
		finalizer->Next = _finalizerList;
		_finalizerList = offset;

		return offset + (uptr)sizeof(Finalizer);
	}

	template<typename T>
	uptr NewPOD()
	{
		const uptr offset = _linearAllocator->Allocate(sizeof(T));

		new (_linearAllocator->GetAddressAt(offset)) T;

		return offset;
	}

private:
	UDT_NO_COPY_SEMANTICS(udtVMScopedStackAllocator);

	u8* GetObjectFromFinalizer(Finalizer* finalizer);

	udtVMLinearAllocator* _linearAllocator;
	uptr _finalizerList;
	uptr _oldUsedByteCount;
};
