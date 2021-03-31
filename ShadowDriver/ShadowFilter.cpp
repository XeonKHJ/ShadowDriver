#include "ShadowFilter.h"
#include <fwpmk.h>
#include <fwpsk.h>
#include "WinSpecific.h"
#include "ShadowCommon.h"

HANDLE EngineHandler = NULL;

NTSTATUS InitializeWfpEngine(ShadowFilterContext *context)
{
	auto pSession = &(context->WfpSession);
	FWPM_SESSION0 session;
	FWPM_PROVIDER0 provider;
	memset(&session, 0, sizeof(session));
	session.displayData.name = L"ShadowDriver Session";
	session.txnWaitTimeoutInMSec = 0xFFFFFFFF;
	//status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, pEngineHandler);;
}

NTSTATUS RegisterCalloutFuntions(ShadowFilterContext *context, NetFilteringCondition *conditions)
{
	NTSTATUS status = STATUS_SUCCESS;
	NetFilteringCondition *conditons = context->if (conditions != nullptr)
	{
		FWPS_CALLOUT0 sendCallout = {0};
		sendCallout.calloutKey = WFP_SEND_ESTABLISHED_CALLOUT_GUID;
		sendCallout.flags = 0;
		sendCallout.classifyFn = ClassifyFn;
		sendCallout.notifyFn = NotifyFn;
		sendCallout.flowDeleteFn = FlowDeleteFn;
	}
}

ShadowFilter::ShadowFilter(void *enviromentContexts)
{
	_context = enviromentContexts;
	ShadowFilterContext *shadowFilterContext = NULL;
	NTSTATUS status;
	if (_context)
	{
		shadowFilterContext = (ShadowFilterContext *)_context;
		InitializeWfpEngine(shadowFilterContext);
	}
	else
	{
		//取消构造
	}
}

int ShadowFilter::AddFilterCondition(NetFilteringCondition *conditions, int length)
{
	if (conditions != nullptr)
	{
		NTSTATUS status = STATUS_SUCCESS;
		FWPM_FILTER_CONDITION0 *wpmConditions = (FWPM_FILTER_CONDITION0 *)ExAllocatePoolWithTag(NonPagedPool, sizeof(FWPM_FILTER_CONDITION0) * length, 'nfcs');
		memset(wpmConditions, 0, sizeof(FWPM_FILTER_CONDITION0) * length);
		FWPM_FILTER0 sendFilter = {0};
		for (int currentIndex = 0; currentIndex < length; ++currentIndex)
		{
			FWPM_FILTER0 wpmFilter = {0};
			FWP_V4_ADDR_AND_MASK addrandMask = {0};
			addrandMask.addr = conditions[currentIndex].IPv4;
			switch (conditions[currentIndex].FilterLayer)
			{
			case NetLayer::LinkLayer:

				break;
			case NetLayer::NetworkLayer:
			{
				switch (conditions[currentIndex].IPAddressType)
				{
				case IpAddrFamily::IPv4:
					break;
				case IpAddrFamily::IPv6:
					break;
				}
			}
			break;
			}
			FWPM_FILTER0 sendFilter = {0};
			FWPM_FILTER_CONDITION0 condition[1] = {0};
			FWP_V4_ADDR_AND_MASK AddrandMask = {0};
			AddrandMask.addr = 0xC0A80166;
			AddrandMask.mask = 0xFFFFFFFF;

			sendFilter.displayData.name = L"ShadowDriveFilter";
			sendFilter.displayData.description = L"ShadowDriver's filter";
			sendFilter.layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
			sendFilter.subLayerKey = SHADOWDRIVER_WFP_SUBLAYER_GUID;
			sendFilter.weight.type = FWP_EMPTY;
			sendFilter.numFilterConditions = 1;
			sendFilter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
			sendFilter.action.calloutKey = SHADOWDRIVER_WFP_SEND_ESTABLISHED_CALLOUT_GUID;
			sendFilter.filterCondition = condition;

			condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
			condition[0].matchType = FWP_MATCH_EQUAL;
			condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
			condition[0].conditionValue.v4AddrMask = &AddrandMask;

			status = FwpmFilterAdd0(engineHandler, &sendFilter, NULL, &filterId);

			UINT64 abc = filterId;

			if (!NT_SUCCESS(status))
			{
				return status;
			}

			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Added Send Filter to WPF.\n");

			FWP_V4_ADDR_AND_MASK AddrandMask2 = {0};
			AddrandMask2.addr = 0xC0A80166;
			AddrandMask2.mask = 0xFFFFFFFF;

			FWPM_FILTER0 receiveFilter = {0};
			FWPM_FILTER_CONDITION0 condition2[1] = {0};
			receiveFilter.displayData.name = L"ShadowDriveFilter";
			receiveFilter.displayData.description = L"ShadowDriver's filter";
			receiveFilter.layerKey = FWPM_LAYER_INBOUND_IPPACKET_V4;
			receiveFilter.subLayerKey = WFP_SUBLAYER_GUID;
			receiveFilter.weight.type = FWP_EMPTY;
			receiveFilter.numFilterConditions = 1;
			receiveFilter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
			receiveFilter.action.calloutKey = WFP_RECEIVE_ESTABLISHED_CALLOUT_GUID;
			receiveFilter.filterCondition = condition2;

			condition2[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
			condition2[0].matchType = FWP_MATCH_EQUAL;
			condition2[0].conditionValue.type = FWP_V4_ADDR_MASK;
			condition2[0].conditionValue.v4AddrMask = &AddrandMask2;

			status = FwpmFilterAdd0(engineHandler, &receiveFilter, NULL, &filterId2);
			UINT64 abc2 = filterId2;

			if (NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Added Receive Filter to WPF.\n");
			}
		}
	}
}

int ShadowFilter::StartFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext *shadowFilterContext = (ShadowFilterContext *)_context;
	if (!NT_SUCCESS(status))
	{
		status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &(shadowFilterContext->WfpSession), &(shadowFilterContext->WfpEngineHandle));
		;
	}

	if (!NT_SUCCESS(status))
	{
		status = RegisterCalloutFuntions(shadowFilterContext);
	}

	return status;
}

void ShadowFilter::EnablePacketModification()
{
}

void ShadowFilter::DisablePacketModification()
{
}
