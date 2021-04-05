#include "driver.h"
#include "device.tmh"
#include "WfpHelper.h"
#include "ShadowFilter.h"
#include "ShadowFilterWindowsSpecific.h"

//ɸѡ����ʶ����
//�����ٵ��� NTSTATUS AddFileterToWfp(HANDLE engineHandler) ������Ż���ֵ��
//Ŀǰ���������ɾ�����ж�filter�Ƿ񱻳ɹ���ӵ�WFP
HANDLE EngineHandler;

NTSTATUS OpenWfpEngine()
{
	FWPM_SESSION0 session;
	FWPM_PROVIDER0 provider;

	memset(&session, 0, sizeof(session));

	session.displayData.name = L"ShadowDriver Session";
	session.txnWaitTimeoutInMSec = 0xFFFFFFFF;
	return FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, &EngineHandler);
}

ShadowFilterContext* _sfContext;
ShadowFilter* _shadowFilter;
NTSTATUS InitializeWfp(PDEVICE_OBJECT deviceObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	if (NT_SUCCESS(status))
	{
		_sfContext = new ShadowFilterContext();
		_shadowFilter = new ShadowFilter(_sfContext);
	}

	if (NT_SUCCESS(status))
	{
		NetFilteringCondition condition = NetFilteringCondition();
		condition.AddrLocation = AddressLocation::Remote;
		condition.FilterLayer = NetLayer::NetworkLayer;
		condition.FilterPath = NetPacketDirection::Out;
		condition.IPAddressType = IpAddrFamily::IPv4;
		condition.IPv4 = 0xC0A80166;
		condition.IPv4Mask = 0xFFFFFFFF;
		condition.MatchType = FilterMatchType::Equal;
		_shadowFilter->AddFilterCondition(&condition, 1);
	}

	if (NT_SUCCESS(status))
	{
		//_shadowFilter.StartFiltering();
	}

	return status;
}