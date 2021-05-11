#include "ShadowCallouts.h"
#include "ShadowFilterContext.h"
//#include "NetFilteringCondition.h"
#include "IOCTLHelper.h"
#include "InjectionHelper.h"

PacketModificationContext ShadowCallout::PendingNetBufferListHeader;
KSPIN_LOCK ShadowCallout::SpinLock;

#ifdef DBG
/// <summary>
/// Print fragment information of a NET_BUFFER_LIST.
/// This function can be called only when DBG is predefinded.
/// </summary>
/// <param name="netBufferList"></param>
void PrintFragmentInfo(PNET_BUFFER_LIST netBufferList)
{
	if (netBufferList)
	{
		auto firstNetBuffer = NET_BUFFER_LIST_FIRST_NB(netBufferList);
		auto currentNbl = netBufferList;
		while (currentNbl)
		{
			if (firstNetBuffer && NET_BUFFER_NEXT_NB(firstNetBuffer))
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

		if (firstNetBuffer && NET_BUFFER_NEXT_NB(firstNetBuffer))
		{
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "\t\n");
		}
	}
}
#endif

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
			PrintFragmentInfo(packet);
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
			for (PNET_BUFFER currentBuffer = firstNetBuffer; currentBuffer != nullptr; currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer))
			{
				++netBufferCount;
			}

			int fragIndex = 0;

			for (PNET_BUFFER currentBuffer = firstNetBuffer; currentBuffer != nullptr; currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer))
			{
				ULONG dataLength = NET_BUFFER_DATA_LENGTH(currentBuffer);
				auto offsetLength = NET_BUFFER_DATA_OFFSET(currentBuffer);

				BYTE* packetBuffer = new BYTE[dataLength];
				PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(currentBuffer, dataLength, packetBuffer, 1, 0);

				auto netBufferListPointerSize = sizeof(PNET_BUFFER_LIST);
				size_t totolBufferLength = dataLength + netBufferListPointerSize + sizeof(netBufferCount) + sizeof(fragIndex) + sizeof(offsetLength);
				BYTE* packetBufferWithMetaInfo = new BYTE[totolBufferLength];

				if (packetBufferWithMetaInfo != nullptr)
				{
					BYTE* currentWrittenPos = packetBufferWithMetaInfo;
					RtlCopyMemory(currentWrittenPos, &packet, netBufferListPointerSize);
					currentWrittenPos += netBufferListPointerSize;
					RtlCopyMemory(currentWrittenPos, &netBufferCount, sizeof(netBufferCount));
					currentWrittenPos += sizeof(netBufferCount);
					RtlCopyMemory(currentWrittenPos, &fragIndex, sizeof(fragIndex));
					currentWrittenPos += sizeof(fragIndex);
					RtlCopyMemory(currentWrittenPos, &offsetLength, sizeof(offsetLength));
					currentWrittenPos += sizeof(offsetLength);
					RtlCopyMemory(currentWrittenPos, dataBuffer, dataLength);
					(context->NetPacketFilteringCallout)(layer, direction, packetBufferWithMetaInfo, totolBufferLength, context->CustomContext);

					delete packetBufferWithMetaInfo;
				}

				delete packetBuffer;

				++fragIndex;
			}
		}
	}
}


NTSTATUS ShadowCallout::InitializeNBLListHeader()
{
	NTSTATUS status = STATUS_SUCCESS;
	KeInitializeSpinLock(&SpinLock);
	InitializeListHead(&(PendingNetBufferListHeader.ListEntry));

	return status;
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

	ShadowFilterContext* context = (ShadowFilterContext*)(filter->context);
	HANDLE injectionHandle = InjectionHelper::InjectionHandles[2];
	NET_BUFFER_LIST* packet = (NET_BUFFER_LIST*)layerData;

	if (packet != nullptr)
	{
		auto injectionState = FwpsQueryPacketInjectionState(injectionHandle, packet, NULL);

#ifdef DBG
		PrintFragmentInfo((PNET_BUFFER_LIST)layerData);
#endif

		classifyOut->actionType = FWP_ACTION_PERMIT;

		// If the packet is a injected one.		
		if (context->IsModificationEnable)
		{
			if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
				injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
			{
				SendPacketToUserMode(NetLayer::NetworkLayer, NetPacketDirection::Out, packet, context);
			}
			else
			{
				PNET_BUFFER_LIST clonedPacket = NULL;
				status = FwpsAllocateCloneNetBufferList(packet, NULL, NULL, 0, &clonedPacket);

				if (NT_SUCCESS(status))
				{
					classifyOut->actionType = FWP_ACTION_BLOCK;
					classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

					// Inqueue cloned net buffer list.
					PacketModificationContext* newBufferListEntry = new PacketModificationContext;
					newBufferListEntry->ReceviedFragmentCounts = 0;
					newBufferListEntry->FragmentCounts = 0;
					newBufferListEntry->NetBufferList = clonedPacket;
					newBufferListEntry->CompartmentId = (COMPARTMENT_ID)inMetaValues->compartmentId;
					InsertTailList(&PendingNetBufferListHeader.ListEntry, &(newBufferListEntry->ListEntry));

					// Get net buffer counts.
					PNET_BUFFER firstNetBuffer = NET_BUFFER_LIST_FIRST_NB(clonedPacket);
					for (PNET_BUFFER currentNetBuffer = firstNetBuffer; currentNetBuffer != nullptr; currentNetBuffer = NET_BUFFER_NEXT_NB(currentNetBuffer))
					{
						++(newBufferListEntry->FragmentCounts);
					}

					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "Queued a net buffer list\t\n");
				}
			}
		}
		else
		{
			SendPacketToUserMode(NetLayer::NetworkLayer, NetPacketDirection::Out, packet, context);
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
	NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(inFixedValues);

	classifyOut->actionType = FWP_ACTION_PERMIT;
#ifdef DBG
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "NetworkInV4ClassifyFn\t\n");
#endif

#ifdef DBG
	PrintFragmentInfo((PNET_BUFFER_LIST)layerData);
#endif
	ShadowFilterContext* context = (ShadowFilterContext*)(filter->context);
	PNET_BUFFER_LIST packet = (PNET_BUFFER_LIST)layerData;

	if (packet && context)
	{
		auto ipHeaderSize = inMetaValues->ipHeaderSize;
		NdisRetreatNetBufferListDataStart(packet, ipHeaderSize, 0, NULL, NULL);

		if (context->IsModificationEnable)
		{
			auto injectionState = FwpsQueryPacketInjectionState(InjectionHelper::InjectionHandles[0], packet, NULL);

			if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
				injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
			{
				SendPacketToUserMode(NetLayer::NetworkLayer, NetPacketDirection::Out, packet, context);
			}
			else
			{
				PNET_BUFFER_LIST clonedPacket = NULL;
				status = FwpsAllocateCloneNetBufferList(packet, NULL, NULL, 0, &clonedPacket);

				if (NT_SUCCESS(status))
				{
					SendPacketToUserMode(NetLayer::NetworkLayer, NetPacketDirection::Out, clonedPacket, context);
					classifyOut->actionType = FWP_ACTION_BLOCK;
					classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

					// Inqueue cloned net buffer list.
					PacketModificationContext* newBufferListEntry = new PacketModificationContext;
					newBufferListEntry->ReceviedFragmentCounts = 0;
					newBufferListEntry->FragmentCounts = 0;
					newBufferListEntry->NetBufferList = clonedPacket;
					newBufferListEntry->CompartmentId = (COMPARTMENT_ID)inMetaValues->compartmentId;
					InsertTailList(&PendingNetBufferListHeader.ListEntry, &(newBufferListEntry->ListEntry));

					// Get net buffer counts.
					PNET_BUFFER firstNetBuffer = NET_BUFFER_LIST_FIRST_NB(clonedPacket);
					for (PNET_BUFFER currentNetBuffer = firstNetBuffer; currentNetBuffer != nullptr; currentNetBuffer = NET_BUFFER_NEXT_NB(currentNetBuffer))
					{
						++(newBufferListEntry->FragmentCounts);
					}

					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "Queued a net buffer list\t\n");
				}
			}
		}
		else
		{
			SendPacketToUserMode(NetLayer::NetworkLayer, NetPacketDirection::In, packet, context);
		}

		NdisAdvanceNetBufferListDataStart(packet, ipHeaderSize, FALSE, 0);
	}
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
#ifdef DBG
	PrintFragmentInfo((PNET_BUFFER_LIST)layerData);
#endif
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
#ifdef DBG
	PrintFragmentInfo((PNET_BUFFER_LIST)layerData);
#endif
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

#ifdef DBG
	PrintFragmentInfo((PNET_BUFFER_LIST)layerData);
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

#ifdef DBG
	PrintFragmentInfo((PNET_BUFFER_LIST)layerData);
#endif

	CalloutPreproecess(layerData, filter, classifyOut, NetLayer::LinkLayer, NetPacketDirection::In);
}

NTSTATUS ShadowCallout::ModifyPacket(void* context, NetPacketDirection direction, NetLayer layer, void* buffer, unsigned long long size, unsigned long long identifier, int fragmentIndex)
{
	UNREFERENCED_PARAMETER(context);


	NTSTATUS status = STATUS_SUCCESS;
	PNET_BUFFER_LIST netBufferList = (PNET_BUFFER_LIST)identifier;
	if (netBufferList)
	{
		PacketModificationContext* netBufferListHeader = &(ShadowCallout::PendingNetBufferListHeader);
		auto currentNetBufferListEntry = CONTAINING_RECORD(netBufferListHeader->ListEntry.Flink, PacketModificationContext, ListEntry);
		PacketModificationContext* pmContext = nullptr;
		PNET_BUFFER_LIST nblToSend = nullptr;

		while (currentNetBufferListEntry != netBufferListHeader)
		{
			if (currentNetBufferListEntry->NetBufferList == netBufferList)
			{
				nblToSend = netBufferList;
				pmContext = currentNetBufferListEntry;
				(currentNetBufferListEntry->ReceviedFragmentCounts)++;
				break;
			}
			currentNetBufferListEntry = CONTAINING_RECORD(currentNetBufferListEntry->ListEntry.Flink, PacketModificationContext, ListEntry);
		}

		if (nblToSend)
		{
			PNET_BUFFER firstNetBuffer = NET_BUFFER_LIST_FIRST_NB(nblToSend);

			if (firstNetBuffer)
			{
				// Get selected net buffer to modification.
				PNET_BUFFER currentBuffer = firstNetBuffer;
				for (int currentFragmentIndex = 0; currentBuffer != nullptr && currentFragmentIndex < fragmentIndex; ++currentFragmentIndex)
				{
					currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
				}

				// Start modification.
				if (currentBuffer)
				{

					SIZE_T dataWritten = 0;
					for (PMDL currentMdl = NET_BUFFER_CURRENT_MDL(currentBuffer); currentMdl != nullptr && dataWritten < size; currentMdl = currentMdl->Next)
					{
#ifdef DBG
						DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "MDLByteCount: %d, MDLByteOffset: %d, DataToWrite: %d", currentMdl->ByteCount, currentMdl->ByteOffset, (int)size);
#endif
						auto currentMdlLength = MmGetMdlByteCount(currentMdl);
						PBYTE currentMdlBuffer = (PBYTE)MmGetMdlVirtualAddress(currentMdl);
						auto writtenSize = size < currentMdlLength ? size : currentMdlLength;
						RtlCopyMemory(currentMdlBuffer, buffer, writtenSize);
					}
				}
				// Indicate that fragment index is out of range.
				else
				{
					//status = 
				}
			}

		}

		// If all fragments are received, the driver injectst the NET_BUFFER_LIST to network stack and deque this NET_BUFFER_LIST from callout queue.
		if (nblToSend && pmContext->ReceviedFragmentCounts == pmContext->FragmentCounts)
		{
			RemoveEntryList(&currentNetBufferListEntry->ListEntry);

			InjectionHelper::Inject(pmContext, direction, layer, nblToSend);
			//status = FwpsInjectNetworkSendAsync(inejctionHandle, NULL, 0, currentNetBufferListEntry->CompartmentId, nblToSend, InjectionHelper::ModificationCompleted, context);
			//delete pmContext;

			if (!NT_SUCCESS(status))
			{
				NdisFreeCloneNetBufferList(nblToSend, 0);
			}
		}

	}
	return status;
}

void ShadowCallout::ModificationComplete(PNET_BUFFER_LIST netBufferList, void* packetContext)
{
	PacketModificationContext* pmContext = (PacketModificationContext*)packetContext;

	UNREFERENCED_PARAMETER(pmContext);

	// Free cloned NET_BUFFER_LIST.
	FwpsFreeCloneNetBufferList(netBufferList, 0);

	// Delete context.
	delete packetContext;
}
