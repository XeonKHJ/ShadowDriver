#pragma once
#include <fwpsk.h>
#include <fwpmk.h>

extern const int FilterIdMaxNumber = 8;

typedef struct ShadowFilterContext
{
	PDEVICE_OBJECT DeviceObject;
	HANDLE WfpEngineHandle;
	HANDLE SendInjectHandle;
	HANDLE ReceiveInjectHandle;
	UINT32 WpsSendCalloutId;
	UINT32 WpsReceiveCalloutId;
	//初始化WFP用的
	FWPM_SESSION0 WfpSession;
	UINT64 FilterIds[FilterIdMaxNumber];
}ShadowFilterContext;

ShadowFilterContext * InitializeShadowFilterContext()
{
	auto context = (ShadowFilterContext*)ExAllocatePoolWithTag(NonPagedPool, sizeof(ShadowFilterContext), 'sfci');
	return context;
}

void DeleteShadowFilterContext(ShadowFilterContext* pContext)
{
	ExFreePoolWithTag(pContext, 'sfci');
}