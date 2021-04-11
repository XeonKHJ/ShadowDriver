#include "IOCTLHelper.h"
#include "ShadowFilterWindowsSpecific.h"
#include "CancelSafeQueueCallouts.h"

int IOCTLHelper::_helperCount = 0;
IOCTLHelperLinkEntry IOCTLHelper::_helperListHeader;
IOCTLHelper::IOCTLHelper(IOCTLHelperContext context)
{
	_context = context;
	InitializeIRPNotificationSystem();
}

void IOCTLHelper::InitializeDriverObjectForIOCTL(_In_ PDRIVER_OBJECT driverObject)
{
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ShadowDriverIrpIoControl;
	InitializeListHead(&(_helperListHeader.ListEntry));
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
		case IOCTL_SHADOWDRIVER_START_WFP:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_START_WFP\n");
			//IoctlStartWpf(Irp);
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
				NotifyUserByDequeuingIoctl(&helper->_context);
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

void IOCTLHelper::NotifyUserByDequeuingIoctl(IOCTLHelperContext * context)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NotifyUserByDequeuingIoctl\n");
	PIRP dequeuedIrp = IoCsqRemoveNextIrp(&(context->IoCsq), NULL);
	if (dequeuedIrp != NULL)
	{
		PIO_STACK_LOCATION dispatchedStackIrp = IoGetCurrentIrpStackLocation(dequeuedIrp);
		if (dispatchedStackIrp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Dequeue a irp\n");
		}

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
