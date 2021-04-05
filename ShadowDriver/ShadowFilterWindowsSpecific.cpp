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

void* operator new[](size_t size)
{
	return ExAllocatePoolWithTag(NonPagedPool, size, 'oona');;
}

void operator delete[](void* toFreeObject)
{
	ExFreePoolWithTag(toFreeObject, 'oona');
}

void operator delete[](void* toFreeObject, size_t)
{
	ExFreePoolWithTag(toFreeObject, 'oona');
}
