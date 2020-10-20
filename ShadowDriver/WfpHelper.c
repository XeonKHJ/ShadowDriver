#include "driver.h"
#include "device.tmh"
#include "WfpHelper.h"

//Windows筛选平台引擎的句柄。
//仅有再调用 NTSTATUS OpenWfpEngine() 成功后才会有值。
static HANDLE EngineHandler = NULL;

//筛选器标识符。
//仅有再调用 NTSTATUS AddFileterToWfp(HANDLE engineHandler) 函数后才会有值。
//目前好像就用来删除和判断filter是否被成功添加到WFP
static UINT64 filterId; 
static UINT64 filterId2;

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

    sublayer.displayData.name = L"ShadowDriverFilterSublayer";
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
    NTSTATUS status;

    FWPM_FILTER0 sendFilter = { 0 };
    FWPM_FILTER_CONDITION0 condition[1] = { 0 };

    FWP_V4_ADDR_AND_MASK AddrandMask = { 0 };
    AddrandMask.addr = 0xC0A800B1;
    AddrandMask.mask = 0xFFFFFFFF;

    sendFilter.displayData.name = L"ShadowDriveFilter";
    sendFilter.displayData.description = L"ShadowDriver's filter";
    sendFilter.layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
    sendFilter.subLayerKey = WFP_SUBLAYER_GUID;
    sendFilter.weight.type = FWP_EMPTY;
    sendFilter.numFilterConditions = 1;
    sendFilter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
    sendFilter.action.calloutKey = WFP_SEND_ESTABLISHED_CALLOUT_GUID;
    sendFilter.filterCondition = condition;

    condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
    condition[0].matchType = FWP_MATCH_EQUAL;
    condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
    condition[0].conditionValue.v4AddrMask = &AddrandMask;

    status = FwpmFilterAdd0(engineHandler, &sendFilter, NULL, &filterId);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Added Send Filter to WPF.\n");

    FWP_V4_ADDR_AND_MASK AddrandMask2 = { 0 };
    AddrandMask2.addr = 0xC0A80167;
    AddrandMask2.mask = 0xFFFFFFFF;

    FWPM_FILTER0 receiveFilter = { 0 };
    FWPM_FILTER_CONDITION0 condition2[1] = { 0 };
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

    status = FwpmFilterAdd0(engineHandler, &receiveFilter, NULL, 0);

    if (NT_SUCCESS(status))
    {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Added Receive Filter to WPF.\n");
    }

    return status;
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