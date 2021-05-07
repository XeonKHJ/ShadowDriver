#include "ShadowCallouts.h"
#include "ShadowFilterContext.h"
//#include "NetFilteringCondition.h"
#include "IOCTLHelper.h"
#include "InjectionHelper.h"

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

#ifdef DBG
			if (NET_BUFFER_NEXT_NB(netBuffer))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "This net buffer list has more than one net buffer.\t\n");

				PNET_BUFFER currentBuffer = netBuffer;

				while (currentBuffer)
				{
					auto netBufferOffset = NET_BUFFER_DATA_OFFSET(currentBuffer);
					auto netBufferLength = NET_BUFFER_DATA_LENGTH(currentBuffer);
					currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "%d, %d", netBufferOffset, netBufferLength);
					if (currentBuffer)
					{
						DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "->");
					}
				}
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "\t\n");
			}
#endif
			if (netBuffer)
			{
				(context->NetPacketFilteringCallout)(layer, direction, dataBuffer, NET_BUFFER_DATA_LENGTH(netBuffer), context->CustomContext);
			}
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

void ShadowCallout::SendPacketToUserMode(NetLayer layer, NetPacketDirection direction, PNET_BUFFER_LIST packet, ShadowFilterContext* context)
{
	if (packet)
	{
		if (context->NetPacketFilteringCallout != NULL)
		{
			// Cauculate the number of net buffers.
			PNET_BUFFER firstNetBuffer = NET_BUFFER_LIST_FIRST_NB(packet);
			int netBufferCount = 0;
			for (PNET_BUFFER currentBuffer = firstNetBuffer; currentBuffer != nullptr; NET_BUFFER_NEXT_NB(currentBuffer))
			{
				++netBufferCount;
			}

			int fragIndex = 0;
			for (PNET_BUFFER currentBuffer = firstNetBuffer; currentBuffer != nullptr; NET_BUFFER_NEXT_NB(currentBuffer))
			{
				ULONG dataLength = NET_BUFFER_DATA_LENGTH(currentBuffer);
				ULONG offsetLength = NET_BUFFER_DATA_OFFSET(currentBuffer);
				BYTE* packetBuffer = new BYTE[dataLength];
				PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(firstNetBuffer, NET_BUFFER_DATA_LENGTH(firstNetBuffer), packetBuffer, 1, 0);

				auto netBufferListPointerSize = sizeof(PNET_BUFFER_LIST);
				BYTE* packetBufferWithMetaInfo = new BYTE[dataLength + netBufferListPointerSize + sizeof(netBufferCount) + sizeof(fragIndex) + sizeof(offsetLength)];
				BYTE* currentWrittenPos = packetBufferWithMetaInfo;
				RtlCopyMemory(currentWrittenPos, &packet, netBufferListPointerSize);
				currentWrittenPos += netBufferListPointerSize;
				RtlCopyMemory(currentWrittenPos, &netBufferCount, sizeof(netBufferCount));
				currentWrittenPos += sizeof(netBufferCount);
				RtlCopyMemory(currentWrittenPos + sizeof(netBufferCount), &fragIndex, sizeof(fragIndex));
				currentWrittenPos += sizeof(fragIndex);
				RtlCopyMemory(currentWrittenPos + sizeof(offsetLength), &offsetLength, sizeof(offsetLength));
				currentWrittenPos += sizeof(offsetLength);
				RtlCopyMemory(currentWrittenPos, dataBuffer, dataLength);
				

				(context->NetPacketFilteringCallout)(layer, direction, packetBufferWithMetaInfo, dataLength + netBufferListPointerSize + sizeof(netBufferCount) + sizeof(fragIndex), context->CustomContext);
				delete packetBuffer;
				delete packetBufferWithMetaInfo;
				++fragIndex;
			}
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
	HANDLE injectionHandle = InjectionHelper::InjectionHandles[2];
	NET_BUFFER_LIST* packet = (NET_BUFFER_LIST*)layerData;

	if (packet != nullptr)
	{
		PNET_BUFFER_LIST clonedPacket = NULL;

		// Send packet to user-mode application.
		status = FwpsAllocateCloneNetBufferList(packet, NULL, NULL, 0, &clonedPacket);

		auto injectionState = FwpsQueryPacketInjectionState(injectionHandle, packet, NULL);

#ifdef DBG

		auto currentNbl = packet;
		while (currentNbl)
		{
			if (NET_BUFFER_NEXT_NB(NET_BUFFER_LIST_FIRST_NB(currentNbl)))
			{
				PNET_BUFFER currentBuffer = NET_BUFFER_LIST_FIRST_NB(currentNbl);

				while (currentBuffer)
				{
					auto netBufferOffset = NET_BUFFER_DATA_OFFSET(currentBuffer);
					auto netBufferLength = NET_BUFFER_DATA_LENGTH(currentBuffer);

					BYTE* testBuffer = new BYTE[netBufferLength];

					BYTE* testBufferPointer = (PBYTE)NdisGetDataBuffer(currentBuffer, netBufferLength, testBuffer, 1, 0);

					//UNREFERENCED_PARAMETER(testBufferPointer);

					delete testBuffer;
					UNREFERENCED_PARAMETER(testBufferPointer);

					currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "(%d, %d)", netBufferOffset, netBufferLength);
					if (currentBuffer)
					{
						DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "->");
					}
				}
			}
			currentNbl = NET_BUFFER_LIST_NEXT_NBL(currentNbl);
			if (currentNbl)
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "|");
			}
		}

		if (NET_BUFFER_LIST_NEXT_NBL(packet))
		{
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "\t\n");
		}

#endif


		classifyOut->actionType = FWP_ACTION_PERMIT;
		// If the packet is a injected one.		
		if (context->IsModificationEnable)
		{
			if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
				injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
			{
				FwpsFreeCloneNetBufferList(clonedPacket, NULL);
			}
			else
			{
				classifyOut->actionType = FWP_ACTION_BLOCK;
				classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

				if (NT_SUCCESS(status))
				{
					SendPacketToUserMode(NetLayer::NetworkLayer, NetPacketDirection::Out, clonedPacket, context);
				}
				else
				{
					FwpsFreeCloneNetBufferList(clonedPacket, NULL);
				}
			}
		}
		else
		{
			if (NT_SUCCESS(status))
			{
				SendPacketToUserMode(NetLayer::NetworkLayer, NetPacketDirection::Out, clonedPacket, context);
				FwpsFreeCloneNetBufferList(clonedPacket, NULL);
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