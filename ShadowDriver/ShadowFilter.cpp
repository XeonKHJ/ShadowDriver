#include "ShadowFilter.h"
#include "ShadowCommon.h"
#include "ShadowFilterWindowsSpecific.h"

HANDLE EngineHandler = NULL;

NTSTATUS InitializeWfpEngine(ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;

	auto pSession = &(context->WfpSession);
	FWPM_SESSION0 session;
	FWPM_PROVIDER0 provider;
	memset(&session, 0, sizeof(session));
	session.displayData.name = L"ShadowDriver Session";
	session.txnWaitTimeoutInMSec = 0xFFFFFFFF;
	//status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, pEngineHandler);;

	return status;
}

NTSTATUS RegisterCalloutFuntions(ShadowFilterContext* context, NetFilteringCondition* conditions)
{
	NTSTATUS status = STATUS_SUCCESS;
	return status;
}

ShadowFilter::ShadowFilter(void* enviromentContexts)
{
	_context = enviromentContexts;
	ShadowFilterContext* shadowFilterContext = NULL;
	NTSTATUS status;
	if (_context)
	{
		shadowFilterContext = (ShadowFilterContext*)_context;
		InitializeWfpEngine(shadowFilterContext);
	}
	else
	{
		//取消构造

	}
}

int ShadowFilter::AddFilterCondition(NetFilteringCondition* conditions, int length)
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* context = (ShadowFilterContext*)_context;
	
	if (conditions != nullptr)
	{
		UINT64 filterId;
		FWPM_FILTER_CONDITION0* wpmConditions = (FWPM_FILTER_CONDITION0*)ExAllocatePoolWithTag(NonPagedPool, sizeof(FWPM_FILTER_CONDITION0) * length, 'nfcs');
		if (wpmConditions != 0)
		{
			memset(wpmConditions, 0, sizeof(FWPM_FILTER_CONDITION0) * length);
			FWPM_FILTER0 filter = { 0 };
			for (int currentIndex = 0; currentIndex < length; ++currentIndex)
			{
				GUID layerKey = { 0 };
				GUID conditionFieldKey = { 0 };
				FWPM_FILTER0 wpmFilter = { 0 };
				FWP_MATCH_TYPE matchType;
				FWP_V4_ADDR_AND_MASK addrandMask = { 0 };
				addrandMask.addr = conditions[currentIndex].IPv4;
				FWP_CONDITION_VALUE0 conditionValue;
				NetFilteringCondition * currentCondition = &conditions[currentIndex];
				

				switch (currentCondition->FilterLayer)
				{
				case NetLayer::LinkLayer:
				{
					//还未实现
					switch (currentCondition->FilterPath)
					{
					case NetPacketDirection::In:
						break;
					case NetPacketDirection::Out:
						break;
					}
				}
				break;
				case NetLayer::NetworkLayer:
				{
					switch (currentCondition->AddrLocation)
					{
					case AddressLocation::Local:
						conditionFieldKey = FWPM_CONDITION_IP_LOCAL_ADDRESS;
						break;
					case AddressLocation::Remote:
						conditionFieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
						break;
					}
					switch (currentCondition->IPAddressType)
					{
					case IpAddrFamily::IPv4:
					{
						conditionValue.type = FWP_V4_ADDR_MASK;
						switch (currentCondition->FilterPath)
						{
						case NetPacketDirection::In:
							layerKey = FWPM_LAYER_INBOUND_IPPACKET_V4;
							break;
						case NetPacketDirection::Out:
							layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
							break;
						}
					}
					break;
					case IpAddrFamily::IPv6:
					{
						conditionValue.type = FWP_V6_ADDR_MASK;
						switch (currentCondition->FilterPath)
						{
						case NetPacketDirection::In:
							layerKey = FWPM_LAYER_INBOUND_IPPACKET_V6;
							break;
						case NetPacketDirection::Out:
							layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V6;
							break;
						}
					}
					break;
					}
				}
				break;
				}
				wpmConditions[currentIndex].fieldKey = conditionFieldKey;
				wpmConditions[currentIndex].matchType = matchType;
				wpmConditions[currentIndex].conditionValue = conditionValue;
			}

			//整理过滤条件，尽量将多数条件放在一个FWPM_FILTER内
			
			
			FWPM_FILTER_CONDITION0 condition[1] = { 0 };
			FWP_V4_ADDR_AND_MASK AddrandMask = { 0 };
			status = FwpmFilterAdd0(context->WfpEngineHandle, &filter, NULL, &filterId);
		}
	}
	else
	{
		status = STATUS_NULL_LM_PASSWORD;
	}
	return (int)status;
}

int ShadowFilter::StartFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* shadowFilterContext = (ShadowFilterContext*)_context;
	if (!NT_SUCCESS(status))
	{
		//status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &(shadowFilterContext->WfpSession), &(shadowFilterContext->WfpEngineHandle));
		;
	}

	if (!NT_SUCCESS(status))
	{
		status = RegisterCalloutFuntions(shadowFilterContext, NULL);
	}
	return status;
}

void ShadowFilter::EnablePacketModification()
{
	//添加注入句柄之类的。
	_isModificationEnabled = true;
}

void ShadowFilter::DisablePacketModification()
{
	_isModificationEnabled = false;
}
