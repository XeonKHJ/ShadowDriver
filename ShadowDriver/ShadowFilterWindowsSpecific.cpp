#include "ShadowFilterWindowsSpecific.h"

void* operator new(size_t size)
{
	return ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'oon');
}

void operator delete(void* toFreeObject) noexcept
{
	if (toFreeObject != nullptr)
	{
		ExFreePoolWithTag(toFreeObject, 'oon');
	}
}

void operator delete(void* toFreeObject, size_t size)
{
	if (toFreeObject != nullptr)
	{
		ExFreePoolWithTag(toFreeObject, 'oon');
	}
}

void* operator new[](size_t size)
{
	return ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'oona');
}

void operator delete[](void* toFreeObject)
{
	if (toFreeObject != nullptr)
	{
		ExFreePoolWithTag(toFreeObject, 'oona');
	}
}

void operator delete[](void* toFreeObject, size_t)
{
	if (toFreeObject != nullptr)
	{
		ExFreePoolWithTag(toFreeObject, 'oona');
	}
}
