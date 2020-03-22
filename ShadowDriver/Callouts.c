#include "driver.h"
#include "Callouts.h"
#include "fwpsk.h"

UINT32 CalloutId;


VOID NTAPI ClassifyFn(
    _In_ const FWPS_INCOMING_VALUES0* inFixedValues,
    _In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    _Inout_opt_ void* layerData,
    _In_ const FWPS_FILTER0* filter,
    _In_ UINT64 flowContext,
    _Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
    FWPS_STREAM_CALLOUT_IO_PACKET* packet;
    
    //DbgPrintEx(DPFLTR_NETAPI_ID, DPFLTR_INFO_LEVEL, "First message.\n");

    packet = (FWPS_STREAM_CALLOUT_IO_PACKET*)layerData;

    RtlZeroMemory(classifyOut, sizeof(FWPS_CLASSIFY_OUT));

    packet->streamAction = FWPS_STREAM_ACTION_NONE;

    classifyOut->actionType = FWP_ACTION_PERMIT;

    if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
    {
        classifyOut->actionType &= FWPS_RIGHT_ACTION_WRITE;
    }
}

NTSTATUS NTAPI NotifyFn(
    _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    _In_ const GUID* filterKey,
    _Inout_ FWPS_FILTER0* filter)
{
    return 1;
}


void NTAPI FlowDeleteFn(
    _In_ UINT16 layerId,
    _In_ UINT32 calloutId,
    _In_ UINT64 flowContext)
{
    return;
}

const FWPS_CALLOUT0 Callout =
{
    { 0x61776eb9, 0xee7e, 0x46c3, { 0x9a, 0x23, 0x2a, 0x8c, 0x4c, 0x64, 0x7a, 0xe3 } },
    0,
    ClassifyFn,
    NotifyFn,
    FlowDeleteFn
};

NTSTATUS RegisterCalloutFuntions(PDEVICE_OBJECT deviceObject)
{
    NTSTATUS status;

    status = FwpsCalloutRegister0(deviceObject, &Callout, &CalloutId);

    return status;
}