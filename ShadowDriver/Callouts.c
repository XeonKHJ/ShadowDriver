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
#include "ShaDriHelper.h"

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

void SendInjectCompleted(
	void* context,
	NET_BUFFER_LIST* netBufferList,
	BOOLEAN dispatchLevel
)
{
	NDIS_STATUS status = netBufferList->Status;

	FWPS_PACKET_INJECTION_STATE injectionState = FwpsQueryPacketInjectionState0(SendInjectHandle, netBufferList, NULL);

	if (status == NDIS_STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Send Inject Completed\n");
	}
	FwpsFreeCloneNetBufferList0(netBufferList, 0);
}

void ReceiveInjectCompleted(
	void* context,
	NET_BUFFER_LIST* netBufferList,
	BOOLEAN dispatchLevel
)
{
	NDIS_STATUS status = netBufferList->Status;

	FWPS_PACKET_INJECTION_STATE injectionState = FwpsQueryPacketInjectionState0(ReceiveInjectHandle, netBufferList, NULL);

	if (status == NDIS_STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Receive Inject Completed\n");
	}
	else
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Receive injection failed. Error code: %02X\n", status);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error NBL: ");

		PrintNetBufferList(netBufferList, DPFLTR_ERROR_LEVEL);
	}

	FwpsFreeCloneNetBufferList0(netBufferList, 0);
}

VOID ModifySendIPPacket(PNET_BUFFER_LIST packet)
{
	if (packet != NULL)
	{
		PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet);

		//��ȡ���ݻ�����
		PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, netBuffer->DataLength, NULL, 1, 0);

		//��MDL����ȡ��Ϣ
		PVOID mdlBuffer = MmGetMdlVirtualAddress(netBuffer->CurrentMdl);
		PCHAR mdlCharBuffer = (PCHAR)mdlBuffer;
		//dataBuffer = (PBYTE)mdlBuffer;

		int mdlStringLength = (int)CaculateHexStringLength(netBuffer->CurrentMdl->ByteCount);
		PVOID outputs = ExAllocatePoolWithTag(NonPagedPool, mdlStringLength, 'op');
		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "CurrentMDL: %s\t\n", (char*)outputs);


		((PCHAR)mdlBuffer)[16] = (CHAR)192;
		((PCHAR)mdlBuffer)[17] = (CHAR)168;
		((PCHAR)mdlBuffer)[18] = (CHAR)1;
		((PCHAR)mdlBuffer)[19] = (CHAR)106;


		//��������Э��ʱTCP����UDP����Ҫ���¼���TCP����UDP��У��͡�
		//���Э��
		CHAR protocal = mdlCharBuffer[9];

		//��ȡ��������ϵ����ݳ��ȣ�����������㣩��
		short ipHeaderLength = (mdlCharBuffer[0] & 0xf) * 4;
		short totalLength = (mdlCharBuffer[2] << 8) + (mdlCharBuffer[3] & 0xFF);
		short transportDataLength = totalLength - ipHeaderLength;

		switch (protocal)
		{
		case 6: //TCPЭ��
		{
			CalculateTcpPacketCheckSum(transportDataLength, mdlCharBuffer, protocal, ipHeaderLength, packet);
		}
		break;
		case 17: //UDPЭ��
			break;
		}

		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "ModifiedMDL: %s\t\n", (char*)outputs);
	}
}

VOID ModifyReceiveIPPacket(PNET_BUFFER_LIST packet)
{
	if (packet != NULL)
	{
		PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet);

		////��ȡ���ݻ�����
		//PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, netBuffer->DataLength, NULL, 1, 0);

		//��MDL����ȡ��Ϣ
		PVOID mdlBuffer = MmGetMdlVirtualAddress(netBuffer->CurrentMdl);

		//dataBuffer = (PBYTE)mdlBuffer;

		/*++++++��ӡ+++++++*/
		int mdlStringLength = (int)CaculateHexStringLength(netBuffer->CurrentMdl->ByteCount);
		PVOID outputs = ExAllocatePoolWithTag(NonPagedPool, mdlStringLength, 'op');
		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "CurrentMDL: %s\t\n", (char*)outputs);
		/*------��ӡ-------*/

		PCHAR mdlCharBuffer = (PCHAR)mdlBuffer;

		//�޸�Ŀ��IP��ַ

		mdlCharBuffer[12] = (CHAR)192;
		mdlCharBuffer[13] = (CHAR)168;
		mdlCharBuffer[14] = (CHAR)0;
		mdlCharBuffer[15] = (CHAR)177;


		//��������Э��ʱTCP����UDP����Ҫ���¼���TCP����UDP��У��͡�
		//���Э��
		CHAR protocal = mdlCharBuffer[9];

		//��ȡ��������ϵ����ݳ��ȣ�����������㣩��
		short ipHeaderLength = (mdlCharBuffer[0] & 0xf) * 4;
		short totalLength = (mdlCharBuffer[2] << 8) + (mdlCharBuffer[3] & 0xFF);
		short transportDataLength = totalLength - ipHeaderLength;


		//����IPУ���
		CHAR currentCheckSum1 = mdlCharBuffer[10];
		CHAR currentCheckSum2 = mdlCharBuffer[11];

		//У�������
		mdlCharBuffer[10] = 0;
		mdlCharBuffer[11] = 0;

		unsigned short checkSum = CalculateCheckSum(mdlCharBuffer, NULL, ipHeaderLength, 0, 2);
		
#if DBG
		//����
		CHAR checkSum1 = (CHAR)((checkSum & 0xff00) >> 8);
		CHAR checkSum2 = (CHAR)((checkSum & 0xff));

		if (checkSum1 != currentCheckSum1 || checkSum2 != currentCheckSum2)
		{
			//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "IP header checksum calculation in receive path error!\n");
		}
#endif

		mdlCharBuffer[10] = (CHAR)((checkSum & 0xff00) >> 8);
		mdlCharBuffer[11] = (CHAR)(checkSum & 0xff);

		switch (protocal)
		{
		case 6: //TCPЭ��
		{
			CalculateTcpPacketCheckSum(transportDataLength, mdlCharBuffer, protocal, ipHeaderLength, packet);
		}
		break;
		case 17: //UDPЭ��
			break;
		}

		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "ModifiedMDL: %s\t\n", (char*)outputs);
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
	NTSTATUS status = 0;
	NET_BUFFER_LIST* packet;
	FWPS_STREAM_DATA* streamData;
	SIZE_T dataLength = 0;
	SIZE_T bytes = 0;
	NDIS_STATUS ndisStatus;

	PNET_BUFFER_LIST clonedPacket = NULL;
	FWPS_PACKET_INJECTION_STATE injectionState = FWPS_PACKET_NOT_INJECTED;

	packet = (NET_BUFFER_LIST*)layerData;
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "Original Net Buffer List:\t\n");
	PrintNetBufferList(packet, DPFLTR_TRACE_LEVEL);
	classifyOut->actionType = FWP_ACTION_PERMIT;

	if (packet)
	{
		if (NET_BUFFER_LIST_NEXT_NBL(packet))
		{
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "The net buffer list link to another list. \n");
		}

		if (NET_BUFFER_NEXT_NB(NET_BUFFER_LIST_FIRST_NB(packet)))
		{
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "The net buffer link to another buffer. \n");
		}
	}


	if (SendInjectHandle != NULL && filter->filterId == filterId && packet)
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

			if (NT_SUCCESS(status))
			{
				//������ݰ������������ɹ�
				ModifySendIPPacket(clonedPacket);

				//���У���


				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "Modified Net Buffer List: \n");
				PrintNetBufferList(clonedPacket, DPFLTR_TRACE_LEVEL);

				//status = FwpsInjectNetworkSendAsync0(InjectHandle, NULL, 0, UNSPECIFIED_COMPARTMENT_ID, clonedPacket, InjectCompleted, NULL);
				status = FwpsInjectNetworkSendAsync0(SendInjectHandle, NULL, 0, inMetaValues->compartmentId, clonedPacket, SendInjectCompleted, NULL);

				//���ע��ʧ�ܣ����������ͨ����
				if (!NT_SUCCESS(status))
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Send Inject Failed\n");
					classifyOut->actionType = FWP_ACTION_PERMIT;
				}
				else
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Send Inject Success\n");
					//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Ready to block\n");
					classifyOut->actionType = FWP_ACTION_BLOCK;
					classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
				}
			}
			else
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
			}

		}
	}
	else if (ReceiveInjectHandle != NULL && filter->filterId == filterId2)
	{
		injectionState = FwpsQueryPacketInjectionState0(ReceiveInjectHandle, packet, NULL);
		//�����������ݰ�������
		if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
			injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
		}
		//�����ݰ����Ǳ��ֶ�ע������ݰ�
		else if (injectionState == FWPS_PACKET_NOT_INJECTED)
		{
			UINT32 ipHeaderSize = 0;

			if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_IP_HEADER_SIZE))
			{
				ipHeaderSize = inMetaValues->ipHeaderSize;
			}

			NdisRetreatNetBufferListDataStart(packet, ipHeaderSize, 0, NULL, NULL);
			status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL, 0, &clonedPacket);
			NdisAdvanceNetBufferListDataStart(packet, ipHeaderSize, FALSE, 0);
			if (NT_SUCCESS(status))
			{
				//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Retreat: \n");
				//PrintNetBufferList(clonedPacket);

				//������ݰ������������ɹ�
				ModifyReceiveIPPacket(clonedPacket);

				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "Modified Receving Net Buffer List: \n");
				PrintNetBufferList(clonedPacket, DPFLTR_TRACE_LEVEL);

				//status = FwpsInjectNetworkSendAsync0(InjectHandle, NULL, 0, UNSPECIFIED_COMPARTMENT_ID, clonedPacket, InjectCompleted, NULL);
				//status = FwpsInjectNetworkSendAsync0(ReceiveInjectHandle, NULL, 0, inMetaValues->compartmentId, clonedPacket, ReceiveInjectCompleted, NULL);

				FWPS_INCOMING_VALUE ifIndex = inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX];
				FWPS_INCOMING_VALUE subIfIndex = inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX];
				status = FwpsInjectNetworkReceiveAsync0(ReceiveInjectHandle, NULL, 0,
					(inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_COMPARTMENT_ID ? inMetaValues->compartmentId : UNSPECIFIED_COMPARTMENT_ID),
					inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX].value.uint32,
					inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_SUB_INTERFACE_INDEX].value.uint32,
					clonedPacket, ReceiveInjectCompleted, NULL);

				//���ע��ʧ�ܣ����������ͨ����
				if (!NT_SUCCESS(status))
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Receive Inject Failed\n");
				}
				else
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Receive Inject Start\n");
					//DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Ready to block\n");
					classifyOut->actionType = FWP_ACTION_BLOCK;
					classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
				}
			}
		}
	}
}

VOID NTAPI ReceiveClassifyFn(
	_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_ const FWPS_FILTER0* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
	UINT32 ipHeaderSize = inMetaValues->ipHeaderSize;
	NDIS_TCP_IP_CHECKSUM_PACKET_INFO checkSum = { 0 };
	if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_IP_HEADER_SIZE))
	{
	}

	checkSum.Value = (ULONG)(ULONG_PTR)NET_BUFFER_LIST_INFO((NET_BUFFER_LIST*)layerData, TcpIpChecksumNetBufferListInfo);
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

NTSTATUS CreateInjectors()
{
	NTSTATUS status;

	status = FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_NETWORK, &SendInjectHandle);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_NETWORK, &ReceiveInjectHandle);

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