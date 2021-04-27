#include "ShadowCallouts.h"
#include "ShadowFilterContext.h"
//#include "NetFilteringCondition.h"
#include "IOCTLHelper.h"

NTSTATUS PacketNotify(_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType, _In_ const GUID* filterKey, _Inout_ FWPS_FILTER0* filter)
{
	return STATUS_SUCCESS;
}

void PacketFlowDeleteNotfy(_In_ UINT16 layerId, _In_ UINT32 calloutId, _In_ UINT64 flowContext)
{
	return;
}

VOID NTAPI NetworkOutV4ClassifyFn(
	_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_ const FWPS_FILTER0* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
	NTSTATUS status = STATUS_SUCCESS;
	NET_BUFFER_LIST* packet;
	FWPS_STREAM_DATA* streamData;
	SIZE_T dataLength = 0;
	SIZE_T bytes = 0;
	NDIS_STATUS ndisStatus;

	PNET_BUFFER_LIST clonedPacket = NULL;
	FWPS_PACKET_INJECTION_STATE injectionState = FWPS_PACKET_NOT_INJECTED;

	packet = (NET_BUFFER_LIST*)layerData;
#ifdef DBG
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "Original Net Buffer List:\t\n");
	//PrintNetBufferList(packet, DPFLTR_TRACE_LEVEL);
#endif
	classifyOut->actionType = FWP_ACTION_PERMIT;

	if (packet)
	{
		ShadowFilterContext* context = (ShadowFilterContext*)(filter->context);
		
		if (context->NetPacketFilteringCallout != NULL)
		{
			//+++++++++++将数据包包装成自己的形式以便进行处理+++++++++++
			status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL, 0, &clonedPacket);
			PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(clonedPacket);
			PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), NULL, 1, 0);
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Packet received!\t\n");
#endif
			(context->NetPacketFilteringCallout)(NetLayer::NetworkLayer, NetPacketDirection::Out, dataBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), context->CustomContext);
		}
		if (context->IsModificationEnable)
		{
			//还未实现
		}

		//删除分配的缓冲区

	}
}

VOID NTAPI NetworkInV4ClassifyFn(
	_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_ const FWPS_FILTER0* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
	classifyOut->actionType = FWP_ACTION_PERMIT;
}


VOID NTAPI NetworkInV6ClassifyFn(
	_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_ const FWPS_FILTER0* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
	classifyOut->actionType = FWP_ACTION_PERMIT;
}

VOID NTAPI NetworkOutV6ClassifyFn(
	_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_ const FWPS_FILTER0* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
	classifyOut->actionType = FWP_ACTION_PERMIT;
}

VOID NTAPI LinkOutClassifyFn(
	_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_ const FWPS_FILTER0* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
	NTSTATUS status = STATUS_SUCCESS;
	NET_BUFFER_LIST* packet;
	SIZE_T dataLength = 0;
	SIZE_T bytes = 0;
	NDIS_STATUS ndisStatus;

	PNET_BUFFER_LIST clonedPacket = NULL;
	FWPS_PACKET_INJECTION_STATE injectionState = FWPS_PACKET_NOT_INJECTED;

	packet = (NET_BUFFER_LIST*)layerData;
	//PrintNetBufferList(packet, DPFLTR_TRACE_LEVEL);
	classifyOut->actionType = FWP_ACTION_PERMIT;
	if (packet)
	{
		ShadowFilterContext* context = (ShadowFilterContext*)(filter->context);

		if (context->NetPacketFilteringCallout != NULL)
		{
			//+++++++++++将数据包包装成自己的形式以便进行处理+++++++++++
			status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL, 0, &clonedPacket);
			PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(clonedPacket);
			auto dataLength = NET_BUFFER_DATA_LENGTH(netBuffer);
			BYTE * packetBuffer = new BYTE[dataLength];
			PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), packetBuffer, 1, 0);

#ifdef DBG
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Packet received!\t\n");
#endif

			(context->NetPacketFilteringCallout)(NetLayer::NetworkLayer, NetPacketDirection::Out, dataBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), context->CustomContext);
			delete packetBuffer;
		}
		if (context->IsModificationEnable)
		{
			//还未实现
		}

		//删除分配的缓冲区

	}
}

VOID NTAPI LinkInClassifyFn(
	_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_ const FWPS_FILTER0* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
	NTSTATUS status = STATUS_SUCCESS;
	NET_BUFFER_LIST* packet;
	SIZE_T dataLength = 0;
	SIZE_T bytes = 0;
	NDIS_STATUS ndisStatus;

	PNET_BUFFER_LIST clonedPacket = NULL;
	FWPS_PACKET_INJECTION_STATE injectionState = FWPS_PACKET_NOT_INJECTED;

	packet = (NET_BUFFER_LIST*)layerData;
	//PrintNetBufferList(packet, DPFLTR_TRACE_LEVEL);
	classifyOut->actionType = FWP_ACTION_PERMIT;
	if (packet)
	{
		ShadowFilterContext* context = (ShadowFilterContext*)(filter->context);

		if (context->NetPacketFilteringCallout != NULL)
		{
			//+++++++++++将数据包包装成自己的形式以便进行处理+++++++++++
			status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL, 0, &clonedPacket);
			PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(clonedPacket);
			auto dataLength = NET_BUFFER_DATA_LENGTH(netBuffer);
			BYTE* packetBuffer = new BYTE[dataLength];
			PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), packetBuffer, 1, 0);

#ifdef DBG
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Packet received!\t\n");
#endif

			(context->NetPacketFilteringCallout)(NetLayer::NetworkLayer, NetPacketDirection::Out, dataBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), context->CustomContext);
			delete packetBuffer;
		}
		if (context->IsModificationEnable)
		{
			//还未实现
		}

		//删除分配的缓冲区
	}
}