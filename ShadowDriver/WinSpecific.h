#pragma once
#include <wdm.h>

typedef struct ShadowFilterContext
{
	PDEVICE_OBJECT DeviceObject;
	HANDLE WfpEngineHandle;
	HANDLE SendInjectHandle;
	HANDLE ReceiveInjectHandle;
	UINT32 WpsSendCalloutId;
	UINT32 WpsReceiveCalloutId;
	//��ʼ��WFP�õ�
	FWPM_SESSION0 WfpSession;
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