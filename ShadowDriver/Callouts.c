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

void ConvertNetBufferListToTcpRawPacket(_Out_ ShadowTcpRawPacket** tcpRawPacketPoint, _In_ PNET_BUFFER_LIST nbl, _In_ UINT32 nblOffset)
{
	PNET_BUFFER firstNb = NET_BUFFER_LIST_FIRST_NB(nbl);
	PMDL firstMdl = NET_BUFFER_FIRST_MDL(firstNb);

	//ΪShadowTcpRawPacket�����ڴ�ռ䡣
	*tcpRawPacketPoint = (ShadowTcpRawPacket*)ExAllocatePoolWithTag(NonPagedPool, sizeof(ShadowTcpRawPacket), 'strp');

	if (*tcpRawPacketPoint)
	{
		ShadowTcpRawPacket* tcpRawPacket = *tcpRawPacketPoint;
		PacketDataBuffer** dataPos = &(tcpRawPacket->Data);

		//��ʼ��ѭ������
		size_t offsetTrace = nblOffset;
		size_t offsetRemainInMDL = 0;
		size_t headerOffsetTrace = 0;
		size_t tcpHeaderLength = 20;
		BOOL isHeaderParsed = FALSE;
		BOOL isHeaderLengthAcquired = FALSE;
		PCHAR packetStartPos;
		for (PNET_BUFFER currentNb = NET_BUFFER_LIST_FIRST_NB(nbl); currentNb; currentNb = NET_BUFFER_NEXT_NB(currentNb))
		{
			PMDL firstMdlPerNb = NET_BUFFER_FIRST_MDL(currentNb);
			for (PMDL currentMdl = firstMdlPerNb; currentMdl; currentMdl = currentMdl->Next)
			{
				PCHAR mdlAddr = (PCHAR)MmGetMdlVirtualAddress(firstMdl);
				int currentRemainBytes = currentMdl->ByteCount;
				//λ�ƻ�û�н���
				if (offsetTrace > 0 && currentRemainBytes <= offsetTrace)
				{
					offsetTrace -= currentRemainBytes;
					continue;
				}
				else if (offsetTrace > 0 && currentRemainBytes > offsetTrace)
				{
					//������λ��
					packetStartPos = mdlAddr + offsetTrace;
					offsetRemainInMDL = (currentRemainBytes)-offsetTrace + 1;

					currentRemainBytes -= offsetTrace;
					offsetTrace = 0;
					mdlAddr = packetStartPos;
				}

				//��ʼ����TCP���ĶΡ�
				if (offsetTrace == 0)
				{
					//��ȡTCP���ȡ�
					if (!isHeaderParsed)
					{
						for (size_t initPos = 0; initPos < currentRemainBytes && headerOffsetTrace < tcpHeaderLength; ++initPos, ++headerOffsetTrace)
						{
							switch (headerOffsetTrace)
							{
							case 0:
								break;
							case 1:
								break;
							case 2:
								break;
							case 3:
								break;
							case 4:
								break;
							case 6:
								break;
							case 7:
								break;
							case 8:
								break;
							case 9:
								break;
							case 10:
								break;
							case 11:
								break;
							case 12:
								break;
							case 13:
								break;
							case 14:
								break;
							case 15:
								break;
							case 16:
								break;
							case 17:
								break;
							case 18:
								break;
							case 19:
								break;
							}
						}

						if (headerOffsetTrace == tcpHeaderLength)
						{
							isHeaderParsed = TRUE;
						}
					}

					if (isHeaderParsed)
					{
						//����ǵû�
						*dataPos = (PacketDataBuffer*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PacketDataBuffer), 'pdb');
						PacketDataBuffer* currentDataBuffer = *dataPos;
						currentDataBuffer->Bytes = currentRemainBytes;
						currentDataBuffer->CurrentBuffer = mdlAddr;
						currentDataBuffer->NextBuffer = NULL;
						dataPos = &(currentDataBuffer->NextBuffer);
					}
				}
			}
		}
	}
	else
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Allocate pool for tcpRawPacketPoint failed.");
	}
}

void DeleteTcpRawPacket(_In_ ShadowTcpRawPacket* tcpRawPacket)
{
	//���ͷ������PacketDataBuffer���ݽṹ
	int chainNumber = 0;
	for (PacketDataBuffer* buffer = tcpRawPacket->Data; buffer; buffer = buffer->NextBuffer)
	{
		++chainNumber;
	}

	PacketDataBuffer* nextBuffer;
	PacketDataBuffer* previousBuffer;
	for (PacketDataBuffer* buffer = tcpRawPacket->Data; buffer; )
	{
		previousBuffer = buffer;
		buffer = buffer->NextBuffer;
		ExFreePoolWithTag(previousBuffer, 'pdb');
	}

	ExFreePoolWithTag(tcpRawPacket, 'strp');
}

void PrintNetBufferList(PNET_BUFFER_LIST packet, ULONG level)
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

				DbgPrintEx(DPFLTR_IHVNETWORK_ID, level, "%s\t\n", outputs);
				ExFreePoolWithTag(outputs, 'op');
			}
		}
		else
		{
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DataBuffer Fetch Failed!\t\n");
		}
	}
}

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
		short totalLength = (mdlCharBuffer[2] << 8) + mdlCharBuffer[3];
		short transportDataLength = totalLength - ipHeaderLength;

		switch (protocal)
		{
		case 6: //TCPЭ��
		{
			if (transportDataLength % 2 == 1)
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "TCP header length is odd\n");
			}

			CHAR fakeHeader[] = { mdlCharBuffer[12] , mdlCharBuffer[13] , mdlCharBuffer[14] , mdlCharBuffer[15],
								  mdlCharBuffer[16] , mdlCharBuffer[17] , mdlCharBuffer[18] , mdlCharBuffer[19],
								  (CHAR)0, protocal, (CHAR)((transportDataLength & 0xff00) >> 8), (CHAR)(transportDataLength & 0xff) };

			ShadowTcpRawPacket* rawPacket;

			//TCP���Ķ��ڻ���������ʼָ��
			PCHAR tcpStartPos = &(mdlCharBuffer[ipHeaderLength]);

			CHAR tcpOriginalCheckSum1 = tcpStartPos[16];
			CHAR tcpOriginalCheckSum2 = tcpStartPos[17];
			unsigned short originalCheckSum = ((tcpOriginalCheckSum1 << 8) & 0xFF00) + (tcpOriginalCheckSum2 & 0xFF);
			

			//��TCP��У���λ����
			tcpStartPos[16] = tcpStartPos[17] = 0;


			//����У���
			unsigned short checkSum = CalculateCheckSum(tcpStartPos, fakeHeader, transportDataLength, 12, 2);

			

			//----------------------------------------------------------------------
			ConvertNetBufferListToTcpRawPacket(&rawPacket, packet, ipHeaderLength);
			unsigned short sum = CalculateCheckSum1(rawPacket, fakeHeader, 12, 2);
			DeleteTcpRawPacket(rawPacket);

			if (sum != originalCheckSum)
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Test: TCP header checksum calculation in send path error!\n");
			}
			//-----------------------------------------------------------------------

			CHAR tcpCheckSum1 = (CHAR)((checkSum & 0xff00) >> 8);
			CHAR tcpCheckSum2 = (CHAR)(checkSum & 0xff);

			if (tcpOriginalCheckSum1 != tcpCheckSum1 || tcpOriginalCheckSum2 != tcpCheckSum2)
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "TCP header checksum calculation in send path error!\n");
			}

			//��У�����䵽TCP���Ķ��С�
			tcpStartPos[16] = (CHAR)((checkSum & 0xff00) >> 8);
			tcpStartPos[17] = (CHAR)(checkSum & 0xff);
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
		mdlCharBuffer[14] = (CHAR)1;
		mdlCharBuffer[15] = (CHAR)106;


		//��������Э��ʱTCP����UDP����Ҫ���¼���TCP����UDP��У��͡�
		//���Э��
		CHAR protocal = mdlCharBuffer[9];

		//��ȡ��������ϵ����ݳ��ȣ�����������㣩��
		short ipHeaderLength = (mdlCharBuffer[0] & 0xf) * 4;
		short totalLength = (mdlCharBuffer[2] << 8) + mdlCharBuffer[3];
		short transportDataLength = totalLength - ipHeaderLength;


		//����IPУ���
		CHAR currentCheckSum1 = mdlCharBuffer[10];
		CHAR currentCheckSum2 = mdlCharBuffer[11];

		//У�������
		mdlCharBuffer[10] = 0;
		mdlCharBuffer[11] = 0;

		unsigned short checkSum = CalculateCheckSum(mdlCharBuffer, NULL, ipHeaderLength, 0, 2);

		//����
		CHAR checkSum1 = (CHAR)((checkSum & 0xff00) >> 8);
		CHAR checkSum2 = (CHAR)((checkSum & 0xff));

		if (checkSum1 != currentCheckSum1 || checkSum2 != currentCheckSum2)
		{
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "IP header checksum calculation in receive path error!\n");
		}

		mdlCharBuffer[10] = (CHAR)((checkSum & 0xff00) >> 8);
		mdlCharBuffer[11] = (CHAR)(checkSum & 0xff);

		switch (protocal)
		{
		case 6: //TCPЭ��
		{
			CHAR fakeHeader[] = { mdlCharBuffer[12] , mdlCharBuffer[13] , mdlCharBuffer[14] , mdlCharBuffer[15],
								  mdlCharBuffer[16] , mdlCharBuffer[17] , mdlCharBuffer[18] , mdlCharBuffer[19],
								  (CHAR)0, protocal, (CHAR)((transportDataLength & 0xff00) >> 8), (CHAR)(transportDataLength & 0xff) };

			//TCP���Ķ��ڻ���������ʼָ��
			PCHAR tcpStartPos = &(mdlCharBuffer[ipHeaderLength]);


			CHAR tcpOriginalCheckSum1 = tcpStartPos[16];
			CHAR tcpOriginalCheckSum2 = tcpStartPos[17];

			//��TCP��У���λ����
			tcpStartPos[16] = tcpStartPos[17] = 0;

			//����У���
			unsigned short checkSum = CalculateCheckSum(tcpStartPos, fakeHeader, transportDataLength, 12, 2);

			CHAR tcpCheckSum1 = (CHAR)((checkSum & 0xff00) >> 8);
			CHAR tcpCheckSum2 = (CHAR)(checkSum & 0xff);

			if (tcpOriginalCheckSum1 != tcpCheckSum1 || tcpOriginalCheckSum2 != tcpCheckSum2)
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "TCP header checksum calculation in receive path error!\n");
			}

			//��У�����䵽TCP���Ķ��С�
			tcpStartPos[16] = (CHAR)((checkSum & 0xff00) >> 8);
			tcpStartPos[17] = (CHAR)(checkSum & 0xff);
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