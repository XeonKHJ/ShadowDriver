#include "driver.h"
#include "device.tmh"
#include "WfpHelper.h"

//Windows筛选平台引擎的句柄。
//仅有再调用 NTSTATUS OpenWfpEngine() 成功后才会有值。
static HANDLE EngineHandler = NULL;

//筛选器标识符。
//仅有再调用 NTSTATUS AddFileterToWfp(HANDLE engineHandler) 函数后才会有值。
static UINT64 filterId; 
VOID UnInitWfp()
{
    if (EngineHandler != NULL)
    {
        if (filterId != 0)
        {
            FwpmFilterDeleteById0(EngineHandler, filterId);
            FwpmSubLayerDeleteByKey0(EngineHandler, &WFP_SUBLAYER_GUID);
        }

        UnRegisterCallout(EngineHandler);

        FwpmEngineClose0(EngineHandler); 
    }
}

NTSTATUS OpenWfpEngine()
{
    FWPM_SESSION0 session;
    FWPM_PROVIDER0 provider;

    memset(&session, 0, sizeof(session));
    
    session.displayData.name = L"ShadowDriver Session";
    session.txnWaitTimeoutInMSec = 0xFFFFFFFF;
    return FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, &EngineHandler);
}

NTSTATUS AddSublayerToWfp(HANDLE engineHandler)
{
    FWPM_SUBLAYER0 sublayer = { 0 };

    sublayer.displayData.name = L"ShadowDriverSublayer";
    sublayer.displayData.description = L"ShadowDriver Sublayer";
    sublayer.subLayerKey = WFP_SUBLAYER_GUID;
    sublayer.weight = FWPM_WEIGHT_RANGE_MAX; //65500
    return FwpmSubLayerAdd0(engineHandler, &sublayer, NULL);
}

/// <summary>
/// 为指定的WFP引擎添加过滤器
/// </summary>
/// <param name="engineHandler">WFP引擎句柄</param>
/// <returns>NT状态</returns>
NTSTATUS AddFilterToWfp(HANDLE engineHandler)
{
    FWPM_FILTER0 filter = { 0 };
    FWPM_FILTER_CONDITION0 condition[1] = { 0 };

    FWP_V4_ADDR_AND_MASK AddrandMask = { 0 };
    AddrandMask.addr = 0xC0A800C6;
    AddrandMask.mask = 0xFFFFFFFF;

    filter.displayData.name = L"ShadowDriveFilter";
    filter.displayData.description = L"ShadowDriver's filter";
    filter.layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
    filter.subLayerKey = WFP_SUBLAYER_GUID;
    filter.weight.type = FWP_EMPTY;
    filter.numFilterConditions = 1;
    filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
    filter.action.calloutKey = WFP_ESTABLISHED_CALLOUT_GUID;
    filter.filterCondition = condition;

    condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
    condition[0].matchType = FWP_MATCH_EQUAL;
    condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
    condition[0].conditionValue.v4AddrMask = &AddrandMask;

    return FwpmFilterAdd0(engineHandler, &filter, NULL, &filterId);
}

NTSTATUS InitializeWfp(PDEVICE_OBJECT deviceObject)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        status = OpenWfpEngine();
    }

    if (NT_SUCCESS(status))
    {
        status = RegisterCalloutFuntions(deviceObject);
    }

    if (NT_SUCCESS(status))
    {
        status = CreateInjector();
    }

    if (NT_SUCCESS(status))
    {
        status = AddCalloutToWfp(EngineHandler);
    }

    if (NT_SUCCESS(status))
    {
        status = AddSublayerToWfp(EngineHandler);
    }

    if (NT_SUCCESS(status))
    {
        status = AddFilterToWfp(EngineHandler);
    }

    if (!NT_SUCCESS(status))
    {
        UnInitWfp();
    }

    return status;
}