#include "driver.h"
#include "device.tmh"
#include "Callouts.h"

UINT32 WpsCalloutId;
UINT32 WpmCalloutId;

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
    FWPS_STREAM_DATA* streamData;
    SIZE_T length = 0;
    SIZE_T bytes;
    CHAR dataBuffer[202] = { 0 };
    CHAR outputs[400] = { 0 };

    packet = (FWPS_STREAM_CALLOUT_IO_PACKET*)layerData;

    if (packet != NULL)
    {
        streamData = packet->streamData;

        RtlZeroMemory(classifyOut, sizeof(FWPS_CLASSIFY_OUT));

        classifyOut->actionType = FWP_ACTION_PERMIT;

        if (streamData->flags & FWPS_STREAM_FLAG_RECEIVE)
        {
            
            length = streamData->dataLength <= 200 ? streamData->dataLength : 200;
            //dataBuffer = ExAllocatePoolWithTag(NonPagedPool, length, 'pac');

            //memset(dataBuffer, 0, length);

            FwpsCopyStreamDataToBuffer0(streamData, dataBuffer, length, &bytes);

            ConvertBytesArrayToHexString(dataBuffer, length, outputs, 400);

            DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL,"%s", outputs);

            //ExFreePoolWithTag(dataBuffer, 'pac');
        }
    }
}

NTSTATUS NTAPI NotifyFn(
    _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    _In_ const GUID* filterKey,
    _Inout_ FWPS_FILTER0* filter)
{
    return STATUS_SUCCESS;
}

void NTAPI FlowDeleteFn(
    _In_ UINT16 layerId,
    _In_ UINT32 calloutId,
    _In_ UINT64 flowContext)
{
    return;
}

NTSTATUS RegisterCalloutFuntions(IN PDEVICE_OBJECT deviceObject)
{
    NTSTATUS status;

    FWPS_CALLOUT0 callout = { 0 };

    callout.calloutKey = WFP_ESTABLISHED_CALLOUT_GUID;
    callout.flags = 0;
    callout.classifyFn = ClassifyFn;
    callout.notifyFn = NotifyFn;
    callout.flowDeleteFn = FlowDeleteFn;
    status = FwpsCalloutRegister0(deviceObject, &callout, &WpsCalloutId);

    return status;
}

NTSTATUS AddCalloutToWfp(IN HANDLE engineHandle)
{
    FWPM_CALLOUT0 callout = { 0 };
    callout.flags = 0;
    callout.displayData.description = L"I think you know what it is.";
    callout.displayData.name = L"ShadowCallout";
    callout.calloutKey = WFP_ESTABLISHED_CALLOUT_GUID;
    callout.applicableLayer = FWPM_LAYER_STREAM_V4;

    return FwpmCalloutAdd0(engineHandle, &callout, NULL, &WpmCalloutId);
}

VOID UnRegisterCallout(HANDLE engineHandle)
{
    if (WpmCalloutId != 0)
    {
        FwpmCalloutDeleteById(engineHandle, WpmCalloutId);
    }

    if (WpsCalloutId != 0)
    {
        FwpsCalloutUnregisterById0(WpsCalloutId);
    }
}