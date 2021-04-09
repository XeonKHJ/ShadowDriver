#include "ShaDriHelper.h"

void ConvertNetBufferListToTcpRawPacket(_Out_ ShadowTcpRawPacket** tcpRawPacketPoint, _In_ PNET_BUFFER_LIST nbl, _In_ UINT32 nblOffset)
{

	//为ShadowTcpRawPacket分配内存空间。
	*tcpRawPacketPoint = (ShadowTcpRawPacket*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(ShadowTcpRawPacket), 'strp');

	if (*tcpRawPacketPoint)
	{
		ShadowTcpRawPacket* tcpRawPacket = *tcpRawPacketPoint;
		PacketDataBuffer** dataPos = &(tcpRawPacket->Data);

		//初始化循环条件
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
				//位移还没有结束
				if (offsetTrace > 0 && currentRemainBytes <= offsetTrace)
				{
					offsetTrace -= currentRemainBytes;
					continue;
				}
				else if (offsetTrace > 0 && currentRemainBytes > offsetTrace)
				{
					//在这里位移
					packetStartPos = mdlAddr + offsetTrace;
					offsetRemainInMDL = (currentRemainBytes)-offsetTrace + 1;

					currentRemainBytes -= offsetTrace;
					offsetTrace = 0;
					mdlAddr = packetStartPos;
				}

				//开始解析TCP报文段。
				if (offsetTrace == 0)
				{
					//获取TCP长度。
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
						//这个记得换
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

	//TCP报文段在缓冲区的起始指针
	PCHAR tcpStartPos = &(mdlCharBuffer[ipHeaderLength]);

	CHAR tcpOriginalCheckSum1 = tcpStartPos[16];
	CHAR tcpOriginalCheckSum2 = tcpStartPos[17];
	unsigned short originalCheckSum = ((tcpOriginalCheckSum1 << 8) & 0xFF00) + (tcpOriginalCheckSum2 & 0xFF);


	//将TCP的校验和位置零
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

	//将校验和填充到TCP报文段中。
	tcpStartPos[16] = (CHAR)((sum & 0xff00) >> 8);
	tcpStartPos[17] = (CHAR)(sum & 0xff);
}

void DeleteTcpRawPacket(_In_ ShadowTcpRawPacket* tcpRawPacket)
{
	//先释放里面的PacketDataBuffer数据结构
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
	//打印IP报
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