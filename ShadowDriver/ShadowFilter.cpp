//shadowcommon���������shadowfilter.h��û��
#include "ShadowFilterWindowsSpecific.h"
#include "ShadowCommon.h"
#include "ShadowFilter.h"
#include "ShadowCallouts.h"

NTSTATUS InitializeWfpEngine(ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;

	auto pSession = &(context->WfpSession);
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
	NTSTATUS status = STATUS_SUCCESS;
	if (_context)
	{
		shadowFilterContext = (ShadowFilterContext*)_context;

		if (NT_SUCCESS(status))
		{
			status = InitializeWfpEngine(shadowFilterContext);
		}
		
		if (NT_SUCCESS(status))
		{
			status = InitializeSublayer(shadowFilterContext);
		}
		
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

//ShadowFilter::~ShadowFilter()
//{
//	ShadowFilterContext* shadowFilterContext = (ShadowFilterContext*)_context;
//	
//	for (UINT8 currentCode = 0; currentCode < ShadowFilterContext::FilterIdMaxNumber; ++currentCode)
//	{
//		if ((shadowFilterContext->CalloutIds)[currentCode] != 0)
//		{
//			FwpmCalloutDeleteById(shadowFilterContext->WfpEngineHandle, (shadowFilterContext->CalloutIds)[currentCode]);
//		}
//
//		if ((shadowFilterContext->CalloutIds)[currentCode] != 0)
//		{
//			FwpsCalloutUnregisterById0((shadowFilterContext->CalloutIds)[currentCode]);
//		}
//
//		if ((shadowFilterContext->FilterIds)[currentCode] != 0)
//		{
//			FwpmFilterDeleteById0(shadowFilterContext->WfpEngineHandle, (shadowFilterContext->FilterIds)[currentCode]);
//		}
//	}
//
//	FwpmSubLayerDeleteByKey0(shadowFilterContext->WfpEngineHandle, &SHADOWDRIVER_WFP_SUBLAYER_GUID);
//
//	FwpmEngineClose0(shadowFilterContext->WfpEngineHandle);
//
//	ShadowFilterContext::DeleteShadowFilterContext(shadowFilterContext);
//}

/*++++++++++++++++++++++++++++++++++++Ϊ��ӹ���������׼���Ĵ���++++++++++++++++++++++++++++++++++++++++++++*/
struct NetFilteringConditionAndCode
{
	NetFilteringCondition* CurrentCondition;
	int Code;
	int Index;
};

/// <summary>
/// �ܹ���8�֣�2���㣨�������·�㣩*2�����򣨽��ͳ�����
/// 0��������㣬1������·�㡣����0�������1�������
/// ��������00Ϊ��������01Ϊ�����������������㣬��������һλ����IPv4��0����IPv6��1��
/// 10Ϊ��·�����11Ϊ��·�����
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

inline GUID GetFilterCalloutGuid(UINT8 code)
{
	GUID guid = { 0 };
	switch (code)
	{
	case 0:
		guid = SHADOWDRIVER_WFP_NETWORK_IPV4_SEND_ESTABLISHED_CALLOUT_GUID;
		break;
	case 1:
		//��·����ڹ���
		break;
	case 2:
		//�����IPv4���չ���
		guid = SHADOWDRIVER_WFP_NETWORK_IPV4_RECEIVE_ESTABLISHED_CALLOUT_GUID;
		break;
	case 3:
		//��·����չ���
		break;
	case 4:
		//�����IPv6���͹���
		guid = SHADOWDRIVER_WFP_NETWORK_IPV6_SEND_ESTABLISHED_CALLOUT_GUID;
		break;
	case 5:
		//������
		break;
	case 6:
		//�����IPv6���չ���
		guid = SHADOWDRIVER_WFP_NETWORK_IPV6_RECEIVE_ESTABLISHED_CALLOUT_GUID;
		break;
	case 7:
		break;
	}
	return guid;
}

NTSTATUS RegisterCalloutFuntions(ShadowFilterContext* context, NetFilteringCondition* conditions)
{
	NTSTATUS status = STATUS_SUCCESS;
	for (UINT8 currentCode = 0; currentCode < ShadowFilterContext::FilterIdMaxNumber; ++currentCode)
	{
		FWPS_CALLOUT0 callout = { 0 };
		UINT32 calloutId;
		callout.calloutKey = GetFilterCalloutGuid(currentCode);
		callout.flags = 0;
		//callout.classifyFn = ClassifyFn;
		//callout.notifyFn = NotifyFn;
		//callout.flowDeleteFn = FlowDeleteFn;
		status = FwpsCalloutRegister0(context->DeviceObject, &callout, &(context->CalloutIds[currentCode]));
	}
	return status;
}

int ShadowFilter::AddFilterCondition(NetFilteringCondition* conditions, int length)
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* context = (ShadowFilterContext*)_context;
	UINT64 filterIds[ShadowFilterContext::FilterIdMaxNumber] = {0};
	GUID* filterCalloutGuid[ShadowFilterContext::FilterIdMaxNumber];
	if (conditions != nullptr && length != 0)
	{
		int conditionCounts[ShadowFilterContext::FilterIdMaxNumber] = { 0 };
		FWPM_FILTER_CONDITION0* wpmConditonsGroupByFilterLayer[ShadowFilterContext::FilterIdMaxNumber] = { 0 };
		NetFilteringConditionAndCode* wpmConditionAndCodes = (NetFilteringConditionAndCode*)ExAllocatePoolWithTag(NonPagedPool, sizeof(NetFilteringConditionAndCode) * length, 'nfcc');
		memset(wpmConditionAndCodes, 0, sizeof(NetFilteringConditionAndCode) * length);
		//����ÿ�����͵Ĺ���������������
		for (int currentIndex = 0; currentIndex < length; ++currentIndex)
		{
			NetFilteringCondition* currentCondition = &conditions[currentIndex];
			UINT8 filterLayerAndPathCode = CalculateFilterLayerAndPathCode(currentCondition);
			wpmConditionAndCodes[currentIndex].CurrentCondition = currentCondition;
			wpmConditionAndCodes[currentIndex].Code = filterLayerAndPathCode;
			wpmConditionAndCodes[currentIndex].Index = conditionCounts[filterLayerAndPathCode];
			conditionCounts[filterLayerAndPathCode]++;
		}

		//�����ڴ�
		for (int i = 0; i < ShadowFilterContext::FilterIdMaxNumber; ++i)
		{
			if (conditionCounts[i] != 0)
			{
				wpmConditonsGroupByFilterLayer[i] = (FWPM_FILTER_CONDITION0*)ExAllocatePoolWithTag(NonPagedPool, sizeof(FWPM_FILTER_CONDITION0) * conditionCounts[i], 'nfcs');
				if (wpmConditonsGroupByFilterLayer[i] != NULL)
				{
					memset(wpmConditonsGroupByFilterLayer[i], 0, sizeof(FWPM_FILTER_CONDITION0) * conditionCounts[i]);
				}
				//����ڴ���������״̬��Ϊ����������ѭ��
				else
				{
					status = STATUS_ERROR_PROCESS_NOT_IN_JOB;
					break;
				}
			}
		}
		if (!NT_SUCCESS(status))
		{
			//�������
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
					//��δʵ��
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
					//��δʵ��
					status = STATUS_NULL_LM_PASSWORD;
				}


			}
		}

		//ע�������
		for (UINT8 currentCode = 0; currentCode < ShadowFilterContext::FilterIdMaxNumber && NT_SUCCESS(status); ++currentCode)
		{
			if (conditionCounts[currentCode] != 0)
			{
				FWPM_FILTER0 wpmFilter = { 0 };
				wpmFilter.displayData.name = L"ShadowDriverFilter";
				wpmFilter.displayData.description = L"ShadowDriver's filter";
				wpmFilter.subLayerKey = SHADOWDRIVER_WFP_SUBLAYER_GUID;
				wpmFilter.weight.type = FWP_EMPTY;
				wpmFilter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
				wpmFilter.flags = FWPM_FILTER_FLAG_NONE;
				wpmFilter.rawContext = (UINT64)context;
				wpmFilter.action.calloutKey = GetFilterCalloutGuid(currentCode);
				wpmFilter.numFilterConditions = conditionCounts[currentCode];
				wpmFilter.filterCondition = wpmConditonsGroupByFilterLayer[currentCode];
				status = FwpmFilterAdd0(context->WfpEngineHandle, &wpmFilter, NULL, &(filterIds[currentCode]));
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
/*-----------------------------------Ϊ��ӹ���������׼���Ĵ���---------------------------------------------*/

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
	//���ע����֮��ġ�
	_isModificationEnabled = true;
}

void ShadowFilter::DisablePacketModification()
{
	_isModificationEnabled = false;
}
