#include "ShaDriHelper.h"

void ConvertNetBufferListToTcpRawPacket(_Out_ ShadowTcpRawPacket** tcpRawPacketPoint, _In_ PNET_BUFFER_LIST nbl, _In_ UINT32 nblOffset)
{

	//ΪShadowTcpRawPacket�����ڴ�ռ䡣
	*tcpRawPacketPoint = (ShadowTcpRawPacket*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(ShadowTcpRawPacket), 'strp');

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
				PCHAR mdlAddr = (PCHAR)MmGetMdlVirtualAddress(currentMdl);
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
						*dataPos = (PacketDataBuffer*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(PacketDataBuffer), 'pdb');
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

void CalculateTcpPacketCheckSum(short transportDataLength, PCHAR mdlCharBuffer, CHAR protocal, short ipHeaderLength, PNET_BUFFER_LIST packet)
{
	//if (transportDataLength % 2 == 1)
	//{
	//	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "TCP header length is odd\n");
	//	if (transportDataLength == 0x55d)
	//	{
	//		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "TCP header length is 0x55d\n");
	//	}
	//}

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

	//----------------------------------------------------------------------
	ConvertNetBufferListToTcpRawPacket(&rawPacket, packet, ipHeaderLength);
	unsigned short sum = CalculateCheckSum1(rawPacket, fakeHeader, 12, 2);
	DeleteTcpRawPacket(rawPacket);

//	if (sum != originalCheckSum)
//	{
//		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "TCP header checksum calculation in send path error!\n");
//
//#if DBG
//		ConvertNetBufferListToTcpRawPacket(&rawPacket, packet, ipHeaderLength);
//		unsigned short sum = CalculateCheckSum1(rawPacket, fakeHeader, 12, 2);
//		DeleteTcpRawPacket(rawPacket);
//#endif
//	}
	//-----------------------------------------------------------------------

	//��У�����䵽TCP���Ķ��С�
	tcpStartPos[16] = (CHAR)((sum & 0xff00) >> 8);
	tcpStartPos[17] = (CHAR)(sum & 0xff);
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

//void CalculateIpPacketCheckSum();

void PrintNetBufferList(PNET_BUFFER_LIST packet, ULONG level)
{
	PVOID dataBuffer = NULL;
	PCHAR outputs = NULL;
	PVOID copiedDataBuffer = NULL;
	//��ӡIP��
	if (packet->FirstNetBuffer != NULL)
	{

		DbgPrintEx(DPFLTR_IHVNETWORK_ID, level, "Start printing net buffer list:\t\n");
		for (PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet); netBuffer; netBuffer = NET_BUFFER_NEXT_NB(netBuffer))
		{
			//NdisRetreatNetBufferDataStart(netBuffer, 0, 0, NULL);
			ULONG dataLength = NET_BUFFER_DATA_LENGTH(netBuffer);

			copiedDataBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, dataLength, 'cpk');

			while (dataBuffer == NULL)
			{
				dataBuffer = NdisGetDataBuffer(netBuffer, dataLength, copiedDataBuffer, 1, 0);
				--dataLength;
			}
			++dataLength;

			if (dataBuffer != NULL)
			{
				size_t outputLength = CaculateHexStringLength(dataLength);
				outputs = (PCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, outputLength, 'op');

				if (outputs != NULL)
				{
					ConvertBytesArrayToHexString((char *)dataBuffer, dataLength, outputs, outputLength);
					
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, level, "%s\t\n", outputs);
					ExFreePoolWithTag(outputs, 'op');
				}
			}
			else
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "DataBuffer Fetch Failed!\t\n");
			}
		}
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, level, "Finish printing net buffer list.\t\n");
	}
}