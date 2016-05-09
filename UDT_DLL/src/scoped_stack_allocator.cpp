#include "scoped_stack_allocator.hpp"

#include <assert.h>


udtVMScopedStackAllocator::udtVMScopedStackAllocator(udtVMLinearAllocator& linearAllocator)
	: _linearAllocator(&linearAllocator)
	, _finalizerList(UDT_U32_MAX)
	, _oldUsedByteCount(linearAllocator.GetCurrentByteCount())
{
}

udtVMScopedStackAllocator::udtVMScopedStackAllocator()
	: _linearAllocator(NULL)
	, _finalizerList(UDT_U32_MAX)
	, _oldUsedByteCount(0)
{
}

udtVMScopedStackAllocator::~udtVMScopedStackAllocator()
{
	uptr finalizerOffset = _finalizerList;
	while(finalizerOffset != UDT_U32_MAX)
	{
		Finalizer* const finalizer = (Finalizer*)_linearAllocator->GetAddressAt(finalizerOffset);
		(*finalizer->Destructor)(GetObjectFromFinalizer(finalizer));
		finalizerOffset = finalizer->Next;
	}

	_linearAllocator->SetCurrentByteCount(_oldUsedByteCount);
}

void udtVMScopedStackAllocator::SetAllocator(udtVMLinearAllocator& linearAllocator)
{
	assert(_linearAllocator == NULL);

	_linearAllocator = &linearAllocator;
	_oldUsedByteCount = linearAllocator.GetCurrentByteCount();
}

u8* udtVMScopedStackAllocator::GetObjectFromFinalizer(Finalizer* finalizer)
{
	return (u8*)(finalizer + 1);
}
