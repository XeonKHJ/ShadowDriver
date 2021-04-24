#include "IOCTLHelper.h"
#include "ShadowFilterWindowsSpecific.h"
#include "CancelSafeQueueCallouts.h"

int IOCTLHelper::_helperCount = 0;
IOCTLHelperLinkEntry IOCTLHelper::_helperListHeader;
ShadowFilter* IOCTLHelper::Filter;
IOCTLHelper::IOCTLHelper(IOCTLHelperContext context)
{
	_context = context;
	InitializeIRPNotificationSystem();
}

IOCTLHelper::~IOCTLHelper()
{
	
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

		if (currentEntry->Helper == helper)
		{
			RemoveEntryList(listEntry);
			delete helper;
			delete currentEntry;
			break;
		}
	} while (currentEntry != &_helperListHeader);
}

NTSTATUS IOCTLHelper::ShadowDriverIrpIoControl(_In_ _DEVICE_OBJECT* DeviceObject, _Inout_ _IRP* Irp)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ShadowDriver_IRP_IoControl\n");
	NTSTATUS status = STATUS_SUCCESS;
	unsigned int dwDataWritten = 0;
	//取出IRP
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (pIoStackIrp)
	{
		ULONG ioControlCode = pIoStackIrp->Parameters.DeviceIoControl.IoControlCode;
		switch (ioControlCode)
		{
		case IOCTL_SHADOWDRIVER_APP_REGISTER:
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_START_WFP\n");
#endif
			Irp->IoStatus.Status = RegisterAppForIOCTLCalls(Irp, pIoStackIrp);
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		case IOCTL_SHADOWDRIVER_APP_DEREGISTER:
			Irp->IoStatus.Status = DeregisterAppForIOCTLCalls(Irp, pIoStackIrp);
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		case IOCTL_SHADOWDRIVER_START_FILTERING:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_START_WFP\n");
			IoctlStartFiltering(Irp, pIoStackIrp);
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
				AppRegisterContext appContext = GetAppContextFromIoctl(Irp, pIoStackIrp);
				Irp->IoStatus.Status = status;
				Irp->IoStatus.Information = dwDataWritten;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				IOCTLHelper* helper = GetHelperByAppId(appContext.AppId);
				NotifyUserByDequeuingIoctl(&helper->_context, NULL, 0);
			}
			break;
		case IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION:
#ifdef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION\n");
#endif
			{
				AppRegisterContext appContext = GetAppContextFromIoctl(Irp, pIoStackIrp);
				IOCTLHelper* helper = GetHelperByAppId(appContext.AppId);
				if (helper != nullptr)
				{
					status = IoCsqInsertIrpEx(&(helper->_context.IoCsq), Irp, NULL, NULL);
				}
				else
				{
					Irp->IoStatus.Status = STATUS_BUFFER_ALL_ZEROS;
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
			Irp->IoStatus.Status = GetQueuedIoctlCount(Irp, pIoStackIrp);
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
			break;
		case IOCTL_SHADOWDRIVER_ADD_CONDITION:
		{
			Irp->IoStatus.Status = IoctlAddCondition(Irp, pIoStackIrp);
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
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
	int dataReadSize = sizeof(AppRegisterContext);
	auto inputBufferLength = ioStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	if (inputBufferLength <= (ULONG)dataReadSize)
	{
		context.AppId = *((int*)(inputBuffer));
		auto nameLength = inputBufferLength - sizeof(int);
		void* nameBuffer = (void*)((char*)inputBuffer + sizeof(int));
		RtlCopyMemory(context.AppName, nameBuffer, nameLength);
	}
	return context;
}

IOCTLHelper* IOCTLHelper::GetHelperByAppId(int id)
{
	IOCTLHelper* result = nullptr;
	IOCTLHelperLinkEntry * currentEntry = &_helperListHeader;
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

void IOCTLHelper::NotifyUserByDequeuingIoctl(IOCTLHelperContext * context, void * outputBuffer, size_t outputLength)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NotifyUserByDequeuingIoctl\n");
	PIRP dequeuedIrp = IoCsqRemoveNextIrp(&(context->IoCsq), NULL);
	if (dequeuedIrp != NULL)
	{
		PIO_STACK_LOCATION dispatchedStackIrp = IoGetCurrentIrpStackLocation(dequeuedIrp);
		auto outputBufferLength = dispatchedStackIrp->Parameters.DeviceIoControl.OutputBufferLength;
		if (dispatchedStackIrp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Dequeue a irp\n");
		}
		if (outputBuffer != nullptr && outputLength > 0 && outputBufferLength >= outputLength)
		{
			void* systemBuffer = dequeuedIrp->AssociatedIrp.SystemBuffer;
			RtlCopyMemory(systemBuffer, outputBuffer, outputLength);
			dequeuedIrp->IoStatus.Information = outputLength;
		}
		
		dequeuedIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(dequeuedIrp, IO_NO_INCREMENT);
	}
}

NTSTATUS IOCTLHelper::InitializeIRPNotificationSystem()
{
	NTSTATUS status = STATUS_SUCCESS;
	//初始化自旋锁，用来给IRP队列上锁
	KeInitializeSpinLock(&(_context.SpinLock));
	//初始化IRP取消安全队列数据结构
	status = IoCsqInitializeEx(&(_context.IoCsq), CsqInsertIrpEx, CsqRemoveIrp, CsqPeekNextIrp, CsqAcquireLock, CsqReleaseLock, CsqCompleteCanceledIrp);
	if (NT_SUCCESS(status))
	{
		InitializeListHead(&(_context.IrpLinkHeadEntry.ListEntry));
	}

	return status;
}

NTSTATUS IOCTLHelper::DeregisterAppForIOCTLCalls(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	IOCTLHelperContext helperContext;
	helperContext.AppContext = GetAppContextFromIoctl(irp, ioStackLocation);

	if (helperContext.AppContext.AppId != 0)
	{
		IOCTLHelper* selectedHelper = GetHelperByAppId(helperContext.AppContext.AppId);
		if (selectedHelper != nullptr)
		{
			RemoveHelper(selectedHelper);
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

NTSTATUS IOCTLHelper::GetQueuedIoctlCount(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	int count = 0;;
	AppRegisterContext context = GetAppContextFromIoctl(irp, ioStackLocation);
	IOCTLHelper * helper = GetHelperByAppId(context.AppId);
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
	
	PVOID outputBuffer = irp->AssociatedIrp.SystemBuffer;
	auto outputBufferLength = ioStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	if (outputBufferLength >= sizeof(int))
	{
		RtlCopyMemory(outputBuffer, &count, sizeof(int));
		irp->IoStatus.Information = sizeof(int);
	}
	return status;
}

NTSTATUS IOCTLHelper::IoctlStartFiltering(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	if (Filter != nullptr)
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

			NetFilteringCondition condition = NetFilteringCondition();
			condition.AddrLocation = AddressLocation::Local;
			condition.FilterLayer = NetLayer::LinkLayer;
			condition.FilterPath = NetPacketDirection::Out;
			//condition.IPAddressType = IpAddrFamily::;
			//condition.IPv4 = 0xC0A80168;
			//condition.IPv4Mask = 0xFFFFFFFF;
			condition.IPv6Addr;
			condition.MatchType = FilterMatchType::Equal;
			condition.MacAddress[0] = 0x00;
			condition.MacAddress[1] = 0x15;
			condition.MacAddress[2] = 0x5D;
			condition.MacAddress[3] = 0x00;
			condition.MacAddress[4] = 0x65;
			condition.MacAddress[5] = 0x06;
			Filter->AddFilterConditions(&condition, 1);
		}

		if (NT_SUCCESS(status))
		{
			Filter->StartFiltering();
		}
	}
	return status;
}

NTSTATUS IOCTLHelper::IoctlAddCondition(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	IOCTLHelperContext helperContext;
	helperContext.AppContext = GetAppContextFromIoctl(irp, ioStackLocation);

	if (helperContext.AppContext.AppId != 0)
	{
		IOCTLHelper* selectedHelper = GetHelperByAppId(helperContext.AppContext.AppId);
		if (selectedHelper != nullptr)
		{
			PVOID inputBuffer = irp->AssociatedIrp.SystemBuffer;
			AppRegisterContext context{};
			int dataReadSize = sizeof(AppRegisterContext);
			auto inputBufferLength = ioStackLocation->Parameters.DeviceIoControl.InputBufferLength;
			if (inputBufferLength <= (ULONG)dataReadSize)
			{
				int beginIndex = sizeof(int) + 50;
				NetFilteringCondition condition;
				condition.FilterLayer = (NetLayer)(*((PCHAR)inputBuffer + beginIndex));
				condition.MatchType = (FilterMatchType)(*((PCHAR)inputBuffer + beginIndex + sizeof(int)));
				condition.AddrLocation = (AddressLocation)(*((PCHAR)inputBuffer + beginIndex + 2*sizeof(int)));
				switch (condition.FilterLayer)
				{
				case NetLayer::NetworkLayer:
					condition.IPAddressType = (IpAddrFamily)(*((PCHAR)inputBuffer + beginIndex + 3 * sizeof(int)));
					switch (condition.IPAddressType)
					{
					case IpAddrFamily::IPv4:
						// Unimplemented
						break;
					case IpAddrFamily::IPv6:
						// Unimplemented
						break;
					}
					break;
				case NetLayer::LinkLayer:
					// Untested
					for (int i = 0; i < 6; ++i)
					{
						int currentIndex = beginIndex + 2 * sizeof(int) + i;
						condition.MacAddress[i] = ((PCHAR)inputBuffer)[currentIndex];
					}
					break;
				default:
					break;
				}
				Filter->AddFilterConditions(&condition, 1);
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

NTSTATUS IOCTLHelper::RegisterAppForIOCTLCalls(PIRP irp, PIO_STACK_LOCATION ioStackLocation)
{
	NTSTATUS status = STATUS_SUCCESS;
	IOCTLHelperContext helperContext;
	helperContext.AppContext = GetAppContextFromIoctl(irp, ioStackLocation);
	
	if (helperContext.AppContext.AppId != 0)
	{
		IOCTLHelper * checkHelper = GetHelperByAppId(helperContext.AppContext.AppId);
		if (checkHelper == nullptr)
		{
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
