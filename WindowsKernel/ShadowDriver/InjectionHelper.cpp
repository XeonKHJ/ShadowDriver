#include "InjectionHelper.h"
#include "PacketModificationContext.h"

HANDLE InjectionHelper::NDISPoolHandle = NULL;
HANDLE InjectionHelper::InjectionHandles[8];
void (*InjectionHelper::InjectCompleted)(PNET_BUFFER_LIST injectedBuffer, void* context) = nullptr;

NTSTATUS InjectionHelper::CreateInjector()
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE injectionHandle = NULL;
	if (NT_SUCCESS(status))
	{

			status = FwpsInjectionHandleCreate(AF_INET, FWPS_INJECTION_TYPE_NETWORK, &injectionHandle);

			if (NT_SUCCESS(status))
			{
				InjectionHandles[0] = injectionHandle;
				InjectionHandles[2] = injectionHandle;
			}
		
	}

	if (NT_SUCCESS(status))
	{

			status = FwpsInjectionHandleCreate(AF_INET6, FWPS_INJECTION_TYPE_NETWORK, &injectionHandle);

			if (NT_SUCCESS(status))
			{
				InjectionHandles[1] = injectionHandle;
				InjectionHandles[3] = injectionHandle;
			}
		
	}

	if (NT_SUCCESS(status))
	{

			status = FwpsInjectionHandleCreate(AF_UNSPEC, FWPS_INJECTION_TYPE_L2, &injectionHandle);

			if (NT_SUCCESS(status))
			{
				InjectionHandles[4] = injectionHandle;
				InjectionHandles[6] = injectionHandle;
			}
		
	}

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

		//This needs to be verified.
		NDISPoolHandle = NdisAllocateNetBufferListPool(NULL, &parameters);
	}

	if (!NT_SUCCESS(status))
	{
		DeleteInjectors();
	}
	
	return status;
}

void InjectionHelper::DeleteInjectors()
{
	NTSTATUS status = STATUS_SUCCESS;
	auto injectionIds = InjectionHandles;
	if (InjectionHandles[0] != NULL || InjectionHandles[2] != NULL)
	{
		HANDLE injectionHandle = injectionIds[0] == NULL ? injectionIds[2] : injectionIds[0];
		status = FwpsInjectionHandleCreate(AF_INET, FWPS_INJECTION_TYPE_NETWORK, &injectionHandle);

		InjectionHandles[0] = NULL;
		InjectionHandles[2] = NULL;
	}

	if (InjectionHandles[1] != NULL || InjectionHandles[3] != NULL)
	{
		HANDLE injectionHandle = injectionIds[1] == NULL ? injectionIds[3] : injectionIds[1];
		status = FwpsInjectionHandleCreate(AF_INET6, FWPS_INJECTION_TYPE_NETWORK, &injectionHandle);
		InjectionHandles[1] = NULL;
		InjectionHandles[3] = NULL;
	}

	if (InjectionHandles[4] != NULL || InjectionHandles[6] != NULL)
	{
		HANDLE injectionHandle = injectionIds[4] == NULL ? injectionIds[6] : injectionIds[4];
		status = FwpsInjectionHandleCreate(AF_UNSPEC, FWPS_INJECTION_TYPE_L2, &injectionHandle);
		InjectionHandles[4] = NULL;
		InjectionHandles[6] = NULL;
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
	HANDLE sendInjectHandle = InjectionHandles[2];
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

void InjectionHelper::ModificationCompleted(void* context, NET_BUFFER_LIST* netBufferList, BOOLEAN dispatchLevel)
{
	UNREFERENCED_PARAMETER(dispatchLevel);

#ifdef DBG
	NDIS_STATUS status = netBufferList->Status;
	UNREFERENCED_PARAMETER(status);
#endif

	if (InjectCompleted != nullptr)
	{
		InjectCompleted(netBufferList, context);
	}
}
