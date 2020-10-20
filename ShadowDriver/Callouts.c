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

		//获取数据缓冲区
		PBYTE dataBuffer = (PBYTE)NdisGetDataBuffer(netBuffer, netBuffer->DataLength, NULL, 1, 0);

		//从MDL中提取信息
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

		//修改IP地址
		for (int i = 0; i < 4; ++i, ++ipAddrPos)
		{
			ipAddrPos[i] = modifiedAddress[i];
		}

		//INT32 sum = 0;
		////将校验和置零
		//dataBuffer[10] = dataBuffer[11] = 0;

		////计算校验和
		//for (int i = 0; i < 10; i += 2)
		//{
		//	WORD dBytes = dataBuffer[i];
		//	dBytes = (dBytes << 16) + dataBuffer[i+1];

		//	sum += dBytes;
		//}

		////处理进位部分
		//while (sum > 0xFFFF)
		//{
		//	UINT32 carryPart = (sum & 0x00FF0000) >> 16;
		//	sum = sum & 0xFFFF;
		//	sum += carryPart;
		//}

		////截取最低4字节
		//DWORD dwordSum = sum & 0xFFFF;

		////取反码
		//dwordSum = ~dwordSum;

		////填充进数据包
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

	//暂时先让这段不进行观察过去的包
	if (SendInjectHandle != NULL && FALSE)
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

			//如果为克隆的数据包创建缓冲区失败的话，则阻断这个数据包。
			if (!NT_SUCCESS(status))
			{

			}
			//如果数据包缓冲区创建成功
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
		classifyOut->actionType = FWP_ACTION_PERMIT;
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