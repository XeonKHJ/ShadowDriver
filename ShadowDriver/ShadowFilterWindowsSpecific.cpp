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
