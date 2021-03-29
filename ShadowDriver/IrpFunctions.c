#include "IrpFunctions.h"

typedef struct
{
    LIST_ENTRY ListEntry;
    PIRP Irp;
} IRP_LINK_ENTRY, * PIRP_LINK_ENTRY;

KSPIN_LOCK _spinLock;
IO_CSQ _csq;
IRP_LINK_ENTRY _irpLinkEntry;

PIRP_LINK_ENTRY InitializeIrpLinkEntry()
{
    PIRP_LINK_ENTRY newEntry = (PIRP_LINK_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(IRP_LINK_ENTRY), 'ile');
    if (newEntry)
    {
        memset(newEntry, 0, sizeof(IRP_LINK_ENTRY));
    }
    return newEntry;
}

void DeleteIrpLinkEntry(PIRP_LINK_ENTRY linkEntry)
{
    ExFreePoolWithTag((PVOID)linkEntry, 'ile');
}


NTSTATUS
CsqInsertIrpEx(
    _In_ struct _IO_CSQ* Csq,
    _In_ PIRP              Irp,
    _In_ PVOID             InsertContext
)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqInsertIrpEx\n");
    NTSTATUS status = STATUS_FAIL_CHECK;
    PIRP_LINK_ENTRY newEntry = InitializeIrpLinkEntry();
    newEntry->Irp = Irp;
    if (newEntry)
    {
        InsertTailList(&(_irpLinkEntry.ListEntry), &(newEntry->ListEntry));
        status = STATUS_SUCCESS;
    }
    return status;
}

VOID
CsqRemoveIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP    Irp
)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqRemoveIrp\n");
    if (!IsListEmpty(&(_irpLinkEntry.ListEntry)))
    {
        PLIST_ENTRY tailEntry = RemoveTailList(&(_irpLinkEntry.ListEntry));
    }
}

PIRP
CsqPeekNextIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP    Irp,
    _In_ PVOID   PeekContext
)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqPeekNextIrp\n");
    PLIST_ENTRY finalEntry = NULL;
    PIRP_LINK_ENTRY tailIrpEntry = NULL;
    if (!IsListEmpty(&(_irpLinkEntry.ListEntry)))
    {
        finalEntry = _irpLinkEntry.ListEntry.Blink;
        tailIrpEntry = CONTAINING_RECORD(&_irpLinkEntry.ListEntry, IRP_LINK_ENTRY, ListEntry);
    }
    else
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL Queue is empty!\n");
    }
    return tailIrpEntry->Irp;
}

VOID CsqAcquireLock(PIO_CSQ IoCsq, PKIRQL PIrql)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqAcquireLock\n");
    KeAcquireSpinLock(&_spinLock, PIrql);
}

VOID CsqReleaseLock(PIO_CSQ IoCsq, KIRQL Irql)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqReleaseLock\n");
    KeReleaseSpinLock(&_spinLock, Irql);
}

VOID
CsqCompleteCanceledIrp(
    _In_ PIO_CSQ    Csq,
    _In_ PIRP       Irp
)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqCompleteCanceledIrp\n");
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS InitializeIRPHandlings()
{
    NTSTATUS status;
    
    //初始化自旋锁，用来给IRP队列上锁
    KeInitializeSpinLock(&_spinLock);

    //初始化IRP取消安全队列数据结构
    status = IoCsqInitializeEx(&_csq, CsqInsertIrpEx, CsqRemoveIrp, CsqPeekNextIrp, CsqAcquireLock, CsqReleaseLock, CsqCompleteCanceledIrp);
    InitializeListHead(&(_irpLinkEntry.ListEntry));
    return status;
}



NTSTATUS
ShadowDriver_IRP_IoControl(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ShadowDriver_IRP_IoControl\n");
    NTSTATUS status = STATUS_SUCCESS;
    UINT dwDataWritten = 0;
    //取出IRP
    PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
    if (pIoStackIrp)
    {
        ULONG ioControlCode = pIoStackIrp->Parameters.DeviceIoControl.IoControlCode;
        switch (ioControlCode)
        {
        case IOCTL_SHADOWDRIVER_START_WFP:
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_START_WFP\n");
            break;
        case IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO:
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO\n");
            break;
        case IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO_SHIT:
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO_WTF\n");
            PIRP dequeuedIrp = IoCsqRemoveNextIrp(&_csq, NULL);
            if (dequeuedIrp != NULL)
            {
                if (IoGetCurrentIrpStackLocation(dequeuedIrp)->Parameters.DeviceIoControl.IoControlCode == IOCTL_SHADOWDRIVER_INVERT_NOTIFICATION)
                {
                    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Dequeue a irp\n");
                }
            }
            break;
        case IOCTL_SHADOWDRIVER_INVERT_NOTIFICATION:
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL_SHADOWDRIVER_INVERT_NOTIFICATION\n");
            status = IoCsqInsertIrpEx(&_csq, Irp, NULL, NULL);
            break;
        default:
            break;
        }
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = dwDataWritten;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
ShadowDriver_IRP_Create(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ShadowDriver_IRP_Create\n");
    NTSTATUS status = STATUS_SUCCESS;
    return status;
}

/// <summary>
/// 用于控制IOCTL队列
/// </summary>
/// <param name="Queue"></param>
/// <param name="Request"></param>
/// <param name="OutputBufferLength"></param>
/// <param name="InputBufferLength"></param>
/// <param name="IoControlCode"></param>
VOID
InvertedEvtIoDeviceControl(
    _In_
    WDFQUEUE Queue,
    _In_
    WDFREQUEST Request,
    _In_
    size_t OutputBufferLength,
    _In_
    size_t InputBufferLength,
    _In_
    ULONG IoControlCode
)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ShadowDriver_InvertedEvtIoDeviceControl\n");
}