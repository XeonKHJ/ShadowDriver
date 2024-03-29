#include "IOCTLHelper.h"
#include "ShadowFilterContext.h"
#include "CancelSafeQueueCallouts.h"
#include "PacketHelper.h"
#include "InjectionHelper.h"
#include "ShadowCallouts.h"

int IOCTLHelper::_helperCount = 0;
IOCTLHelperLinkEntry IOCTLHelper::_helperListHeader;
PDEVICE_OBJECT IOCTLHelper::_deviceObject;

IOCTLHelper::IOCTLHelper(IOCTLHelperContext context)
{
	_context = context;
	InitializeIRPNotificationSystem();
}

IOCTLHelper::~IOCTLHelper()
{
	CancelAllPendingNotifyIoctls();
}

void IOCTLHelper::InitializeDriverObjectForIOCTL(_In_ PDRIVER_OBJECT driverObject)
{
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ShadowDriverIrpIoControl;
	driverObject->MajorFunction[IRP_MJ_CLEANUP] = ShadowDriverIrpCleanUp;
	driverObject->MajorFunction[IRP_MJ_CLOSE] = ShadowDriverIrpClose;
	InitializeListHead(&(_helperListHeader.ListEntry));
}

NTSTATUS IOCTLHelper::NotifyUserApp(void* buffer, size_t size)
{
	NTSTATUS status = STATUS_SUCCESS;

	NotifyUserByDequeuingIoctl(&_context, buffer, size);

	return status;
}

void IOCTLHelper::SetDeviceObject(PDEVICE_OBJECT deviceObject)
{
	IOCTLHelper::_deviceObject = deviceObject;
}

void IOCTLHelper::AddHelper(IOCTLHelper* helper)
{
	IOCTLHelper::_helperCount++;
	IOCTLHelperLinkEntry* newEntry = new IOCTLHelperLinkEntry();
	newEntry->Helper = helper;
	InsertTailList(&(_helperListHeader.ListEntry), &(newEntry->ListEntry));
#if DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "AddHelper_end\n");
#endif
}

void IOCTLHelper::RemoveHelper(IOCTLHelper* helper)
{
	IOCTLHelperLinkEntry* currentEntry = &_helperListHeader;
	do
	{
		PLIST_ENTRY listEntry = currentEntry->ListEntry.Flink;
		currentEntry = CONTAINING_RECORD(listEntry, IOCTLHelperLinkEntry, ListEntry);

		// Delete IOCTLHelper instance.
		if (currentEntry->Helper == helper)
		{
			// Delete helper instance.
			delete helper;

			// Remove helper instance from list.
			RemoveEntryList(listEntry);
			_helperCount--;
			// Delete link list entry that containts deleted helper instance.
			delete currentEntry;
			break;
		}
	} while (currentEntry != &_helperListHeader);
}

/// <summary>
/// Filter IOCTL request.Executing corresponding commands.
/// </summary>
/// <param name="DeviceObject"></param>
/// <param name="Irp"></param>
/// <returns></returns>
NTSTATUS IOCTLHelper::ShadowDriverIrpIoControl(_In_ _DEVICE_OBJECT* DeviceObject, _Inout_ _IRP* Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
#ifdef DBG
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ShadowDriver_IRP_IoControl\n");
#endif
	NTSTATUS status = STATUS_SUCCESS;
	//取出IRP
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (pIoStackIrp)
	{
		Irp->IoStatus.Information = 0;
		ULONG ioControlCode = pIoStackIrp->Parameters.DeviceIoControl.IoControlCode;
		switch (ioControlCode)
		{
		case IOCTL_SHADOWDRIVER_APP_REGISTER:
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_START_WFP\n");
#endif
			status = IoctlRegisterApp(Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		case IOCTL_SHADOWDRIVER_APP_DEREGISTER:
			status = IoctlDeregisterApp(Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		case IOCTL_SHADOWDRIVER_START_FILTERING:
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_START_WFP\n");
#endif
			status = IoctlStartFiltering(Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		case IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO:
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO\n");
#endif
			//IoctlRequirePacketInfo(Irp);
			break;
		case IOCTL_SHADOWDRIVER_DEQUEUE_NOTIFICATION:
#if DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO_WTF\n");
#endif
			{
				//Complete manually dequeue request IOCTL.
				int appId = GetAppIdFromIoctl(Irp, pIoStackIrp);
				WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
				Irp->IoStatus.Status = status;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);

				//Dequeue an notificaton IOCTL.
				IOCTLHelper* helper = GetHelperByAppId(appId);
				NotifyUserByDequeuingIoctl(&helper->_context, NULL, 0);
			}
			break;
		case IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION:
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION\n");
#endif
			{
				int appId = GetAppIdFromIoctl(Irp, pIoStackIrp);
				IOCTLHelper* helper = GetHelperByAppId(appId);
				if (helper != nullptr)
				{
					status = IoCsqInsertIrpEx(&(helper->_context.IoCsq), Irp, NULL, NULL);
					status = STATUS_PENDING;
				}
				else
				{
					status = SHADOW_APP_UNREGISTERED;
					WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
					Irp->IoStatus.Status = status;
					IoCompleteRequest(Irp, IO_NO_INCREMENT);
				}
			}
			break;
		case IOCTL_SHADOWDRIVER_GET_DRIVER_VERSION:
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION\n");
#endif
			//IoctlGetDriverVersion(Irp, pIoStackIrp);
			break;
		case IOCTL_SHADOWDRIVER_GET_QUEUE_INFO:
		{
			status = IoctlGetQueuedIoctlCount(Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
		break;
		case IOCTL_SHADOWDRIVER_ADD_CONDITION:
		{
			status = IoctlAddCondition(Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
		break;
		case IOCTL_SHADOWDRIVER_STOP_FILTERING:
			status = IoctlStopFiltering(Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		case IOCTL_SHADOWDRIVER_GET_REGISTERED_APP_COUNT:
			status = STATUS_SUCCESS;
			WriteDataToIrpOutputBuffer(&_helperCount, sizeof(int), Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		case IOCTL_SHADOWDRIVER_ENABLE_MODIFICATION:
			status = STATUS_SUCCESS;
			status = IoctlEnableModification(Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		case IOCTL_SHADOWDRIVER_INJECT_PACKET:
			status = STATUS_SUCCESS;
			status = IoctlInjectPacket(Irp, pIoStackIrp);
			WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
#ifdef DBG
		case IOCTL_SHADOWDRIVER_DIRECT_IN_TEST:
		{

			auto inputBuffer = Irp->MdlAddress;
			UNREFERENCED_PARAMETER(inputBuffer);
			//int dataReadSize = sizeof(int);
			//auto inputBufferLength = pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength;
			//if (inputBufferLength >= (ULONG)dataReadSize)
			//{
			//	
			//	auto result = *((int*)(inputBuffer));
			//	UNREFERENCED_PARAMETER(result);
			//}
			auto addr = MmGetMdlVirtualAddress(inputBuffer);
			auto addrCount = MmGetMdlByteCount(inputBuffer);
			int appid = *(int*)addr;
			*(int*)addr = 30;
			//WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);

			UNREFERENCED_PARAMETER(addrCount);
			UNREFERENCED_PARAMETER(appid);

			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		}
		case IOCTL_SHADOWDRIVER_NEITHER_IO_TEST:
		{
			auto inputBuffer = Irp->UserBuffer;
			int appid = *(int*)inputBuffer;
			UNREFERENCED_PARAMETER(appid);
			*(int*)inputBuffer = 40;

			*(int *)pIoStackIrp->Parameters.DeviceIoControl.Type3InputBuffer = 30;

			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		}
#endif
		default:
#ifdef DBG 
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Received an unknown IOCTL!\t\n");
#endif
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		}
	}
	return status;
}

NTSTATUS IOCTLHelper::ShadowDriverIrpClose(_In_ _DEVICE_OBJECT* DeviceObject, _Inout_ _IRP* Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
#ifdef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ShadowDriverIrpClose\t\n");
#endif
	NTSTATUS status = STATUS_SUCCESS;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS IOCTLHelper::ShadowDriverIrpCleanUp(_In_ _DEVICE_OBJECT* DeviceObject, _Inout_ _IRP* Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
#ifdef DBG
	DbgPrintEx(DPFLTR_ENVIRON_ID, DPFLTR_TRACE_LEVEL, "ShadowDriverIrpCleanUp\t\n");
#endif
	auto fileObject = Irp->Tail.Overlay.OriginalFileObject;

	auto helper = GetHelperByOriginalFileObject(fileObject);

	if (helper != nullptr)
	{
#ifdef DBG
		DbgPrintEx(DPFLTR_ENVIRON_ID, DPFLTR_INFO_LEVEL, "Clean up file object pointer: %p\t\n", (void*)fileObject);
#endif

		auto filter = helper->_context.Filter;
		auto filterContext = helper->_context.FilterContext;

		// Delete helper.
		RemoveHelper(helper);

		//Delete ShadowFilter instance;
		delete filter;

		//Delete ShadowFilterContext instance.
		delete filterContext;
	}

	NTSTATUS status = STATUS_SUCCESS;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

AppRegisterContext IOCTLHelper::GetAppContextFromIoctl(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	PVOID inputBuffer = irp->AssociatedIrp.SystemBuffer;
	AppRegisterContext context{};
	int dataReadSize = AppRegisterContextMaxSize;
	auto inputBufferLength = ioStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	if (inputBufferLength >= (ULONG)dataReadSize)
	{
		context.AppId = *((int*)(inputBuffer));
		auto nameLength = AppRegisterContextMaxSize - StatusSize;
		void* nameBuffer = (void*)((char*)inputBuffer + sizeof(int));
		RtlCopyMemory(context.AppName, nameBuffer, nameLength);
	}
	return context;
}

int IOCTLHelper::GetAppIdFromIoctl(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	int result = 0;
	PVOID inputBuffer = irp->AssociatedIrp.SystemBuffer;
	int dataReadSize = sizeof(int);
	auto inputBufferLength = ioStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	if (inputBufferLength >= (ULONG)dataReadSize)
	{
		result = *((int*)(inputBuffer));
	}
	return result;
}

IOCTLHelper* IOCTLHelper::GetHelperByAppId(int id)
{
	IOCTLHelper* result = nullptr;
	IOCTLHelperLinkEntry* currentEntry = &_helperListHeader;
	do
	{
		PLIST_ENTRY listEntry = currentEntry->ListEntry.Flink;
		currentEntry = CONTAINING_RECORD(listEntry, IOCTLHelperLinkEntry, ListEntry);

		if (currentEntry->Helper != nullptr)
		{
			if (currentEntry->Helper->_context.AppContext.AppId == id)
			{
				result = currentEntry->Helper;
				break;
			}
		}
	} while (currentEntry != &_helperListHeader);
	return result;
}

IOCTLHelper* IOCTLHelper::GetHelperByOriginalFileObject(PFILE_OBJECT fileObject)
{
	IOCTLHelper* result = nullptr;
	IOCTLHelperLinkEntry* currentEntry = &_helperListHeader;
	do
	{
		PLIST_ENTRY listEntry = currentEntry->ListEntry.Flink;
		currentEntry = CONTAINING_RECORD(listEntry, IOCTLHelperLinkEntry, ListEntry);

		if (currentEntry->Helper != nullptr)
		{
			if (currentEntry->Helper->_context.OriginalFileObject == fileObject)
			{
				result = currentEntry->Helper;
				break;
			}
		}
	} while (currentEntry != &_helperListHeader);
	return result;
}

void IOCTLHelper::NotifyUserByDequeuingIoctl(IOCTLHelperContext* context, void* outputBuffer, size_t outputLength)
{
	NTSTATUS status = STATUS_SUCCESS;

#ifdef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NotifyUserByDequeuingIoctl\n");
#endif

	PIRP dequeuedIrp = IoCsqRemoveNextIrp(&(context->IoCsq), NULL);
	if (dequeuedIrp != NULL)
	{
		PIO_STACK_LOCATION dispatchedStackIrp = IoGetCurrentIrpStackLocation(dequeuedIrp);
		if (dispatchedStackIrp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION)
		{
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Dequeue a irp\n");
#endif
		}
		status = WriteDataToIrpOutputBuffer(outputBuffer, outputLength, dequeuedIrp, dispatchedStackIrp);
		WriteStatusToOutputBuffer(&status, dequeuedIrp, dispatchedStackIrp);

		dequeuedIrp->IoStatus.Status = status;
		IoCompleteRequest(dequeuedIrp, IO_NO_INCREMENT);
	}
}

NTSTATUS IOCTLHelper::InitializeIRPNotificationSystem()
{
	NTSTATUS status = STATUS_SUCCESS;

	// Initialize spin lock. This lock will be used on cancel-safe queue that manages the notification ioctls.
	KeInitializeSpinLock(&(_context.SpinLock));
	//初始化IRP取消安全队列数据结构
	status = IoCsqInitializeEx(&(_context.IoCsq), CsqInsertIrpEx, CsqRemoveIrp, CsqPeekNextIrp, CsqAcquireLock, CsqReleaseLock, CsqCompleteCanceledIrp);
	if (NT_SUCCESS(status))
	{
		InitializeListHead(&(_context.IrpLinkHeadEntry.ListEntry));
	}

	return status;
}

void IOCTLHelper::CancelAllPendingNotifyIoctls()
{
	// Initialize spin lock was define as a value not a reference, no need to delete it on purpose.
	// Csq is also the same.
	// But link list for queueing IOCTL needs to be deleted manually.
	auto currentEntry = _context.IrpLinkHeadEntry.ListEntry.Flink;
	while (!IsListEmpty(&(_context.IrpLinkHeadEntry.ListEntry)))
	{
		auto nextEntry = currentEntry->Flink;
		IRP_LINK_ENTRY* irpEntry = CONTAINING_RECORD(currentEntry, IRP_LINK_ENTRY, ListEntry);
		IoCancelIrp(irpEntry->Irp);
		currentEntry = nextEntry;
	}
}

NTSTATUS IOCTLHelper::IoctlDeregisterApp(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	auto appId = GetAppIdFromIoctl(irp, ioStackLocation);

	if (appId != 0)
	{
		IOCTLHelper* selectedHelper = GetHelperByAppId(appId);
		if (selectedHelper != nullptr)
		{
#if DBG
			DbgPrintEx(DPFLTR_ENVIRON_ID, DPFLTR_INFO_LEVEL, "Deregistering app from file object pointer: %p\t\n", (void*)(irp->Tail.Overlay.OriginalFileObject));
#endif

			auto filter = selectedHelper->_context.Filter;
			auto filterContext = selectedHelper->_context.FilterContext;

			// Delete helper.
			RemoveHelper(selectedHelper);

			//Delete ShadowFilter instance;
			delete filter;

			//Delete ShadowFilterContext instance.
			delete filterContext;
		}
		else
		{
			status = SHADOW_APP_UNREGISTERED;
		}
	}
	else
	{
		status = SHADOW_APP_UNREGISTERED;
	}
	return status;
}

NTSTATUS IOCTLHelper::IoctlGetQueuedIoctlCount(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int count = 0;;
	int appId = GetAppIdFromIoctl(irp, ioStackLocation);
	IOCTLHelper* helper = GetHelperByAppId(appId);
	if (helper != nullptr)
	{
		IRP_LINK_ENTRY* linkHeadEntry = &(helper->_context.IrpLinkHeadEntry);
		LIST_ENTRY* currentEntry = linkHeadEntry->ListEntry.Flink;
		while (currentEntry != &(linkHeadEntry->ListEntry))
		{
			++count;
			currentEntry = currentEntry->Flink;
		}
	}
	else
	{
		status = SHADOW_APP_UNREGISTERED;
	}

	status = WriteDataToIrpOutputBuffer(&count, sizeof(count), irp, ioStackLocation);

	return status;
}

NTSTATUS IOCTLHelper::IoctlStartFiltering(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int appId = GetAppIdFromIoctl(irp, ioStackLocation);

	if (appId != 0)
	{
		IOCTLHelper* helper = GetHelperByAppId(appId);
		if (helper != nullptr)
		{
			auto filter = helper->_context.Filter;
			if (filter != nullptr)
			{
				if (NT_SUCCESS(status))
				{
					status = filter->StartFiltering();
				}
			}
		}
		else
		{
			//Unregsitered app trying to start filtering.
			status = SHADOW_APP_UNREGISTERED;
		}
	}
	else
	{
		status = SHADOW_APP_APPID_INVALID;
	}

	return status;
}

NTSTATUS IOCTLHelper::IoctlStopFiltering(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int appId = GetAppIdFromIoctl(irp, ioStackLocation);

	if (appId != 0)
	{
		IOCTLHelper* helper = GetHelperByAppId(appId);
		if (helper != nullptr)
		{
			ShadowFilter* filter = helper->_context.Filter;
			// Cancel all pending notification IOCTLs.
			helper->CancelAllPendingNotifyIoctls();
			filter->StopFiltering();
		}
		else
		{
			status = SHADOW_APP_UNREGISTERED;
		}
	}
	else
	{
		status = SHADOW_APP_APPID_INVALID;
	}
	return status;
}

NTSTATUS IOCTLHelper::IoctlAddCondition(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int appId = GetAppIdFromIoctl(irp, ioStackLocation);

	if (appId != 0)
	{
		IOCTLHelper* selectedHelper = GetHelperByAppId(appId);

		if (selectedHelper != nullptr)
		{
			auto filter = selectedHelper->_context.Filter;
			PVOID inputBuffer = irp->AssociatedIrp.SystemBuffer;
			AppRegisterContext context{};
			const int i = sizeof(unsigned long long);
			//最长的情况是16位的IPv6 + 16位的IPv6掩码
			int dataReadSize = 6 * sizeof(int) + 16 + 16;
			auto inputBufferLength = ioStackLocation->Parameters.DeviceIoControl.InputBufferLength;
			if (inputBufferLength >= (ULONG)dataReadSize)
			{
				FilterCondition condition{};
				PCHAR inputBufferBytes = (PCHAR)inputBuffer;
				int currentIndex = StatusSize;
				inputBufferBytes += currentIndex;
				condition.FilterLayer = (NetLayer)(*(int*)inputBufferBytes);
				inputBufferBytes += sizeof(int);
				condition.MatchType = (FilterMatchType)(*(int*)inputBufferBytes);
				inputBufferBytes += sizeof(int);
				condition.AddrLocation = (AddressLocation)(*(int*)inputBufferBytes);
				inputBufferBytes += sizeof(int);
				condition.FilterPath = (NetPacketDirection)(*(int*)inputBufferBytes);
				inputBufferBytes += sizeof(int);
				condition.IPAddressType = (IpAddrFamily)(*(int*)inputBufferBytes);
				inputBufferBytes += sizeof(int);
				switch (condition.FilterLayer)
				{
				case NetLayer::NetworkLayer:
					switch (condition.IPAddressType)
					{
					case IpAddrFamily::IPv4:
						condition.IPv4Address = *((unsigned int*)(inputBufferBytes));
						inputBufferBytes += sizeof(unsigned int);
						condition.IPv4Mask = *((unsigned int*)(inputBufferBytes));
						inputBufferBytes += sizeof(unsigned int);
						break;
					case IpAddrFamily::IPv6:
						RtlCopyMemory(condition.IPv6Address, inputBufferBytes, 16);
						inputBufferBytes += 16;
						RtlCopyMemory(condition.IPv6Mask, inputBufferBytes, 16);
						inputBufferBytes += 16;
						break;
					}
					break;
				case NetLayer::LinkLayer:
					// Untested
					RtlCopyMemory(&(condition.InterfaceId), inputBufferBytes, 4);
					inputBufferBytes += 4;
					break;
				default:
					break;
				}
				filter->AddFilterConditions(&condition, 1);
			}
			else
			{
				status = STATUS_BUFFER_TOO_SMALL;
			}
		}
		else
		{
			status = SHADOW_APP_UNREGISTERED;
		}
	}
	else
	{
		status = SHADOW_APP_APPID_INVALID;
	}
	return status;
}

NTSTATUS IOCTLHelper::IoctlEnableModification(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int appId = GetAppIdFromIoctl(irp, ioStackLocation);

	if (appId != 0)
	{
		IOCTLHelper* helper = GetHelperByAppId(appId);
		if (helper != nullptr)
		{
			ShadowFilter* filter = helper->_context.Filter;
			status = filter->EnablePacketModification();
		}
		else
		{
			status = SHADOW_APP_UNREGISTERED;
		}
	}
	else
	{
		status = SHADOW_APP_APPID_INVALID;
	}
	return status;
}

NTSTATUS IOCTLHelper::IoctlInjectPacket(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int appId = GetAppIdFromIoctl(irp, ioStackLocation);

	if (appId != 0)
	{
		IOCTLHelper* helper = GetHelperByAppId(appId);
		if (helper != nullptr)
		{
			ShadowFilter* filter = helper->_context.Filter;

			UNREFERENCED_PARAMETER(filter);

			char* inputBuffer = (char*)(irp->AssociatedIrp.SystemBuffer);
			// Read inject info.
			auto inputBufferLength = ioStackLocation->Parameters.DeviceIoControl.InputBufferLength;
			if (inputBufferLength >= 20)
			{
				// Skip app id.
				char* currentPointer = inputBuffer + sizeof(int);

				NetLayer layer = (NetLayer)(*(int *)(currentPointer));
				currentPointer += sizeof(int);
				NetPacketDirection direction = (NetPacketDirection)(*(int*)(currentPointer));;
				currentPointer += sizeof(int);
				IpAddrFamily addrFamily = (IpAddrFamily)(*(int*)(currentPointer));;
				UNREFERENCED_PARAMETER(addrFamily);
				currentPointer += sizeof(int);
				int packetSize = *(int*)(currentPointer);
				currentPointer += sizeof(int);
				PNET_BUFFER_LIST netBufferList = *((PNET_BUFFER_LIST*)currentPointer);
				currentPointer += sizeof(PNET_BUFFER_LIST);
				UNREFERENCED_PARAMETER(netBufferList);
				int fragmentIndex = *(int*)(currentPointer);
				currentPointer += sizeof(int);

#ifdef DBG
				if (fragmentIndex > 0)
				{
					DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "Fragment index is larger than 0.\t\n");
				}
#endif

				if (packetSize > 0 && filter->GetModificationStatus())
				{
					char* packetStartPointer = currentPointer;
					UNREFERENCED_PARAMETER(packetStartPointer);
					if (netBufferList)
					{
						status = ShadowFilter::InjectPacket(filter->GetContext(), direction, layer, packetStartPointer, packetSize, (unsigned long long)netBufferList, fragmentIndex);
					}
					else
					{
						//status = InjectionHelper::Inject((ShadowFilterContext*)(filter->GetContext()), direction, layer, packetStartPointer, packetSize);
					}

				}
				else
				{
					// if packetSize is smaller than or equals 0, then set status.

				}



				//	UNREFERENCED_PARAMETER(currentIndex);
				//	UNREFERENCED_PARAMETER(direction);
				//	UNREFERENCED_PARAMETER(addrFamily);
				//	UNREFERENCED_PARAMETER(layer);
				//	UNREFERENCED_PARAMETER(packetSize);
				//}
			}
			else
			{
				status = SHADOW_APP_UNREGISTERED;
			}
		}
		else
		{
			status = SHADOW_APP_APPID_INVALID;
		}
	}
	return status;
}

NTSTATUS IOCTLHelper::IoctlRegisterApp(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	IOCTLHelperContext helperContext;
	helperContext.AppContext = GetAppContextFromIoctl(irp, ioStackLocation);
	auto inputBufferLength = ioStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	PVOID inputBuffer = irp->AssociatedIrp.SystemBuffer;
	
	// Make sure that app id is not zero and output buffer size is enought for status to fit in.
	if (helperContext.AppContext.AppId != 0)
	{
		//检查该应用是否已经注册过
		IOCTLHelper* checkHelper = GetHelperByAppId(helperContext.AppContext.AppId);
		if (checkHelper == nullptr)
		{
#ifdef DBG
			DbgPrintEx(DPFLTR_ENVIRON_ID, DPFLTR_INFO_LEVEL, "Registering app from file object pointer: %p\t\n", (void*)(irp->Tail.Overlay.OriginalFileObject));
			DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "Registering app id is: %d\t\n", helperContext.AppContext.AppId);
#endif
			ShadowFilterContext* sfContext = new ShadowFilterContext();
			sfContext->DeviceObject = _deviceObject;
			sfContext->NetPacketFilteringCallout = PacketHelper::FilterFunc;

			//Set up guids
			if (inputBufferLength >= sizeof(int) + 40 + 16 + 16 * 5)
			{
				auto beginIndex = sizeof(int) + 40;

				sfContext->SublayerGuid = *((PGUID)(&((PCHAR)inputBuffer)[beginIndex]));
				beginIndex += 16;
				sfContext->CalloutGuids[0] = *((PGUID)(&((PCHAR)inputBuffer)[beginIndex + 0]));
				sfContext->CalloutGuids[1] = *((PGUID)(&((PCHAR)inputBuffer)[beginIndex + 16]));
				sfContext->CalloutGuids[2] = *((PGUID)(&((PCHAR)inputBuffer)[beginIndex + 16 * 2]));
				sfContext->CalloutGuids[3] = *((PGUID)(&((PCHAR)inputBuffer)[beginIndex + 16 * 3]));
				sfContext->CalloutGuids[4] = *((PGUID)(&((PCHAR)inputBuffer)[beginIndex + 16 * 4]));
				sfContext->CalloutGuids[6] = *((PGUID)(&((PCHAR)inputBuffer)[beginIndex + 16 * 5]));
			}
			auto shadowFilter = new ShadowFilter(sfContext);
			helperContext.Filter = shadowFilter;
			helperContext.FilterContext = sfContext;

			//Use original file object as a indentifier.
			helperContext.OriginalFileObject = irp->Tail.Overlay.OriginalFileObject;

			auto ioctlHelper = new IOCTLHelper(helperContext);
			sfContext->CustomContext = ioctlHelper;
			AddHelper(ioctlHelper);
		}
		else
		{
			status = SHADOW_APP_REGISTERED;
		}
	}
	else
	{
		status = SHADOW_APP_APPID_INVALID;
	}

	return status;
}

NTSTATUS IOCTLHelper::WriteDataToIrpOutputBuffer(PVOID bufferToWrite, SIZE_T bufferSize, PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	auto outputBufferLength = ioStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	PVOID outputBuffer = irp->AssociatedIrp.SystemBuffer;
	if (outputBuffer != nullptr && bufferToWrite != nullptr && bufferSize > 0 && outputBufferLength >= (bufferSize + StatusSize))
	{
		void* systemBuffer = irp->AssociatedIrp.SystemBuffer;
		void* startPosBuffer = (void*)(((char*)systemBuffer) + StatusSize);
		RtlCopyMemory(startPosBuffer, bufferToWrite, bufferSize);
		irp->IoStatus.Information += bufferSize;
	}
	else
	{
		status = STATUS_BUFFER_TOO_SMALL;
	}

	return status;
}

void* IOCTLHelper::WriteStatusToOutputBuffer(NTSTATUS* pStatus, PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	void* result = nullptr;
	auto outputBufferLength = ioStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	PVOID outputBuffer = irp->AssociatedIrp.SystemBuffer;
	auto remainSize = outputBufferLength - irp->IoStatus.Information;
	int status = *pStatus;
	if (remainSize >= sizeof(status))
	{
		*((int*)outputBuffer) = status;
		irp->IoStatus.Information += sizeof(status);

		result = (PVOID)(((PCHAR)outputBuffer) + sizeof(status));

		// When successfuly writing ntstatus into output buffer, status needs to set to STATUS_SUCCESS so that this IOCTL can be completed.
		*pStatus = STATUS_SUCCESS;
	}
	else
	{
		*pStatus = STATUS_BUFFER_TOO_SMALL;
	}

	return result;
}
