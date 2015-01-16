#include "scoped_stack_allocator.hpp"

#include <assert.h>


udtVMScopedStackAllocator::udtVMScopedStackAllocator(udtVMLinearAllocator& linearAllocator)
	: _linearAllocator(&linearAllocator)
	, _finalizerList(NULL)
	, _oldUsedByteCount(linearAllocator.GetCurrentByteCount())
{
}

udtVMScopedStackAllocator::udtVMScopedStackAllocator()
	: _linearAllocator(NULL)
	, _finalizerList(NULL)
	, _oldUsedByteCount(0)
{
}

udtVMScopedStackAllocator::~udtVMScopedStackAllocator()
{
	Finalizer* finalizer = _finalizerList;
	while(finalizer != NULL)
	{
		(*finalizer->Destructor)(GetObjectFromFinalizer(finalizer));
		finalizer = finalizer->Next;
	}

	_linearAllocator->SetCurrentByteCount(_oldUsedByteCount);
}

void udtVMScopedStackAllocator::SetAllocator(udtVMLinearAllocator& linearAllocator)
{
	assert(_linearAllocator == NULL);

	_linearAllocator = &linearAllocator;
	_oldUsedByteCount = linearAllocator.GetCurrentByteCount();
}

u8* udtVMScopedStackAllocator::Allocate(uptr byteCount)
{
	return _linearAllocator->Allocate(byteCount);
}

u8* udtVMScopedStackAllocator::GetObjectFromFinalizer(Finalizer* finalizer)
{
	return (u8*)(finalizer + 1);
}
