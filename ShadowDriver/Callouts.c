/*++

Module Name:

	Callout.c

Abstract:

	�����˹���WFP�а��Ĳ����ת������Ҫ�߼�������

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"
#include "Callouts.h"

//���ص��������豸ע��ʱ��á�
UINT32 WpsSendCalloutId;
UINT32 WpsReceiveCalloutId;

//���ص�������ӽ���������ʱ��á�
UINT32 WpmSendCalloutId;
UINT32 WpmReceiveCalloutId;

HANDLE SendInjectHandle = NULL;
HANDLE ReceiveInjectHandle = NULL;

extern UINT64 filterId;
extern UINT64 filterId2;

void PrintNetBufferList(PNET_BUFFER_LIST packet)
{
	PVOID dataBuffer = NULL;
	PCHAR outputs = NULL;
	PVOID copiedDataBuffer = NULL;
	//��ӡIP��
	if (packet->FirstNetBuffer != NULL)
	{
		PNET_BUFFER_LIST clonedBuffer;

		PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet);

		int dataLength = netBuffer->DataLength;

		//NdisRetreatNetBufferDataStart(netBuffer, 0, 0, NULL);
		int orignalDataLength = NET_BUFFER_DATA_LENGTH(netBuffer);
		
		copiedDataBuffer = ExAllocatePoolWithTag(NonPagedPool, orignalDataLength, 'cpk');

		dataLength = orignalDataLength;

		while (dataBuffer == NULL)
		{
			
			dataBuffer = NdisGetDataBuffer(netBuffer, dataLength, copiedDataBuffer, 1, 0);
			
			--dataLength;
		}
		++dataLength;

		if (dataBuffer != NULL)
		{
			size_t outputLength = CaculateHexStringLength(dataLength);
			outputs = ExAllocatePoolWithTag(NonPagedPool, outputLength, 'op');

			if (outputs != NULL)
			{
				ConvertBytesArrayToHexString(dataBuffer, dataLength, outputs, outputLength);

				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "%s\t\n", outputs);
				ExFreePoolWithTag(outputs, 'op');
			}
		}
		else
		{
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "DataBuffer Fetch Failed!\t\n");
		}
	}
}

void InjectPacket(PNET_BUFFER_LIST clonedPacket)
{
	FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_NETWORK, &SendInjectHandle);
}

void InjectCompleted(
	void* context,
	NET_BUFFER_LIST* netBufferList,
	BOOLEAN dispatchLevel
)
{
	NDIS_STATUS status = netBufferList->Status;

	FWPS_PACKET_INJECTION_STATE injectionState = FwpsQueryPacketInjectionState0(SendInjectHandle, netBufferList, NULL);

	if (status == NDIS_STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Completed\n");
	}

	//PrintNetBufferList(netBufferList);

	FwpsFreeCloneNetBufferList0(netBufferList, 0);

}

VOID ModifyPacket(PNET_BUFFER_LIST packet)
{
	if (packet != NULL)
	{
		PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet);

		//��ȡ���ݻ�����
		PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, netBuffer->DataLength, NULL, 1, 0);

		//��MDL����ȡ��Ϣ
		PVOID mdlBuffer = MmGetMdlVirtualAddress(netBuffer->CurrentMdl);

		//dataBuffer = (PBYTE)mdlBuffer;

		int mdlStringLength = CaculateHexStringLength(netBuffer->CurrentMdl->ByteCount);
		PVOID outputs = ExAllocatePoolWithTag(NonPagedPool, mdlStringLength, 'op');
		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "CurrentMDL: %s\t\n", outputs);

		char toModifyChar = ((PCHAR)mdlBuffer)[16];

		((PCHAR)mdlBuffer)[16] = (CHAR)192;
		((PCHAR)mdlBuffer)[17] = (CHAR)168;
		((PCHAR)mdlBuffer)[18] = (CHAR)1;
		((PCHAR)mdlBuffer)[19] = (CHAR)101;

		ULONG dataLength = netBuffer->DataLength;
		ULONG ipPacketLength = 20;
		ULONG ipaddrOffset = 12;
		PBYTE ipAddrPos = (dataBuffer += dataLength);

		BYTE modifiedAddress[] = { 192, 168, 10, 1 };

		//�޸�IP��ַ
		for (int i = 0; i < 4; ++i, ++ipAddrPos)
		{
			ipAddrPos[i] = modifiedAddress[i];
		}

		//INT32 sum = 0;
		////��У�������
		//dataBuffer[10] = dataBuffer[11] = 0;

		////����У���
		//for (int i = 0; i < 10; i += 2)
		//{
		//	WORD dBytes = dataBuffer[i];
		//	dBytes = (dBytes << 16) + dataBuffer[i+1];

		//	sum += dBytes;
		//}

		////�����λ����
		//while (sum > 0xFFFF)
		//{
		//	UINT32 carryPart = (sum & 0x00FF0000) >> 16;
		//	sum = sum & 0xFFFF;
		//	sum += carryPart;
		//}

		////��ȡ���4�ֽ�
		//DWORD dwordSum = sum & 0xFFFF;

		////ȡ����
		//dwordSum = ~dwordSum;

		////�������ݰ�
		//(*(PDWORD) & (dataBuffer[10])) = dwordSum;

		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "ModifiedMDL: %s\t\n", outputs);
	}
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
	
	
	PNET_BUFFER_LIST clonedPacket = NULL;
	FWPS_PACKET_INJECTION_STATE injectionState = FWPS_PACKET_NOT_INJECTED;

	packet = (NET_BUFFER_LIST*)layerData;
	PrintNetBufferList(packet);
	
	//��ʱ������β����й۲��ȥ�İ�
	if (SendInjectHandle != NULL && filter->filterId == filterId)
	{
		injectionState = FwpsQueryPacketInjectionState0(SendInjectHandle, packet, NULL);

		//�����������ݰ�������
		if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
			injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			//PrintNetBufferList(packet);
		}
		//�����ݰ����Ǳ��ֶ�ע������ݰ�
		else if (injectionState == FWPS_PACKET_NOT_INJECTED)
		{
			
			status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL, 0, &clonedPacket);

			//���Ϊ��¡�����ݰ�����������ʧ�ܵĻ��������������ݰ���
			if (!NT_SUCCESS(status))
			{

			}
			//������ݰ������������ɹ�
			else
			{
				ModifyPacket(clonedPacket);
				PrintNetBufferList(clonedPacket);
				//status = FwpsInjectNetworkSendAsync0(InjectHandle, NULL, 0, UNSPECIFIED_COMPARTMENT_ID, clonedPacket, InjectCompleted, NULL);
				status = FwpsInjectNetworkSendAsync0(SendInjectHandle, NULL, 0, inMetaValues->compartmentId, clonedPacket, InjectCompleted, NULL);

				if (!NT_SUCCESS(status))
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Failed\n");
				}
				else
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Success\n");
				}
			}

			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Ready to block\n");
			classifyOut->actionType = FWP_ACTION_BLOCK;
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

	FWPS_CALLOUT0 sendCallout = { 0 };

	sendCallout.calloutKey = WFP_SEND_ESTABLISHED_CALLOUT_GUID;
	sendCallout.flags = 0;
	sendCallout.classifyFn = ClassifyFn;
	sendCallout.notifyFn = NotifyFn;
	sendCallout.flowDeleteFn = FlowDeleteFn;
	status = FwpsCalloutRegister0(deviceObject, &sendCallout, &WpsSendCalloutId);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Registered Send Callout to Device Object.\n");

	FWPS_CALLOUT0 receiveCallout = { 0 };
	receiveCallout.calloutKey = WFP_RECEIVE_ESTABLISHED_CALLOUT_GUID;
	receiveCallout.flags = 0;
	receiveCallout.classifyFn = ClassifyFn;
	receiveCallout.notifyFn = NotifyFn;
	receiveCallout.flowDeleteFn = FlowDeleteFn;
	status = FwpsCalloutRegister0(deviceObject, &receiveCallout, &WpsReceiveCalloutId);

	if (NT_SUCCESS(status))
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Registered Receive Callout to Device Object.\n");
	}

	return status;
}

NTSTATUS CreateInjector()
{
	NTSTATUS status;

	status = FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_NETWORK | FWPS_INJECTION_TYPE_FORWARD, &SendInjectHandle);

	status = FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_NETWORK | FWPS_INJECTION_TYPE_FORWARD, &ReceiveInjectHandle);

	return status;
}

/// <summary>
/// ��ӻص�����������
/// </summary>
/// <param name="engineHandle">����������</param>
/// <returns>״̬</returns>
NTSTATUS AddCalloutToWfp(IN HANDLE engineHandle)
{
	NTSTATUS status;
	FWPM_CALLOUT0 sendCallout = { 0 };

	//���ڲ�׽�ٷ��͵�·�ϵ����ݰ�
	sendCallout.flags = 0;
	sendCallout.displayData.description = L"I think you know what it is.";
	sendCallout.displayData.name = L"ShadowSendCallouts";
	sendCallout.calloutKey = WFP_SEND_ESTABLISHED_CALLOUT_GUID;
	sendCallout.applicableLayer = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
	status = FwpmCalloutAdd0(engineHandle, &sendCallout, NULL, &WpmSendCalloutId);
	

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Added Send Callout to WFP.\n");

	//���ڲ�׽�ڽ��յ�·�ϵ����ݰ�
	FWPM_CALLOUT0 receiveCallout = { 0 };
	receiveCallout.flags = 0;
	receiveCallout.displayData.description = L"I think you know what it is.";
	receiveCallout.displayData.name = L"ShadowReceiveCallouts";
	receiveCallout.calloutKey = WFP_RECEIVE_ESTABLISHED_CALLOUT_GUID;
	receiveCallout.applicableLayer = FWPM_LAYER_INBOUND_IPPACKET_V4;
	status = FwpmCalloutAdd0(engineHandle, &receiveCallout, NULL, &WpmReceiveCalloutId);

	if (NT_SUCCESS(status))
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Added Receive Callout to WFP.\n");
	}

	return status;
}

/// <summary>
/// �����������豸ע���ص���
/// </summary>
/// <param name="engineHandle"></param>
VOID UnRegisterCallout(HANDLE engineHandle)
{
	if (WpmSendCalloutId != 0)
	{
		FwpmCalloutDeleteById(engineHandle, WpmSendCalloutId);
	}

	if (WpsSendCalloutId != 0)
	{
		FwpsCalloutUnregisterById0(WpsSendCalloutId);
	}
}