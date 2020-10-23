/*++

Module Name:

	Callout.c

Abstract:

	包含了关于WFP中包的捕获和转发的主要逻辑函数。

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"
#include "Callouts.h"

//当回调函数向设备注册时获得。
UINT32 WpsSendCalloutId;
UINT32 WpsReceiveCalloutId;

//当回调函数添加进过滤引擎时获得。
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
	//打印IP报
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
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Completed\n");
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
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Completed\n");
	}
	FwpsFreeCloneNetBufferList0(netBufferList, 0);
}

VOID ModifySendIPPacket(PNET_BUFFER_LIST packet)
{
	if (packet != NULL)
	{
		PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet);

		//获取数据缓冲区
		PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, netBuffer->DataLength, NULL, 1, 0);

		//从MDL中提取信息
		PVOID mdlBuffer = MmGetMdlVirtualAddress(netBuffer->CurrentMdl);
		PCHAR mdlCharBuffer = (PCHAR)mdlBuffer;
		//dataBuffer = (PBYTE)mdlBuffer;

		int mdlStringLength = CaculateHexStringLength(netBuffer->CurrentMdl->ByteCount);
		PVOID outputs = ExAllocatePoolWithTag(NonPagedPool, mdlStringLength, 'op');
		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "CurrentMDL: %s\t\n", outputs);

		
		((PCHAR)mdlBuffer)[16] = (CHAR)192;
		((PCHAR)mdlBuffer)[17] = (CHAR)168;
		((PCHAR)mdlBuffer)[18] = (CHAR)1;
		((PCHAR)mdlBuffer)[19] = (CHAR)103;
		

		//如果传输层协议时TCP或者UDP，则要重新计算TCP或者UDP的校验和。
		//检查协议
		CHAR protocal = mdlCharBuffer[9];

		//获取网络层以上的数据长度（不包括网络层）。
		short ipHeaderLength = (mdlCharBuffer[0] & 0xf) * 4;
		short totalLength = (mdlCharBuffer[2] << 8) + mdlCharBuffer[3];
		short transportDataLength = totalLength - ipHeaderLength;

		switch (protocal)
		{
		case 6: //TCP协议
		{
			CHAR fakeHeader[] = { mdlCharBuffer[12] , mdlCharBuffer[13] , mdlCharBuffer[14] , mdlCharBuffer[15],
								  mdlCharBuffer[16] , mdlCharBuffer[17] , mdlCharBuffer[18] , mdlCharBuffer[19],
								  (CHAR)0, protocal, (CHAR)((transportDataLength & 0xff00) >> 8), (CHAR)(transportDataLength & 0xff) };

			//TCP报文段在缓冲区的起始指针
			PCHAR tcpStartPos = &(mdlCharBuffer[ipHeaderLength]);

			//将TCP的校验和位置零
			tcpStartPos[16] = tcpStartPos[17] = 0;

			//计算校验和
			unsigned short checkSum = CalculateCheckSum(tcpStartPos, fakeHeader, transportDataLength, 12);

			//将校验和填充到TCP报文段中。
			tcpStartPos[16] = (CHAR)((checkSum & 0xff00) >> 8);
			tcpStartPos[17] = (CHAR)(checkSum & 0xff);
		}
		break;
		case 17: //UDP协议
			break;
		}


		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "ModifiedMDL: %s\t\n", outputs);
	}
}

VOID ModifyReceiveIPPacket(PNET_BUFFER_LIST packet)
{
	if (packet != NULL)
	{
		PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(packet);

		////获取数据缓冲区
		//PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, netBuffer->DataLength, NULL, 1, 0);

		//从MDL中提取信息
		PVOID mdlBuffer = MmGetMdlVirtualAddress(netBuffer->CurrentMdl);

		//dataBuffer = (PBYTE)mdlBuffer;

		/*++++++打印+++++++*/
		int mdlStringLength = CaculateHexStringLength(netBuffer->CurrentMdl->ByteCount);
		PVOID outputs = ExAllocatePoolWithTag(NonPagedPool, mdlStringLength, 'op');
		ConvertBytesArrayToHexString(mdlBuffer, netBuffer->CurrentMdl->ByteCount, outputs, mdlStringLength);
		DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "CurrentMDL: %s\t\n", outputs);
		/*------打印-------*/

		PCHAR mdlCharBuffer = (PCHAR)mdlBuffer;

		//修改目标IP地址
		/*
		mdlCharBuffer[12] = (CHAR)192;
		mdlCharBuffer[13] = (CHAR)168;
		mdlCharBuffer[14] = (CHAR)1;
		mdlCharBuffer[15] = (CHAR)103;
		*/

		

		//如果传输层协议时TCP或者UDP，则要重新计算TCP或者UDP的校验和。
		//检查协议
		CHAR protocal = mdlCharBuffer[9];

		//获取网络层以上的数据长度（不包括网络层）。
		short ipHeaderLength = (mdlCharBuffer[0] & 0xff) * 4;
		short totalLength = (mdlCharBuffer[2] << 8) + mdlCharBuffer[3];
		short transportDataLength = totalLength - ipHeaderLength;

		switch (protocal)
		{
		case 6: //TCP协议
		{
			CHAR fakeHeader[] = { mdlCharBuffer[8] , mdlCharBuffer[9] , mdlCharBuffer[10] , mdlCharBuffer[11],
								  mdlCharBuffer[12] , mdlCharBuffer[13] , mdlCharBuffer[14] , mdlCharBuffer[15],
								  (CHAR)0, protocal, (CHAR)((transportDataLength & 0xff00) >> 8), (CHAR)(transportDataLength & 0xff)};

			//TCP报文段在缓冲区的起始指针
			PCHAR tcpStartPos =  &(mdlCharBuffer[ipHeaderLength - 1]);

			//将TCP的校验和位置零
			tcpStartPos[16] = tcpStartPos[17] = 0;

			//计算校验和
			unsigned short checkSum = CalculateCheckSum(tcpStartPos, fakeHeader, transportDataLength, ipHeaderLength);

			//将校验和填充到TCP报文段中。
			tcpStartPos[16] = (CHAR)((checkSum & 0xff00) >> 8);
			tcpStartPos[17] = (CHAR)(checkSum & 0xff);
		}
			break;
		case 17: //UDP协议
			break;
		}

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
	NDIS_STATUS ndisStatus;

	PNET_BUFFER_LIST clonedPacket = NULL;
	FWPS_PACKET_INJECTION_STATE injectionState = FWPS_PACKET_NOT_INJECTED;

	packet = (NET_BUFFER_LIST*)layerData;
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Original Net Buffer List:\t\n");
	PrintNetBufferList(packet);
	classifyOut->actionType = FWP_ACTION_PERMIT;

	//暂时先让这段不进行观察过去的包
	if (SendInjectHandle != NULL && filter->filterId == filterId)
	{
		injectionState = FwpsQueryPacketInjectionState0(SendInjectHandle, packet, NULL);

		//如果捕获的数据包是主动
		if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
			injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			//PrintNetBufferList(packet);
		}
		//该数据包不是被手动注入的数据包
		else if (injectionState == FWPS_PACKET_NOT_INJECTED)
		{
			status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL, 0, &clonedPacket);

			if (NT_SUCCESS(status))
			{
				//如果数据包缓冲区创建成功
				ModifySendIPPacket(clonedPacket);

				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Modified Net Buffer List: \n");
				PrintNetBufferList(clonedPacket);

				//status = FwpsInjectNetworkSendAsync0(InjectHandle, NULL, 0, UNSPECIFIED_COMPARTMENT_ID, clonedPacket, InjectCompleted, NULL);
				status = FwpsInjectNetworkSendAsync0(SendInjectHandle, NULL, 0, inMetaValues->compartmentId, clonedPacket, SendInjectCompleted, NULL);

				//如果注入失败，则令包正常通过。
				if (!NT_SUCCESS(status))
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Failed\n");
					classifyOut->actionType = FWP_ACTION_PERMIT;
				}
				else
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Success\n");
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Ready to block\n");
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
		//如果捕获的数据包是主动
		if (injectionState == FWPS_PACKET_INJECTED_BY_SELF ||
			injectionState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
		}
		//该数据包不是被手动注入的数据包
		else if (injectionState == FWPS_PACKET_NOT_INJECTED)
		{
			NdisRetreatNetBufferListDataStart(packet, 20, 0, NULL, NULL);
			status = FwpsAllocateCloneNetBufferList0(packet, NULL, NULL, 0, &clonedPacket);

			if (NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Retreat: \n");
				PrintNetBufferList(clonedPacket);

				//如果数据包缓冲区创建成功
				ModifyReceiveIPPacket(clonedPacket);

				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Modified Net Buffer List: \n");
				PrintNetBufferList(clonedPacket);

				//status = FwpsInjectNetworkSendAsync0(InjectHandle, NULL, 0, UNSPECIFIED_COMPARTMENT_ID, clonedPacket, InjectCompleted, NULL);
				//status = FwpsInjectNetworkSendAsync0(ReceiveInjectHandle, NULL, 0, inMetaValues->compartmentId, clonedPacket, ReceiveInjectCompleted, NULL);
				status = FwpsInjectNetworkReceiveAsync0(ReceiveInjectHandle, NULL, 0, inMetaValues->compartmentId, inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX].value.uint32, inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_SUB_INTERFACE_INDEX].value.uint32, clonedPacket, ReceiveInjectCompleted, NULL);

				//如果注入失败，则令包正常通过。
				if (!NT_SUCCESS(status))
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Failed\n");
				}
				else
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Inject Start\n");
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_INFO_LEVEL, "Ready to block\n");
					classifyOut->actionType = FWP_ACTION_BLOCK;
					classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
				}
			}
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
/// 添加回调到过滤引擎
/// </summary>
/// <param name="engineHandle">过滤引擎句柄</param>
/// <returns>状态</returns>
NTSTATUS AddCalloutToWfp(IN HANDLE engineHandle)
{
	NTSTATUS status;
	FWPM_CALLOUT0 sendCallout = { 0 };

	//用于捕捉再发送道路上的数据包
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

	//用于捕捉在接收道路上的数据包
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
/// 向过滤引擎和设备注销回调。
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