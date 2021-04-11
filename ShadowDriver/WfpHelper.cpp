#include "driver.h"
#include "device.tmh"
#include "WfpHelper.h"
#include "ShadowFilter.h"
#include "ShadowFilterWindowsSpecific.h"
#include "IOCTLHelper.h"

//筛选器标识符。
//仅有再调用 NTSTATUS AddFileterToWfp(HANDLE engineHandler) 函数后才会有值。
//目前好像就用来删除和判断filter是否被成功添加到WFP
HANDLE EngineHandler;
//ShadowFilter* IOCTLHelper::Filter;
NTSTATUS OpenWfpEngine()
{
	FWPM_SESSION0 session;
	FWPM_PROVIDER0 provider;

	memset(&session, 0, sizeof(session));

	session.displayData.name = L"ShadowDriver Session";
	session.txnWaitTimeoutInMSec = 0xFFFFFFFF;
	return FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, &EngineHandler);
}

void FilterFunc(NetLayer netLayer, NetPacketDirection direction, void* buffer, unsigned long long bufferSize)
{
	IOCTLHelper::NotifyUserApp(buffer, bufferSize);
}

ShadowFilterContext* _sfContext;
ShadowFilter* _shadowFilter;
NTSTATUS InitializeWfp(PDEVICE_OBJECT deviceObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	if (NT_SUCCESS(status))
	{
		_sfContext = new ShadowFilterContext();
		ShadowFilterContext* sfContext = _sfContext;
		_sfContext->DeviceObject = deviceObject;
		_sfContext->NetPacketFilteringCallout = FilterFunc;
		_shadowFilter = new ShadowFilter(_sfContext);
		IOCTLHelper::Filter = _shadowFilter;
	}

	return status;
}