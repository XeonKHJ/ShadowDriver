#include "ShadowFilterWindowsSpecific.h"

ShadowFilterContext* ShadowFilterContext::InitializeShadowFilterContext()
{
	auto context = (ShadowFilterContext*)ExAllocatePoolWithTag(NonPagedPool, sizeof(ShadowFilterContext), 'sfci');
	return context;
}

void ShadowFilterContext::DeleteShadowFilterContext(ShadowFilterContext* pContext)
{
	ExFreePoolWithTag(pContext, 'sfci');
}

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
