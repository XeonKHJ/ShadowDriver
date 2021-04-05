#include "ShadowFilterWindowsSpecific.h"

void* operator new(size_t size)
{
	return ExAllocatePoolWithTag(NonPagedPool, size, 'oon');;
}

void operator delete(void* toFreeObject) noexcept
{
	ExFreePoolWithTag(toFreeObject, 'oon');
}

void operator delete(void* toFreeObject, size_t size)
{
	ExFreePoolWithTag(toFreeObject, 'oon');
}
