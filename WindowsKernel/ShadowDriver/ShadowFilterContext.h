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
	GUID CalloutGuids[8];
	HANDLE WfpEngineHandle;
	UINT32 WpsSendCalloutId;
	UINT32 WpsReceiveCalloutId;
	UINT64 FilterIds[FilterIdMaxNumber];
	UINT32 WpsCalloutIds[FilterIdMaxNumber];
	UINT32 WpmCalloutIds[FilterIdMaxNumber];
	bool IsModificationEnable;
	bool IsFilteringStarted;
	void (*NetPacketFilteringCallout)(NetLayer layer, NetPacketDirection direction, void * buffer, unsigned long long bufferSize, void * context);
	void* CustomContext;
	
}ShadowFilterContext;