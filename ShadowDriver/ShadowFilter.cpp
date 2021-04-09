//shadowcommon这玩意放在shadowfilter.h就没事
#include "ShadowFilterWindowsSpecific.h"
#include "ShadowCommon.h"
#include "ShadowFilter.h"
#include "ShadowCallouts.h"

NTSTATUS InitializeWfpEngine(ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;

	FWPM_SESSION0 session;
	FWPM_PROVIDER0 provider;
	memset(&session, 0, sizeof(session));
	session.displayData.name = L"ShadowDriver Session";
	session.txnWaitTimeoutInMSec = 0xFFFFFFFF;
	status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, &(context->WfpEngineHandle));;
	return status;
}

NTSTATUS InitializeSublayer(ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;
	FWPM_SUBLAYER0 sublayer = { 0 };
	sublayer.displayData.name = L"ShadowDriverFilterSublayer";
	sublayer.displayData.description = L"ShadowDriver Sublayer";
	sublayer.subLayerKey = SHADOWDRIVER_WFP_SUBLAYER_GUID;
	sublayer.weight = FWPM_WEIGHT_RANGE_MAX; //65500
	status = FwpmSubLayerAdd0(context->WfpEngineHandle, &sublayer, NULL);
	return status;
}

ShadowFilter::ShadowFilter(void* enviromentContexts)
{
	_context = enviromentContexts;
	ShadowFilterContext* shadowFilterContext = NULL;
	NetPacketFilteringCallout = NULL;
	_conditionCount = 0;
	_filteringConditions = nullptr;
	NTSTATUS status = STATUS_SUCCESS;
	if (_context)
	{
		shadowFilterContext = (ShadowFilterContext*)_context;
	}
	else
	{
		status = STATUS_ABANDONED;
	}
	if (!NT_SUCCESS(status))
	{
		this->~ShadowFilter();
	}
}

ShadowFilter::~ShadowFilter()
{
	ShadowFilterContext* shadowFilterContext = (ShadowFilterContext*)_context;

	StopFiltering();

	//删除动态分配的内存区域
	delete[] _filteringConditions;
}

/*++++++++++++++++++++++++++++++++++++为添加过滤条件做准备的代码++++++++++++++++++++++++++++++++++++++++++++*/
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
	case NetPacketDirection::Out:
		filterLayerAndPathCode = (filterLayerAndPathCode << 1) + 0;
		break;
	case NetPacketDirection::In:
		filterLayerAndPathCode = (filterLayerAndPathCode << 1) + 1;
		break;
	}

	return filterLayerAndPathCode;
}

inline GUID GetFilterCalloutGuid(UINT8 code)
{
	GUID guid = { 0 };
	switch (code)
	{
	case 0:
		guid = SHADOWDRIVER_WFP_NETWORK_IPV4_SEND_ESTABLISHED_CALLOUT_GUID;
		break;
	case 1:
		//链路层出口过滤
		break;
	case 2:
		//网络层IPv4接收过滤
		guid = SHADOWDRIVER_WFP_NETWORK_IPV4_RECEIVE_ESTABLISHED_CALLOUT_GUID;
		break;
	case 3:
		//链路层接收过滤
		break;
	case 4:
		//网络层IPv6发送过滤
		guid = SHADOWDRIVER_WFP_NETWORK_IPV6_SEND_ESTABLISHED_CALLOUT_GUID;
		break;
	case 5:
		//无意义
		break;
	case 6:
		//网络层IPv6接收过滤
		guid = SHADOWDRIVER_WFP_NETWORK_IPV6_RECEIVE_ESTABLISHED_CALLOUT_GUID;
		break;
	case 7:
		break;
	}
	return guid;
}

inline GUID GetLayerKeyByCode(UINT8 code)
{
	GUID guid = { 0 };
	switch (code)
	{
	case 0:
		guid = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
		break;
	case 1:
		//链路层出口过滤
		break;
	case 2:
		//网络层IPv4接收过滤
		guid = FWPM_LAYER_INBOUND_IPPACKET_V4;
		break;
	case 3:
		//链路层接收过滤
		break;
	case 4:
		//网络层IPv6发送过滤
		guid = FWPM_LAYER_OUTBOUND_IPPACKET_V6;
		break;
	case 5:
		//无意义
		break;
	case 6:
		//网络层IPv6接收过滤
		guid = FWPM_LAYER_INBOUND_IPPACKET_V6;
		break;
	case 7:
		break;
	}
	return guid;
}

inline void AddCalloutsAccrodingToCode(FWPS_CALLOUT0* callout, UINT8 code)
{
	switch (code)
	{
	case 0:
		callout->classifyFn = NetworkOutV4ClassifyFn;
		break;
	case 1:
		//链路层出口过滤
		break;
	case 2:
		//网络层IPv4接收过滤
		callout->classifyFn = NetworkInV4ClassifyFn;
		break;
	case 3:
		//链路层接收过滤
		break;
	case 4:
		//网络层IPv6发送过滤
		callout->classifyFn = NetworkOutV6ClassifyFn;
		break;
	case 5:
		//无意义
		break;
	case 6:
		//网络层IPv6接收过滤
		callout->classifyFn = NetworkInV6ClassifyFn;
		break;
	case 7:
		break;
	}
}

NTSTATUS RegisterCalloutFuntionsAccrodingToCode(ShadowFilterContext* context, UINT8 currentCode)
{
	NTSTATUS status = STATUS_SUCCESS;

	FWPS_CALLOUT0 wpsCallout = { 0 };
	UINT32 calloutId;
	wpsCallout.calloutKey = GetFilterCalloutGuid(currentCode);
	wpsCallout.flags = 0;
	wpsCallout.notifyFn = PacketNotify;
	wpsCallout.flowDeleteFn = PacketFlowDeleteNotfy;
	AddCalloutsAccrodingToCode(&wpsCallout, currentCode);
	status = FwpsCalloutRegister0(context->DeviceObject, &wpsCallout, &(context->WpsCalloutIds[currentCode]));
	return status;
}

NTSTATUS AddCalloutToWfpAcrrodingToCode(ShadowFilterContext* context, UINT8 currentCode)
{
	NTSTATUS status;
	FWPM_CALLOUT0 wpmCallout = { 0 };
	wpmCallout.flags = 0;
	wpmCallout.displayData.description = L"I think you know what it is.";
	wpmCallout.displayData.name = L"ShadowSendCallouts";
	wpmCallout.calloutKey = GetFilterCalloutGuid(currentCode);
	wpmCallout.applicableLayer = GetLayerKeyByCode(currentCode);
	status = FwpmCalloutAdd0(context->WfpEngineHandle, &wpmCallout, NULL, &(context->WpmCalloutIds[currentCode]));
	return status;
}

NTSTATUS AddFilterConditionAndFilter(ShadowFilterContext* context, NetFilteringCondition* conditions, int length)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT64 filterIds[ShadowFilterContext::FilterIdMaxNumber] = { 0 };
	GUID* filterCalloutGuid[ShadowFilterContext::FilterIdMaxNumber];
	if (conditions != nullptr && length != 0 && context != nullptr)
	{
		int conditionCounts[ShadowFilterContext::FilterIdMaxNumber] = { 0 };
		FWPM_FILTER_CONDITION0* wpmConditonsGroupByFilterLayer[ShadowFilterContext::FilterIdMaxNumber] = { 0 };
		NetFilteringConditionAndCode* wpmConditionAndCodes = new NetFilteringConditionAndCode[length];
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
		for (UINT8 currentCode = 0; currentCode < ShadowFilterContext::FilterIdMaxNumber; ++currentCode)
		{
			if (conditionCounts[currentCode] != 0)
			{
				wpmConditonsGroupByFilterLayer[currentCode] = new FWPM_FILTER_CONDITION0[conditionCounts[currentCode]];
				//如果内存分配错误则将状态置为错误并且跳出循环
				if(wpmConditonsGroupByFilterLayer[currentCode] == nullptr)
				{
					status = STATUS_ERROR_PROCESS_NOT_IN_JOB;
					break;
				}

				RegisterCalloutFuntionsAccrodingToCode(context, currentCode);
				AddCalloutToWfpAcrrodingToCode(context, currentCode);
			}
		}
		if (NT_SUCCESS(status))
		{
			//添加条件
			for (int currentNo = 0; currentNo < length; currentNo++)
			{
				FWP_V4_ADDR_AND_MASK v4AddrAndMask = { 0 };
				FWP_V6_ADDR_AND_MASK v6AddrAndMask = { 0 };
				NetFilteringConditionAndCode* currentConditionAndCodes = &(wpmConditionAndCodes[currentNo]);
				NetFilteringCondition* currentCondition = currentConditionAndCodes->CurrentCondition;
				FWPM_FILTER_CONDITION0* currentWpmCondition = NULL;
				if (wpmConditonsGroupByFilterLayer[currentConditionAndCodes->Code] != NULL)
				{
					currentWpmCondition = &(wpmConditonsGroupByFilterLayer[currentConditionAndCodes->Code][currentConditionAndCodes->Index]);
				}
				else
				{
					status = STATUS_ABANDONED;
				}

				switch (currentCondition->FilterLayer)
				{
				case NetLayer::LinkLayer:
				{
					//还未实现
					status = STATUS_ABANDONED;
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
						v4AddrAndMask.addr = currentCondition->IPv4;
						v4AddrAndMask.mask = currentCondition->IPv4Mask;
						currentWpmCondition->conditionValue.v4AddrMask = &v4AddrAndMask;
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
		for (UINT8 currentCode = 0; currentCode < ShadowFilterContext::FilterIdMaxNumber && NT_SUCCESS(status); ++currentCode)
		{
			if (conditionCounts[currentCode] != 0)
			{
				FWPM_FILTER0 wpmFilter = { 0 };
				wpmFilter.displayData.name = L"ShadowDriverFilter";
				wpmFilter.displayData.description = L"ShadowDriver's filter";
				wpmFilter.subLayerKey = SHADOWDRIVER_WFP_SUBLAYER_GUID;
				wpmFilter.layerKey = GetLayerKeyByCode(currentCode);
				wpmFilter.weight.type = FWP_EMPTY;
				wpmFilter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
				wpmFilter.flags = FWPM_FILTER_FLAG_NONE;
				wpmFilter.rawContext = (UINT64)context;
				wpmFilter.action.calloutKey = GetFilterCalloutGuid(currentCode);
				wpmFilter.numFilterConditions = conditionCounts[currentCode];
				wpmFilter.filterCondition = wpmConditonsGroupByFilterLayer[currentCode];

				//一下这个status需要被验证
				status = FwpmFilterAdd0(context->WfpEngineHandle, &wpmFilter, NULL, &(filterIds[currentCode]));
			}
		}

		//清理动态分配的变量
		delete[]wpmConditionAndCodes;
		for (int i = 0; i < ShadowFilterContext::FilterIdMaxNumber; ++i)
		{
			if (conditionCounts[i] != 0)
			{
				delete[]wpmConditonsGroupByFilterLayer[i];
			}
		}
	}
	else
	{
		status = STATUS_NULL_LM_PASSWORD;
	}
	return (int)status;
}
/*-----------------------------------为添加过滤条件做准备的代码---------------------------------------------*/

int ShadowFilter::AddFilterCondition(NetFilteringCondition* conditions, int length)
{
	int statusCode = 0;
	if (conditions != nullptr && length != 0)
	{
		NetFilteringCondition* newConditions = new NetFilteringCondition[_conditionCount + length];
		NetFilteringCondition* oldConditions = _filteringConditions;
		for (int i = 0; i < _conditionCount; ++i)
		{
			newConditions[i] = oldConditions[i];
		}
		for (int i = 0; i < length; ++i)
		{
			newConditions[_conditionCount + i] = conditions[i];
		}

		_conditionCount += length;
		_filteringConditions = newConditions;

		if (oldConditions != nullptr)
		{
			delete[] oldConditions;
		}
	}
	else
	{
		statusCode = -1;
	}
	return statusCode;
}

int ShadowFilter::StartFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* shadowFilterContext = (ShadowFilterContext*)_context;

	if (NT_SUCCESS(status))
	{
		status = InitializeWfpEngine(shadowFilterContext);
	}

	if (NT_SUCCESS(status))
	{
		status = InitializeSublayer(shadowFilterContext);
	}

	if (NT_SUCCESS(status))
	{
		if (_conditionCount != 0 && _filteringConditions != nullptr)
		{
			status = AddFilterConditionAndFilter(shadowFilterContext, _filteringConditions, _conditionCount);
		}
	}
	return status;
}

/// <summary>
/// 用于停止过滤。
/// STATUS值待处理。
/// </summary>
/// <returns></returns>
int ShadowFilter::StopFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* shadowFilterContext = (ShadowFilterContext*)_context;

	//反注册回调函数
	for (UINT8 currentCode = 0; currentCode <= shadowFilterContext->FilterIdMaxNumber; ++currentCode)
	{
		if (shadowFilterContext->FilterIds[currentCode] != NULL)
		{
			//这个状态要检查
			status = FwpmFilterDeleteById0(shadowFilterContext->WfpEngineHandle, shadowFilterContext->FilterIds[currentCode]);
			FwpmCalloutDeleteById(shadowFilterContext->WfpEngineHandle, (shadowFilterContext->WpmCalloutIds)[currentCode]);
			(shadowFilterContext->WpmCalloutIds)[currentCode] = NULL;
			status = FwpsCalloutUnregisterById0((shadowFilterContext->WpsCalloutIds)[currentCode]);
			(shadowFilterContext->WpsCalloutIds)[currentCode] = NULL;
		}
	}
	status = FwpmSubLayerDeleteByKey0(shadowFilterContext->WfpEngineHandle, &SHADOWDRIVER_WFP_SUBLAYER_GUID);
	status = FwpmEngineClose0(shadowFilterContext->WfpEngineHandle);

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
