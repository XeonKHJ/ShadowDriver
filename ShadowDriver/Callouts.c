#include "driver.h"
#include "device.tmh"
#include "Callouts.h"

UINT32 WpsCalloutId;
UINT32 WpmCalloutId;

HANDLE InjectHandle = NULL;
void InjectPacket(PNET_BUFFER_LIST clonedPacket)
{
    FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_NETWORK, &InjectHandle);
}

void InjectCompleted(
    void* context,
    NET_BUFFER_LIST* netBufferList,
    BOOLEAN dispatchLevel
)
{
    FwpsFreeCloneNetBufferList0(netBufferList, 0);
}

VOID NTAPI ClassifyFn(
    _In_ const FWPS_INCOMING_VALUES0* inFixedValues,
    _In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    _Inout_opt_ void* layerData,
    _In_ const FWPS_FILTER0* filter,
    _In_ UINT64 flowContext,
    _Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
    NTSTATUS status;
    NET_BUFFER_LIST* packet;
    FWPS_STREAM_DATA* streamData;
    SIZE_T dataLength = 0;
    SIZE_T bytes = 0;
    PVOID dataBuffer = NULL;
    PCHAR outputs = NULL;
    PNET_BUFFER_LIST clonedPacket;
    FWPS_PACKET_INJECTION_STATE injectionState = FWPS_PACKET_NOT_INJECTED;

    packet = (NET_BUFFER_LIST*)layerData;

    if (InjectHandle != NULL)
    {
        injectionState = FwpsQueryPacketInjectionState0(InjectHandle, packet, NULL);

        if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
            injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
        {
            classifyOut->actionType = FWP_ACTION_PERMIT;
        }
    }
    

    if (classifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
    {
        ULONG localIp, remoteIp;

        localIp = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS].value.uint32;
        remoteIp = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS].value.uint32;

        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Ready to block\n");

        //´òÓ¡IP±¨
        if (packet->FirstNetBuffer != NULL)
        {
            PNET_BUFFER_LIST clonedBuffer;

            PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet);

            dataBuffer = NdisGetDataBuffer(netBuffer, netBuffer->DataLength, NULL, 1, 0);


            size_t outputLength = CaculateHexStringLength(netBuffer->DataLength);
            outputs = ExAllocatePoolWithTag(NonPagedPool, outputLength, 'op');
            ConvertBytesArrayToHexString(dataBuffer, netBuffer->DataLength, outputs, 400);

            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "%s\t\n", outputs);
            ExFreePoolWithTag(outputs, 'op');
        }
        
        status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL,0, &clonedPacket);

        classifyOut->actionType = FWP_ACTION_BLOCK;
        classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

        if (NT_SUCCESS(status))
        {
            status = FwpsInjectNetworkSendAsync0(InjectHandle, NULL, 0, inMetaValues->compartmentId, clonedPacket, InjectCompleted, NULL);
        }
    }
    else
    {
        classifyOut->actionType = FWP_ACTION_PERMIT;

        if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
        {
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
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

NTSTATUS CreateInjector()
{
    NTSTATUS status;

    status = FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_NETWORK | FWPS_INJECTION_TYPE_FORWARD, &InjectHandle);

    return status;
}

NTSTATUS AddCalloutToWfp(IN HANDLE engineHandle)
{
    FWPM_CALLOUT0 callout = { 0 };
    callout.flags = 0;
    callout.displayData.description = L"I think you know what it is.";
    callout.displayData.name = L"ShadowCallout";
    callout.calloutKey = WFP_ESTABLISHED_CALLOUT_GUID;
    callout.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V4;



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