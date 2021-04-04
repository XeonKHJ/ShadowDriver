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

/*-------------------------------------------------------------------------------------------------------*/
struct NetFilteringConditionAndCode
{
	NetFilteringCondition* CurrentCondition;
	int Code;
	int Index;
};

/// <summary>
/// 总共有8种；2个层（网络层链路层）*2个方向（进和出）。
/// 0代表网络层，1代表链路层。接着0代表进，1代表出。
/// 这样就有00为网络层进，01为网络层出。如果是网络层，接下来那一位代表IPv4（0）或IPv6（1）
/// 10为链路层进，11为链路层出。
/// </summary>
inline UINT8 CalculateFilterLayerAndPathCode(NetFilteringCondition* currentCondition)
{
	UINT8 filterLayerAndPathCode = 0;
	switch (currentCondition->FilterLayer)
	{
	case NetLayer::LinkLayer:
	{
		filterLayerAndPathCode = 1;
	}
	break;
	case NetLayer::NetworkLayer:
	{
		filterLayerAndPathCode = 0;
		switch (currentCondition->IPAddressType)
		{
		case IpAddrFamily::IPv4:
		{
			filterLayerAndPathCode = (filterLayerAndPathCode << 1) + 0;
		}
		break;
		case IpAddrFamily::IPv6:
		{
			filterLayerAndPathCode = (filterLayerAndPathCode << 1) + 1;
		}
		break;
		}
	}
	break;
	}
	switch (currentCondition->FilterPath)
	{
	case NetPacketDirection::In:
		filterLayerAndPathCode = (filterLayerAndPathCode << 1) + 0;
		break;
	case NetPacketDirection::Out:
		filterLayerAndPathCode = (filterLayerAndPathCode << 1) + 1;
		break;
	}

	return filterLayerAndPathCode;
}

int ShadowFilter::AddFilterCondition(NetFilteringCondition* conditions, int length)
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* context = (ShadowFilterContext*)_context;
	UINT64 filterIds[8] = {0};
	if (conditions != nullptr && length != 0)
	{
		int conditionCounts[8] = { 0 };
		FWPM_FILTER_CONDITION0* wpmConditonsGroupByFilterLayer[8] = { 0 };
		NetFilteringConditionAndCode* wpmConditionAndCodes = (NetFilteringConditionAndCode*)ExAllocatePoolWithTag(NonPagedPool, sizeof(NetFilteringConditionAndCode) * length, 'nfcc');
		memset(wpmConditionAndCodes, 0, sizeof(NetFilteringConditionAndCode) * length);
		FWPM_FILTER0 filter = { 0 };
		//计算每个类型的过滤器条件的数量
		for (int currentIndex = 0; currentIndex < length; ++currentIndex)
		{
			NetFilteringCondition* currentCondition = &conditions[currentIndex];
			UINT8 filterLayerAndPathCode = CalculateFilterLayerAndPathCode(currentCondition);
			wpmConditionAndCodes[currentIndex].CurrentCondition = currentCondition;
			wpmConditionAndCodes[currentIndex].Code = filterLayerAndPathCode;
			wpmConditionAndCodes[currentIndex].Index = conditionCounts[filterLayerAndPathCode];
			conditionCounts[filterLayerAndPathCode]++;
		}

		//分配内存
		for (int i = 0; i < 8; ++i)
		{
			if (conditionCounts[i] != 0)
			{
				wpmConditonsGroupByFilterLayer[i] = (FWPM_FILTER_CONDITION0*)ExAllocatePoolWithTag(NonPagedPool, sizeof(FWPM_FILTER_CONDITION0) * conditionCounts[i], 'nfcs');
				if (wpmConditonsGroupByFilterLayer[i] != NULL)
				{
					memset(wpmConditonsGroupByFilterLayer[i], 0, sizeof(FWPM_FILTER_CONDITION0) * conditionCounts[i]);
				}
				//如果内存分配错误则将状态置为错误并且跳出循环
				else
				{
					status = STATUS_ERROR_PROCESS_NOT_IN_JOB;
					break;
				}
			}
		}
		if (!NT_SUCCESS(status))
		{
			//添加条件
			for (int currentNo = 0; currentNo < length; currentNo++)
			{
				NetFilteringConditionAndCode* currentConditionAndCodes = &(wpmConditionAndCodes[currentNo]);
				NetFilteringCondition* currentCondition = currentConditionAndCodes->CurrentCondition;
				FWPM_FILTER_CONDITION0* currentWpmCondition = &(wpmConditonsGroupByFilterLayer[currentConditionAndCodes->Code][currentConditionAndCodes->Index]);
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
						currentWpmCondition->fieldKey = FWPM_CONDITION_IP_LOCAL_ADDRESS;
						break;
					case AddressLocation::Remote:
						currentWpmCondition->fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
						break;
					}
					switch (currentCondition->IPAddressType)
					{
					case IpAddrFamily::IPv4:
					{
						(currentWpmCondition->conditionValue).type = FWP_V4_ADDR_MASK;
					}
					break;
					case IpAddrFamily::IPv6:
					{
						(currentWpmCondition->conditionValue).type = FWP_V6_ADDR_MASK;
					}
					break;
					}
				}
				break;
				}
				switch (currentCondition->MatchType)
				{
				case FilterMatchType::Equal:
					currentWpmCondition->matchType = FWP_MATCH_EQUAL;
					break;
				default:
					//还未实现
					status = STATUS_NULL_LM_PASSWORD;
				}


			}
		}

		//注册过滤器
		for (int currentCode = 0; currentCode < 8 && NT_SUCCESS(status); ++currentCode)
		{
			if (conditionCounts[currentCode] != 0)
			{
				FWPM_FILTER0 wpmFilter = { 0 };
				wpmFilter.displayData.name = L"ShadowDriverFilter";
				wpmFilter.displayData.description = L"ShadowDriver's filter";
				wpmFilter.subLayerKey = WFP_SUBLAYER_GUID;
				wpmFilter.weight.type = FWP_EMPTY;
				wpmFilter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
				//这个要根据实际情况来，还没写。
				wpmFilter.action.calloutKey = SHADOWDRIVER_WFP_NETWORK_IPV4_SEND_ESTABLISHED_CALLOUT_GUID;
				wpmFilter.numFilterConditions = conditionCounts[currentCode];
				wpmFilter.filterCondition = wpmConditonsGroupByFilterLayer[currentCode];
				status = FwpmFilterAdd0(context->WfpEngineHandle, &filter, NULL, &(filterIds[currentCode]));
			}

			else
			{
				break;
			}
		}
	}
	else
	{
		status = STATUS_NULL_LM_PASSWORD;
	}
	return (int)status;
}
/*--------------------------------------------------------------------------------------------------------*/

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
