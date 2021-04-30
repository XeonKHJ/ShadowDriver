#include "WfpHelper.h"
#include "ShadowCallouts.h"
#include <ShadowDriverStatus.h>

WfpHelper::~WfpHelper()
{
#ifdef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "Deleting a WfpHelper instance.\t\n");
#endif
	for (UINT8 code = 0; code < ShadowFilterContext::FilterIdMaxNumber; ++code)
	{
		
		if (_groupCounts[code] > 0)
		{
			auto conditions = _conditionsByCode[code];
			auto addresses = _addressesByCode[code];
			auto conditionCount = _groupCounts[code];
			auto wpmConditions = _fwpmConditionsByCode[code];

			// Delete address;
			for (int i = 0; i < conditionCount; ++i)
			{
				auto wpmCondition = &(wpmConditions[i]);
				switch (wpmCondition->conditionValue.type)
				{
				case FWP_BYTE_ARRAY6_TYPE:
					delete wpmCondition->conditionValue.byteArray6;
					break;
				case FWP_V4_ADDR_MASK:
					delete wpmCondition->conditionValue.v4AddrMask;
					break;
				default:
					break;
				}
			}

			// Delete condtion array that was allocated in ConvertToFwpmCondition.
			delete[] conditions;
			delete[] wpmConditions;
		}

	}

}

/// <summary>
/// 总共有8种；2个层（网络层链路层）*2个方向（进和出）。
/// 0代表网络层，1代表链路层。接着0代表进，1代表出。
/// 这样就有00为网络层进，01为网络层出。如果是网络层，接下来那一位代表IPv4（0）或IPv6（1）
/// 10为链路层进，11为链路层出。
/// </summary>
UINT8  WfpHelper::CalculateFilterLayerAndPathCode(NetFilteringCondition* currentCondition)
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
			filterLayerAndPathCode = filterLayerAndPathCode + (0 << 2);
		}
		break;
		case IpAddrFamily::IPv6:
		{
			filterLayerAndPathCode = filterLayerAndPathCode + (1 << 2);
		}
		break;
		}
	}
	break;
	}
	switch (currentCondition->FilterPath)
	{
	case NetPacketDirection::Out:
		filterLayerAndPathCode = filterLayerAndPathCode + (0 << 1);
		break;
	case NetPacketDirection::In:
		filterLayerAndPathCode = filterLayerAndPathCode + (1 << 1);
		break;
	}

	return filterLayerAndPathCode;
}

GUID  WfpHelper::GetLayerKeyByCode(UINT8 code)
{
	GUID guid = { 0 };
	switch (code)
	{
	case 0:
		guid = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
		break;
	case 1:
		// 链路层出口过滤
		// Untested
		guid = FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET;
		break;
	case 2:
		//网络层IPv4接收过滤
		guid = FWPM_LAYER_INBOUND_IPPACKET_V4;
		break;
	case 3:
		// 链路层接收过滤
		// Untested
		guid = FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET;
		break;
	case 4:
		// 网络层IPv6发送过滤
		// Untested
		guid = FWPM_LAYER_OUTBOUND_IPPACKET_V6;
		break;
	case 5:
		// 无意义
		break;
	case 6:
		// 网络层IPv6接收过滤
		// Untested
		guid = FWPM_LAYER_INBOUND_IPPACKET_V6;
		break;
	case 7:
		break;
	}
	return guid;
}

NTSTATUS WfpHelper::InitializeWfpEngine(ShadowFilterContext* context)
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

NTSTATUS WfpHelper::InitializeSublayer(ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;
	FWPM_SUBLAYER0 sublayer = { 0 };
	sublayer.displayData.name = L"ShadowDriverFilterSublayer";
	sublayer.displayData.description = L"ShadowDriver Sublayer";
	sublayer.subLayerKey = context->SublayerGuid;
	sublayer.weight = FWPM_WEIGHT_RANGE_MAX; //65500
	status = FwpmSubLayerAdd0(context->WfpEngineHandle, &sublayer, NULL);
	return status;
}

NTSTATUS WfpHelper::AddFwpmFiltersToWpf(ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;
	for (UINT8 code = 0; code < ShadowFilterContext::FilterIdMaxNumber && NT_SUCCESS(status); ++code)
	{
		if (_groupCounts[code] > 0)
		{
			FWPM_FILTER0 wpmFilter = { 0 };
			wpmFilter.displayData.name = L"ShadowDriverFilter";
			wpmFilter.displayData.description = L"ShadowDriver's filter";
			wpmFilter.subLayerKey = context->SublayerGuid;
			wpmFilter.layerKey = GetLayerKeyByCode(code);
			wpmFilter.weight.type = FWP_EMPTY;
			wpmFilter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
			wpmFilter.flags = FWPM_FILTER_FLAG_NONE;
			wpmFilter.rawContext = (UINT64)context;
			wpmFilter.action.calloutKey = context->CalloutGuids[code];
			wpmFilter.numFilterConditions = _groupCounts[code];
			wpmFilter.filterCondition = _fwpmConditionsByCode[code];
			status = FwpmFilterAdd0(context->WfpEngineHandle, &wpmFilter, NULL, &(context->FilterIds[code]));
		}
	}
	return status;
}

NTSTATUS WfpHelper::AddCalloutsToWfp(_Inout_ ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;
	for (UINT8 code = 0; code < ShadowFilterContext::FilterIdMaxNumber && NT_SUCCESS(status); ++code)
	{
		if (_groupCounts[code] > 0)
		{
			FWPM_CALLOUT0 wpmCallout = { 0 };
			wpmCallout.flags = 0;
			wpmCallout.displayData.description = L"I think you know what it is.";
			wpmCallout.displayData.name = L"ShadowSendCallouts";
			wpmCallout.calloutKey = context->CalloutGuids[code];
			wpmCallout.applicableLayer = GetLayerKeyByCode(code);
			status = FwpmCalloutAdd0(context->WfpEngineHandle, &wpmCallout, NULL, &(context->WpmCalloutIds[code]));
		}
	}
	return status;
}

void WfpHelper::AddCalloutsAccrodingToCode(FWPS_CALLOUT0* callout, UINT8 code)
{
	switch (code)
	{
	case 0:
		callout->classifyFn = ShadowCallout::NetworkOutV4ClassifyFn;
		break;
	case 1:
		//链路层出口过滤
		callout->classifyFn = ShadowCallout::LinkOutClassifyFn;
		break;
	case 2:
		//网络层IPv4接收过滤
		callout->classifyFn = ShadowCallout::NetworkInV4ClassifyFn;
		break;
	case 3:
		//链路层接收过滤
		callout->classifyFn = ShadowCallout::LinkInClassifyFn;
		break;
	case 4:
		//网络层IPv6发送过滤
		// Untested
		callout->classifyFn = ShadowCallout::NetworkOutV6ClassifyFn;
		break;
	case 5:
		//无意义
		break;
	case 6:
		//网络层IPv6接收过滤
		// Untested
		callout->classifyFn = ShadowCallout::NetworkInV6ClassifyFn;
		break;
	case 7:
		break;
	}
}

FWPM_FILTER_CONDITION0 WfpHelper::ConvertToFwpmCondition(NetFilteringCondition* condition, _Inout_ NTSTATUS * status)
{
	FWPM_FILTER_CONDITION0 fwpmCondition{};
	switch (condition->FilterLayer)
	{
	case NetLayer::LinkLayer:
	{
		fwpmCondition.conditionValue.type = FWP_BYTE_ARRAY6_TYPE;
		auto macAddress = new FWP_BYTE_ARRAY6;
		fwpmCondition.conditionValue.byteArray6 = macAddress;
		RtlCopyMemory(macAddress, condition->MacAddress, FWP_BYTE_ARRAY6_SIZE);
		switch (condition->AddrLocation)
		{
		case AddressLocation::Local:
			fwpmCondition.fieldKey = FWPM_CONDITION_MAC_LOCAL_ADDRESS;
			break;
		case AddressLocation::Remote:
			fwpmCondition.fieldKey = FWPM_CONDITION_MAC_REMOTE_ADDRESS;
			break;
		}
	}
		break;
	case NetLayer::NetworkLayer:
	{
		switch (condition->IPAddressType)
		{
		case IpAddrFamily::IPv4:
		{
			fwpmCondition.conditionValue.type = FWP_V4_ADDR_MASK;
			auto v4AddrAndMask = new FWP_V4_ADDR_AND_MASK;
			RtlCopyMemory(&(v4AddrAndMask->addr), &(condition->IPv4Address), 4);
			RtlCopyMemory(&(v4AddrAndMask->mask), &(condition->IPv4Mask), 4);
			fwpmCondition.conditionValue.v4AddrMask = v4AddrAndMask;
			switch (condition->AddrLocation)
			{
			case AddressLocation::Local:
				fwpmCondition.fieldKey = FWPM_CONDITION_IP_LOCAL_ADDRESS_V4;
				break;
			case AddressLocation::Remote:
				fwpmCondition.fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS_V4;
				break;
			}
		}
			break;
		case IpAddrFamily::IPv6:
			*status = SHADOW_FILTER_NOT_IMPLEMENTED;
			break;
		default:
			*status = SHADOW_FILTER_NOT_IMPLEMENTED;
			break;
		}
	}
		break;
	}

	switch (condition->MatchType)
	{
	case FilterMatchType::Equal:
		fwpmCondition.matchType = FWP_MATCH_EQUAL;
		break;
	default:
		*status = SHADOW_FILTER_NOT_IMPLEMENTED;
		break;
	}

	return fwpmCondition;
}

NTSTATUS WfpHelper::AllocateConditionGroups(NetFilteringCondition* conditionsToGroup, int conditionCount)
{
	NTSTATUS status = STATUS_SUCCESS;
	struct NetFilteringConditionAndCode
	{
		NetFilteringCondition* Condition;
		int Index;
		int Code;
	};

	NetFilteringConditionAndCode* indicators = new NetFilteringConditionAndCode[conditionCount];
	for (int i = 0; i < conditionCount; ++i)
	{
		NetFilteringCondition* currentCondition = &(conditionsToGroup[i]);
		auto code = CalculateFilterLayerAndPathCode(currentCondition);
		NetFilteringConditionAndCode* indicator = &(indicators[i]);
		indicator->Code = code;
		indicator->Index = _groupCounts[code];
		indicator->Condition = currentCondition;
		++(_groupCounts[code]);
	}

	for (int currentCode = 0; currentCode < ShadowFilterContext::FilterIdMaxNumber; ++currentCode)
	{
		int currentCodeConditionCount = _groupCounts[currentCode];
		if (currentCodeConditionCount > 0)
		{
			_conditionsByCode[currentCode] = new NetFilteringCondition * [currentCodeConditionCount];
			_fwpmConditionsByCode[currentCode] = new FWPM_FILTER_CONDITION0[currentCodeConditionCount];
		}
	}

	for (int i = 0; i < conditionCount; ++i)
	{
		auto indicator = &(indicators[i]);
		_conditionsByCode[indicator->Code][indicator->Index] = indicator->Condition;
		_fwpmConditionsByCode[indicator->Code][indicator->Index] = ConvertToFwpmCondition(indicator->Condition, &status);
	}

	delete[] indicators;
	return status;
}

NTSTATUS WfpHelper::RegisterCalloutsToDevice(_Inout_ ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;
	for (UINT8 code = 0; code < ShadowFilterContext::FilterIdMaxNumber && NT_SUCCESS(status); ++code)
	{
		if (_groupCounts[code] > 0)
		{
			FWPS_CALLOUT0 wpsCallout = { 0 };
			wpsCallout.calloutKey = context->CalloutGuids[code];
			wpsCallout.flags = 0;
			wpsCallout.notifyFn = ShadowCallout::PacketNotify;
			wpsCallout.flowDeleteFn = ShadowCallout::PacketFlowDeleteNotfy;
			AddCalloutsAccrodingToCode(&wpsCallout, code);
			status = FwpsCalloutRegister0(context->DeviceObject, &wpsCallout, &(context->WpsCalloutIds[code]));
		}
	}
	return status;
}


