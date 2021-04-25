#include "IOCTLHelper.h"
#include "ShadowFilterWindowsSpecific.h"
#include "CancelSafeQueueCallouts.h"
#include "PacketHelper.h"

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
	InitializeListHead(&(_helperListHeader.ListEntry));
}

NTSTATUS IOCTLHelper::NotifyUserApp(void* buffer, size_t size)
{
	NTSTATUS status = STATUS_SUCCESS;

	IOCTLHelperLinkEntry* currentEntry = &_helperListHeader;
	do
	{
		PLIST_ENTRY listEntry = currentEntry->ListEntry.Flink;
		currentEntry = CONTAINING_RECORD(listEntry, IOCTLHelperLinkEntry, ListEntry);

		if (currentEntry->Helper != nullptr)
		{
			NotifyUserByDequeuingIoctl(&currentEntry->Helper->_context, buffer, size);
		}
	} while (currentEntry != &_helperListHeader);

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
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "AddHelper_end\n");
#endif
}

void IOCTLHelper::RemoveHelper(IOCTLHelper* helper)
{
	IOCTLHelper* result = nullptr;
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

			// Delete link list entry that containts deleted helper instance.
			delete currentEntry;
			break;
		}
	} while (currentEntry != &_helperListHeader);
}

NTSTATUS IOCTLHelper::ShadowDriverIrpIoControl(_In_ _DEVICE_OBJECT* DeviceObject, _Inout_ _IRP* Irp)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ShadowDriver_IRP_IoControl\n");
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
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO\n");
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
				}
				else
				{
					status = STATUS_BUFFER_ALL_ZEROS;
					WriteStatusToOutputBuffer(&status, Irp, pIoStackIrp);
					Irp->IoStatus.Status = status;
					IoCompleteRequest(Irp, IO_NO_INCREMENT);
				}
			}
			break;
		case IOCTL_SHADOWDRIVER_GET_DRIVER_VERSION:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION\n");
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
			break;
		default:
			break;
		}
	}
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

void IOCTLHelper::NotifyUserByDequeuingIoctl(IOCTLHelperContext* context, void* outputBuffer, size_t outputLength)
{
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NotifyUserByDequeuingIoctl\n");
	PIRP dequeuedIrp = IoCsqRemoveNextIrp(&(context->IoCsq), NULL);
	if (dequeuedIrp != NULL)
	{
		PIO_STACK_LOCATION dispatchedStackIrp = IoGetCurrentIrpStackLocation(dequeuedIrp);
		auto outputBufferLength = dispatchedStackIrp->Parameters.DeviceIoControl.OutputBufferLength;
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
	while(!IsListEmpty(&(_context.IrpLinkHeadEntry.ListEntry)))
	{
		IRP_LINK_ENTRY* irpEntry = CONTAINING_RECORD(currentEntry, IRP_LINK_ENTRY, ListEntry);
		auto cancelResult = IoCancelIrp(irpEntry->Irp);
		currentEntry = currentEntry->Flink;
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
			auto filter = selectedHelper->_context.Filter;
			auto filterContext = selectedHelper->_context.FilterContext;

			// Delete helper.
			RemoveHelper(selectedHelper);

			//Delete ShadowFilter instance;
			delete selectedHelper->_context.Filter;

			//Delete ShadowFilterContext instance.
			delete selectedHelper->_context.FilterContext;
		}
		else
		{
			status = STATUS_ABANDONED;
		}
	}
	else
	{
		status = STATUS_ABANDONED;
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

	status = WriteDataToIrpOutputBuffer(&count, sizeof(count), irp, ioStackLocation);

	return status;
}

NTSTATUS IOCTLHelper::IoctlStartFiltering(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int appId = GetAppIdFromIoctl(irp, ioStackLocation);
	IOCTLHelper* helper = GetHelperByAppId(appId);
	if (helper != nullptr)
	{
		auto filter = helper->_context.Filter;
		if (filter != nullptr)
		{
			if (NT_SUCCESS(status))
			{
				//NetFilteringCondition condition = NetFilteringCondition();
				//condition.AddrLocation = AddressLocation::Remote;
				//condition.FilterLayer = NetLayer::NetworkLayer;
				//condition.FilterPath = NetPacketDirection::Out;
				//condition.IPAddressType = IpAddrFamily::IPv4;
				//condition.IPv4 = 0xC0A80168;
				//condition.IPv4Mask = 0xFFFFFFFF;
				//condition.MatchType = FilterMatchType::Equal;
				//Filter->AddFilterCondition(&condition, 1);

				//NetFilteringCondition condition = NetFilteringCondition();
				//condition.AddrLocation = AddressLocation::Local;
				//condition.FilterLayer = NetLayer::LinkLayer;
				//condition.FilterPath = NetPacketDirection::Out;
				//condition.MatchType = FilterMatchType::Equal;
				//condition.MacAddress[0] = 0x00;
				//condition.MacAddress[1] = 0x15;
				//condition.MacAddress[2] = 0x5D;
				//condition.MacAddress[3] = 0x00;
				//condition.MacAddress[4] = 0x65;
				//condition.MacAddress[5] = 0x06;
				
				//filter->AddFilterConditions(&condition, 1);
			}

			if (NT_SUCCESS(status))
			{
				filter->StartFiltering();
			}
		}
	}
	else
	{
		//Unregsitered app trying to start filtering.
		status = STATUS_BAD_DATA;
	}
	return status;
}

NTSTATUS IOCTLHelper::IoctlStopFiltering(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int appId = GetAppIdFromIoctl(irp, ioStackLocation);
	IOCTLHelper * helper = GetHelperByAppId(appId);
	ShadowFilter * filter = helper->_context.Filter;
	if (filter != nullptr)
	{
		if (NT_SUCCESS(status))
		{
			// Cancel all pending notification IOCTLs.
			helper->CancelAllPendingNotifyIoctls();

			filter->StopFiltering();
		}
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
				NetFilteringCondition condition;
				PCHAR inputBufferBytes = (PCHAR)inputBuffer;
				int currentIndex = StatusSize;
				inputBufferBytes += currentIndex;
				condition.FilterLayer = (NetLayer)(*(int*)inputBufferBytes);
				inputBufferBytes += sizeof(int);
				condition.MatchType = (FilterMatchType)(*(int*)inputBufferBytes);
				inputBufferBytes += sizeof(int);
				condition.AddrLocation = (AddressLocation)(*(int *)inputBufferBytes);
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
					RtlCopyMemory(condition.MacAddress, inputBufferBytes, 6);
					inputBufferBytes += 6;
					break;
				default:
					break;
				}
				filter->AddFilterConditions(&condition, 1);
			}
		}
		else
		{
			status = STATUS_ABANDONED;
		}
	}
	else
	{
		status = STATUS_ABANDONED;
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
			ShadowFilterContext* sfContext = new ShadowFilterContext();
			sfContext->DeviceObject = _deviceObject;
			sfContext->NetPacketFilteringCallout = FilterFunc;
			auto guidSize = sizeof(GUID);

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
			auto ioctlHelper = new IOCTLHelper(helperContext);
			AddHelper(ioctlHelper);
		}
		else
		{
			status = STATUS_ABANDONED;
		}
	}
	else
	{
		status = STATUS_ABANDONED;
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
	}
	else
	{
		*pStatus = STATUS_BUFFER_TOO_SMALL;
	}

	return result;
}
