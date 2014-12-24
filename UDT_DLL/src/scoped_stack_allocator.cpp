#include "scoped_stack_allocator.hpp"


udtVMScopedStackAllocator::udtVMScopedStackAllocator(udtVMLinearAllocator& linearAllocator)
	: _linearAllocator(linearAllocator)
	, _finalizerList(NULL)
	, _oldUsedByteCount(linearAllocator.GetCurrentByteCount())
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

	_linearAllocator.SetCurrentByteCount(_oldUsedByteCount);
}

u8* udtVMScopedStackAllocator::Allocate(u32 byteCount)
{
	return _linearAllocator.Allocate(byteCount);
}

u8* udtVMScopedStackAllocator::GetObjectFromFinalizer(Finalizer* finalizer)
{
	return (u8*)(finalizer + 1);
}
