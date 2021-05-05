#include "InjectionHelper.h"

HANDLE InjectionHelper::NDISPoolHandle = NULL;
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

	if (!NT_SUCCESS(status))
	{
		DeleteInjectors(context);
	}

	return status;
}

UINT32 InjectionHelper::Inject(ShadowFilterContext* context, NetPacketDirection direction, NetLayer layer, void* buffer, SIZE_T size)
{
	//UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(direction);
	UNREFERENCED_PARAMETER(layer);
	UNREFERENCED_PARAMETER(buffer);
	UNREFERENCED_PARAMETER(size);

	NTSTATUS status = STATUS_SUCCESS;

	BYTE* newBuffer = new BYTE[size];
	RtlCopyMemory(newBuffer, buffer, size);
	PMDL mdl = NdisAllocateMdl(NDISPoolHandle, buffer, (UINT)size);
	PNET_BUFFER_LIST netBufferList = nullptr;
	status = FwpsAllocateNetBufferAndNetBufferList(NDISPoolHandle, 0, 0, mdl, 0, size, &netBufferList);

	if (NT_SUCCESS(status))
	{
		switch (layer)
		{
		case NetworkLayer:
			switch (direction)
			{
			case Out:
				status = FwpsInjectNetworkSendAsync(context->InjectionHandles[2], NULL, 0, UNSPECIFIED_COMPARTMENT_ID, netBufferList, InjectionHelper::SendInjectCompleted, context);
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

	return status;
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
	//auto mdlAddress = MmGetMdlVirtualAddress(mdl);

	// Delete net buffer list.
	NdisFreeNetBufferList(netBufferList);

	// Delete mdl
	NdisFreeMdl(mdl);

	// Delete 
	//delete mdlAddress;
}
