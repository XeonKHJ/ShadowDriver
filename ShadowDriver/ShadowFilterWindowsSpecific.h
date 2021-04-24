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
	GUID SublayerGuid;
	GUID CalloutGuid[8];
	HANDLE WfpEngineHandle;
	HANDLE SendInjectHandle;
	HANDLE ReceiveInjectHandle;
	UINT32 WpsSendCalloutId;
	UINT32 WpsReceiveCalloutId;
	UINT64 FilterIds[FilterIdMaxNumber];
	UINT32 WpsCalloutIds[FilterIdMaxNumber];
	UINT32 WpmCalloutIds[FilterIdMaxNumber];
	bool IsModificationEnable;
	bool IsFilteringStarted;
	void (*NetPacketFilteringCallout)(NetLayer, NetPacketDirection, void*, unsigned long long);
	void* CustomContext;
}ShadowFilterContext;

void* operator new(size_t);
void operator delete(void *) noexcept;
void operator delete(void*, size_t);

void* operator new[](size_t);
void operator delete[](void*);
void operator delete[](void*, size_t);