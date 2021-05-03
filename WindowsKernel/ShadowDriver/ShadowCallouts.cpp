#include "ShadowCallouts.h"
#include "ShadowFilterContext.h"
//#include "NetFilteringCondition.h"
#include "IOCTLHelper.h"

NTSTATUS ShadowCallout::CalloutPreproecess(
	_Inout_opt_ void* layerData,
	_In_ const FWPS_FILTER* filter,
	_Inout_ FWPS_CLASSIFY_OUT* classifyOut,
	NetLayer layer,
	NetPacketDirection direction
)
{
	NTSTATUS status = STATUS_SUCCESS;
	NET_BUFFER_LIST* packet;
	SIZE_T dataLength = 0;
	PNET_BUFFER_LIST clonedPacket = NULL;

	packet = (NET_BUFFER_LIST*)layerData;

#ifdef DBG
	//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "Original Net Buffer List:\t\n");
	//PrintNetBufferList(packet, DPFLTR_TRACE_LEVEL);
#endif

	classifyOut->actionType = FWP_ACTION_PERMIT;

	if (packet)
	{
		ShadowFilterContext* context = (ShadowFilterContext*)(filter->context);

		if (context->NetPacketFilteringCallout != NULL)
		{
			status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL, 0, &clonedPacket);
			
			PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(clonedPacket);
			dataLength = NET_BUFFER_DATA_LENGTH(netBuffer);
			BYTE* packetBuffer = new BYTE[dataLength];
			PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), packetBuffer, 1, 0);

			(context->NetPacketFilteringCallout)(layer, direction, dataBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), context->CustomContext);
			FwpsFreeCloneNetBufferList0(clonedPacket, 0);
			delete packetBuffer;
		}
		if (context->IsModificationEnable)
		{
			//还未实现
		}
		//删除分配的缓冲区
	}

	return status;
}

void ShadowCallout::SendPacketToUserMode(NetLayer layer, NetPacketDirection direction, PNET_BUFFER_LIST packet, ShadowFilterContext * context)
{
	if (packet)
	{
		if (context->NetPacketFilteringCallout != NULL)
		{
			PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet);
			ULONG dataLength = NET_BUFFER_DATA_LENGTH(netBuffer);
			BYTE* packetBuffer = new BYTE[dataLength];
			PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), packetBuffer, 1, 0);

			(context->NetPacketFilteringCallout)(layer, direction, dataBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), context->CustomContext);
			delete packetBuffer;
		}
	}
}


NTSTATUS ShadowCallout::PacketNotify(_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
	_In_ const GUID* filterKey,
	_Inout_ FWPS_FILTER* filter)
{
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(notifyType);

	return STATUS_SUCCESS;
}

void ShadowCallout::PacketFlowDeleteNotfy(_In_ UINT16 layerId, _In_ UINT32 calloutId, _In_ UINT64 flowContext)
{
	UNREFERENCED_PARAMETER(layerId);
	UNREFERENCED_PARAMETER(calloutId);
	UNREFERENCED_PARAMETER(flowContext);

	return;
}

VOID NTAPI ShadowCallout::NetworkOutV4ClassifyFn(
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_opt_ const void* classifyContext,
	_In_ const FWPS_FILTER* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT* classifyOut
)
{
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(inFixedValues);

	NTSTATUS status = STATUS_SUCCESS;

#ifdef DBG
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "NetworkOutV4ClassifyFn\t\n");
#endif
	classifyOut->actionType = FWP_ACTION_PERMIT;

	ShadowFilterContext* contextToVerify = (ShadowFilterContext*)classifyContext;

	UNREFERENCED_PARAMETER(contextToVerify);

	ShadowFilterContext* context = (ShadowFilterContext*)(filter->context);
	HANDLE injectionHandle = context->InjectionHandles[2];
	NET_BUFFER_LIST* packet = (NET_BUFFER_LIST*)layerData;

	if (packet != nullptr)
	{
		PNET_BUFFER_LIST clonedPacket = NULL;

		// Send packet to user-mode application.
		status = FwpsAllocateCloneNetBufferList(packet, NULL, NULL, 0, &clonedPacket);

		if (NT_SUCCESS(status))
		{
			SendPacketToUserMode(NetLayer::NetworkLayer, NetPacketDirection::Out, clonedPacket, context);
			FwpsFreeCloneNetBufferList(clonedPacket, NULL);
		}

		if (context->IsModificationEnable && injectionHandle != NULL)
		{
			classifyOut->actionType = FWP_ACTION_BLOCK;
			auto injectionState = FwpsQueryPacketInjectionState(injectionHandle, packet, NULL);

			// If the packet is a injected one.
			if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
				injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
			}
		}
	}
}

VOID NTAPI ShadowCallout::NetworkInV4ClassifyFn(
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_opt_ const void* classifyContext,
	_In_ const FWPS_FILTER* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT* classifyOut
)
{
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(inFixedValues);

	classifyOut->actionType = FWP_ACTION_PERMIT;
#ifdef DBG
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "NetworkInV4ClassifyFn\t\n");
#endif
	CalloutPreproecess(layerData, filter, classifyOut, NetLayer::NetworkLayer, NetPacketDirection::In);
}


VOID NTAPI ShadowCallout::NetworkInV6ClassifyFn(
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_opt_ const void* classifyContext,
	_In_ const FWPS_FILTER* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT* classifyOut
)
{
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(inFixedValues);

	classifyOut->actionType = FWP_ACTION_PERMIT;
	CalloutPreproecess(layerData, filter, classifyOut, NetLayer::NetworkLayer, NetPacketDirection::In);
}

VOID NTAPI ShadowCallout::NetworkOutV6ClassifyFn(
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_opt_ const void* classifyContext,
	_In_ const FWPS_FILTER* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT* classifyOut
)
{
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(inFixedValues);

	classifyOut->actionType = FWP_ACTION_PERMIT;
	CalloutPreproecess(layerData, filter, classifyOut, NetLayer::NetworkLayer, NetPacketDirection::Out);
}

VOID NTAPI ShadowCallout::LinkOutClassifyFn(
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_opt_ const void* classifyContext,
	_In_ const FWPS_FILTER* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT* classifyOut
)
{
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(inFixedValues);

	classifyOut->actionType = FWP_ACTION_PERMIT;
#ifdef DBG
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "LinkOutClassifyFn\t\n");
#endif
	CalloutPreproecess(layerData, filter, classifyOut, NetLayer::LinkLayer, NetPacketDirection::Out);
}

VOID NTAPI ShadowCallout::LinkInClassifyFn(
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_opt_ const void* classifyContext,
	_In_ const FWPS_FILTER* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT* classifyOut
)
{
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(inFixedValues);

	classifyOut->actionType = FWP_ACTION_PERMIT;
#ifdef DBG
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "LinkInClassifyFn\t\n");
#endif
	CalloutPreproecess(layerData, filter, classifyOut, NetLayer::LinkLayer, NetPacketDirection::In);
}