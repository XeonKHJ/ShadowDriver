#include "IOCTLHelper.h"
#include "ShadowCommon.h"
#include "ShadowFilterWindowsSpecific.h"
#include "CancelSafeQueueCallouts.h"

IOCTLHelper::IOCTLHelper(_In_ PDRIVER_OBJECT driverObject)
{
	_context;
	//��ʼ������
	InitializeIrpLinkEntry();
}

IRP_LINK_ENTRY* IOCTLHelper::InitializeIrpLinkEntry()
{
	IRP_LINK_ENTRY* newEntry = new IRP_LINK_ENTRY();
	return newEntry;
}


void IOCTLHelper::InitializeDriverObjectForIOCTL(_In_ PDRIVER_OBJECT driverObject)
{
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ShadowDriverIrpIoControl;
}

NTSTATUS IOCTLHelper::ShadowDriverIrpIoControl(_In_ _DEVICE_OBJECT* DeviceObject, _Inout_ _IRP* Irp)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ShadowDriver_IRP_IoControl\n");
	NTSTATUS status = STATUS_SUCCESS;
	unsigned int dwDataWritten = 0;
	//ȡ��IRP
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (pIoStackIrp)
	{
		ULONG ioControlCode = pIoStackIrp->Parameters.DeviceIoControl.IoControlCode;
		switch (ioControlCode)
		{
		case IOCTL_SHADOWDRIVER_START_WFP:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_START_WFP\n");
			//IoctlStartWpf(Irp);
			break;
		case IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO\n");
			//IoctlRequirePacketInfo(Irp);
			break;
		case IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO_SHIT:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO_WTF\n");
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = dwDataWritten;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			//TestDequeueIOCTL();
			break;
		case IOCTL_SHADOWDRIVER_INVERT_NOTIFICATION:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_INVERT_NOTIFICATION\n");
			//status = IoCsqInsertIrpEx(&_csq, Irp, NULL, NULL);
			break;
		case IOCTL_SHADOWDRIVER_GET_DRIVER_VERSION:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_INVERT_NOTIFICATION\n");
			//IoctlGetDriverVersion(Irp, pIoStackIrp);
			break;
		default:
			break;
		}
	}
	return status;
}

NTSTATUS IOCTLHelper::InitializeIRPNotificationSystem()
{
	NTSTATUS status = STATUS_SUCCESS;
	//��ʼ����������������IRP��������
	KeInitializeSpinLock(&(_context.SpinLock));
	//��ʼ��IRPȡ����ȫ�������ݽṹ
	status = IoCsqInitializeEx(&(_context.IoCsq), CsqInsertIrpEx, CsqRemoveIrp, CsqPeekNextIrp, CsqAcquireLock, CsqReleaseLock, CsqCompleteCanceledIrp);
	if (NT_SUCCESS(status))
	{
		InitializeListHead(&(_context.IrpLinkHeadEntry.ListEntry));
	}

	return status;
}
