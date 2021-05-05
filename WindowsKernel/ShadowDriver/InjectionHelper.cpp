#include "InjectionHelper.h"

NTSTATUS InjectionHelper::CreateInjector(ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;
	auto filterIds = context->FilterIds;
	HANDLE injectionHandle = NULL;
	if (NT_SUCCESS(status))
	{
		if (filterIds[0] != NULL || filterIds[2] != NULL)
		{
			status = FwpsInjectionHandleCreate(AF_INET, FWPS_INJECTION_TYPE_NETWORK, &injectionHandle);

			if (NT_SUCCESS(status))
			{
				context->InjectionHandles[0] = injectionHandle;
				context->InjectionHandles[2] = injectionHandle;
			}
		}
	}

	if (NT_SUCCESS(status))
	{
		if (filterIds[1] != NULL || filterIds[3] != NULL)
		{
			status = FwpsInjectionHandleCreate(AF_INET6, FWPS_INJECTION_TYPE_NETWORK, &injectionHandle);

			if (NT_SUCCESS(status))
			{
				context->InjectionHandles[1] = injectionHandle;
				context->InjectionHandles[3] = injectionHandle;
			}
		}
	}

	if (NT_SUCCESS(status))
	{
		if (filterIds[4] != NULL || filterIds[6] != NULL)
		{
			status = FwpsInjectionHandleCreate(AF_UNSPEC, FWPS_INJECTION_TYPE_L2, &injectionHandle);

			if (NT_SUCCESS(status))
			{
				context->InjectionHandles[4] = injectionHandle;
				context->InjectionHandles[6] = injectionHandle;
			}
		}
	}

	// Allocate net buffer list pool.
	if (NT_SUCCESS(status))
	{
		NET_BUFFER_LIST_POOL_PARAMETERS parameters{};
		parameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
		parameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
		parameters.Header.Size = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
		parameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
		parameters.fAllocateNetBuffer = TRUE;
		parameters.ContextSize = 0;

		// Indicate Inject modified packet. 
		// This one needs to be changed later. Because every app id needs their own PoolTag.
		parameters.PoolTag = 'imp';

		parameters.DataSize = 0;
		context->NDISPoolHandle = NdisAllocateNetBufferListPool(NULL, &parameters);
	}

	if (!NT_SUCCESS(status))
	{
		DeleteInjectors(context);
	}

	return status;
}

UINT32 InjectionHelper::Inject(ShadowFilterContext* context, NetPacketDirection direction, NetLayer layer, void* buffer, SIZE_T size)
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(direction);
	UNREFERENCED_PARAMETER(layer);
	UNREFERENCED_PARAMETER(buffer);
	UNREFERENCED_PARAMETER(size);

	NTSTATUS status = STATUS_SUCCESS;

	BYTE* newBuffer = new BYTE[size];
	RtlCopyMemory(newBuffer, buffer, size);
	PMDL mdl = NdisAllocateMdl(context->NDISPoolHandle, buffer, (UINT)size);
	PNET_BUFFER_LIST netBufferList = nullptr;
	status = FwpsAllocateNetBufferAndNetBufferList(context->NDISPoolHandle, 0, 0, mdl, 0, size, &netBufferList);

	if (NT_SUCCESS(status))
	{
		switch (layer)
		{
		case NetworkLayer:
			switch (direction)
			{
			case Out:
				FwpsInjectNetworkSendAsync(context->InjectionHandles[2], NULL, 0, UNSPECIFIED_COMPARTMENT_ID, netBufferList, InjectionHelper::SendInjectCompleted, context);
				break;
			case In:
				break;
			default:
				break;
			}
			break;
		case LinkLayer:
			break;
		default:
			break;
		}
	}

	return UINT32();
}

void InjectionHelper::DeleteInjectors(ShadowFilterContext* context)
{
	NTSTATUS status = STATUS_SUCCESS;
	auto injectionIds = context->InjectionHandles;
	if (injectionIds[0] != NULL || injectionIds[2] != NULL)
	{
		HANDLE injectionHandle = injectionIds[0] == NULL ? injectionIds[2] : injectionIds[0];
		status = FwpsInjectionHandleCreate(AF_INET, FWPS_INJECTION_TYPE_NETWORK, &injectionHandle);

		context->InjectionHandles[0] = NULL;
		context->InjectionHandles[2] = NULL;
	}

	if (injectionIds[1] != NULL || injectionIds[3] != NULL)
	{
		HANDLE injectionHandle = injectionIds[1] == NULL ? injectionIds[3] : injectionIds[1];
		status = FwpsInjectionHandleCreate(AF_INET6, FWPS_INJECTION_TYPE_NETWORK, &injectionHandle);
		context->InjectionHandles[1] = NULL;
		context->InjectionHandles[3] = NULL;
	}

	if (injectionIds[4] != NULL || injectionIds[6] != NULL)
	{
		HANDLE injectionHandle = injectionIds[4] == NULL ? injectionIds[6] : injectionIds[4];
		status = FwpsInjectionHandleCreate(AF_UNSPEC, FWPS_INJECTION_TYPE_L2, &injectionHandle);
		context->InjectionHandles[4] = NULL;
		context->InjectionHandles[6] = NULL;
	}

	if (context->NDISPoolHandle != NULL)
	{
		NdisFreeNetBufferListPool(context->NDISPoolHandle);
		context->NDISPoolHandle = NULL;
	}
}

UINT32 InjectionHelper::GetInjectionFlagByCode(unsigned int code)
{
	UINT32 flag = NULL;
	switch (code)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		flag = FWPS_INJECTION_TYPE_NETWORK;
	case 4:
	case 6:
		flag = FWPS_INJECTION_TYPE_L2;
	}

	return flag;
}

UINT32 InjectionHelper::InjectByCode(unsigned int code)
{
	UNREFERENCED_PARAMETER(code);
	return UINT32();
}

void InjectionHelper::SendInjectCompleted(void* context, NET_BUFFER_LIST* netBufferList, BOOLEAN dispatchLevel)
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(netBufferList);
	UNREFERENCED_PARAMETER(dispatchLevel);
	NDIS_STATUS status = netBufferList->Status;
	ShadowFilterContext* sfContext = (ShadowFilterContext*)context;
	HANDLE sendInjectHandle = sfContext->InjectionHandles[2];
	FWPS_PACKET_INJECTION_STATE injectionState = FwpsQueryPacketInjectionState0(sendInjectHandle, netBufferList, NULL);

	UNREFERENCED_PARAMETER(injectionState);

	if (status == NDIS_STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Send Inject Completed\n");
	}
	
	auto nb = NET_BUFFER_LIST_FIRST_NB(netBufferList);
	auto mdl = NET_BUFFER_FIRST_MDL(nb);
	auto mdlAddress = MmGetMdlVirtualAddress(mdl);

	// Delete net buffer list.
	NdisFreeNetBufferList(netBufferList);

	// Delete mdl
	NdisFreeMdl(mdl);

	// Delete 
	delete mdlAddress;
}
