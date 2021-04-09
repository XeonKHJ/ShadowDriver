#include "ShadowFilterWindowsSpecific.h"

void* operator new(size_t size)
{
	void * newPool = ExAllocatePool2(NonPagedPool, size, 'oon');
	return newPool;
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
	void* newPool = ExAllocatePool2(NonPagedPool, size, 'oona');
	return newPool;
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
