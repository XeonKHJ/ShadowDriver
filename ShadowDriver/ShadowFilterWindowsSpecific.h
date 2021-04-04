#pragma once
#include <fwpsk.h>
#include <fwpmk.h>

//#define STATUS_FUNCTION_NOT_IMPLMENTED STATUS_SEVERITY_ERROR<<30 + 1<<29

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
	UINT32 CalloutIds[FilterIdMaxNumber];
	BOOL IsModificationEnable;
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