#pragma once
#include <fwpsk.h>
#include <fwpmk.h>
#include "NetFilteringCondition.h"

//#define STATUS_FUNCTION_NOT_IMPLMENTED STATUS_SEVERITY_ERROR<<30 + 1<<29

typedef struct ShadowFilterContext
{
public:
	static const int FilterIdMaxNumber = 8;
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

	void (*NetPacketFilteringCallout)(NetLayer, NetPacketDirection, void*, unsigned long long);

	static ShadowFilterContext* InitializeShadowFilterContext();
	static void DeleteShadowFilterContext(ShadowFilterContext* pContext);
}ShadowFilterContext;