#include "CancelSafeQueueCallouts.h"
#include "ShadowFilterContext.h"
#include "IOCTLHelperContext.h"

NTSTATUS
CsqInsertIrpEx(
    _In_ struct _IO_CSQ*   Csq,
    _In_ PIRP              Irp,
    _In_ PVOID             InsertContext
)
{
    UNREFERENCED_PARAMETER(InsertContext);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqInsertIrpEx\n");
    IOCTLHelperContext* context = CONTAINING_RECORD(Csq, IOCTLHelperContext, IoCsq);
    NTSTATUS status = STATUS_FAIL_CHECK;
    IRP_LINK_ENTRY * newEntry = new IRP_LINK_ENTRY();
    newEntry->Irp = Irp;
    if (newEntry)
    {
        InsertTailList(&(context->IrpLinkHeadEntry.ListEntry), &(newEntry->ListEntry));
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
    IOCTLHelperContext* context = CONTAINING_RECORD(Csq, IOCTLHelperContext, IoCsq);
    if (!IsListEmpty(&(context->IrpLinkHeadEntry.ListEntry)))
    {
        IRP_LINK_ENTRY * matchedIRPLiskEntry = NULL;
        IRP_LINK_ENTRY * currentIrpListEntry = CONTAINING_RECORD(context->IrpLinkHeadEntry.ListEntry.Flink, IRP_LINK_ENTRY, ListEntry);
        while ((matchedIRPLiskEntry == NULL) && (currentIrpListEntry != &context->IrpLinkHeadEntry))
        {
            if (currentIrpListEntry->Irp == Irp)
            {
                matchedIRPLiskEntry = currentIrpListEntry;
            }
            currentIrpListEntry = CONTAINING_RECORD(currentIrpListEntry->ListEntry.Flink, IRP_LINK_ENTRY, ListEntry);
        }
        if (matchedIRPLiskEntry != NULL && matchedIRPLiskEntry != &context->IrpLinkHeadEntry)
        {
            RemoveEntryList(&(matchedIRPLiskEntry->ListEntry));
            delete matchedIRPLiskEntry;
        }
    }
}

PIRP
CsqPeekNextIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP    Irp,
    _In_ PVOID   PeekContext
)
{
    UNREFERENCED_PARAMETER(PeekContext);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqPeekNextIrp\n");
    IOCTLHelperContext* context = CONTAINING_RECORD(Csq, IOCTLHelperContext, IoCsq);
    PLIST_ENTRY selectedEntry = NULL;
    IRP_LINK_ENTRY * selectedIrpEntry = NULL;
    PIRP irp = NULL;
    if (!IsListEmpty(&(context->IrpLinkHeadEntry.ListEntry)))
    {
        if (Irp)
        {
            IRP_LINK_ENTRY * matchedIRPLiskEntry = NULL;
            IRP_LINK_ENTRY * currentIrpListEntry = CONTAINING_RECORD(context->IrpLinkHeadEntry.ListEntry.Flink, IRP_LINK_ENTRY, ListEntry);
            while ((matchedIRPLiskEntry == NULL) && (currentIrpListEntry != &(context->IrpLinkHeadEntry)))
            {

                if (currentIrpListEntry->Irp == Irp)
                {
                    matchedIRPLiskEntry = currentIrpListEntry;
                }
                currentIrpListEntry = CONTAINING_RECORD(currentIrpListEntry->ListEntry.Flink, IRP_LINK_ENTRY, ListEntry);
            }

            if (matchedIRPLiskEntry)
            {
                if (matchedIRPLiskEntry != &(context->IrpLinkHeadEntry))
                {
                    selectedEntry = matchedIRPLiskEntry->ListEntry.Flink;
                    selectedIrpEntry = CONTAINING_RECORD(selectedEntry, IRP_LINK_ENTRY, ListEntry);
                    if (selectedIrpEntry != NULL)
                    {
                        irp = selectedIrpEntry->Irp;
                    }
                }
            }
        }
        else
        {
            selectedEntry = context->IrpLinkHeadEntry.ListEntry.Flink;
            selectedIrpEntry = CONTAINING_RECORD(selectedEntry, IRP_LINK_ENTRY, ListEntry);
            if (selectedIrpEntry != NULL)
            {
                irp = selectedIrpEntry->Irp;
            }
        }
    }
    else
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IOCTL Queue is empty!\n");
    }
    return irp;
}

VOID CsqAcquireLock(PIO_CSQ IoCsq, PKIRQL PIrql)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqAcquireLock\n");
    IOCTLHelperContext* context = CONTAINING_RECORD(IoCsq, IOCTLHelperContext, IoCsq);
    KeAcquireSpinLock(&(context->SpinLock), PIrql);
}

VOID CsqReleaseLock(PIO_CSQ IoCsq, KIRQL Irql)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqReleaseLock\n");
    IOCTLHelperContext* context = CONTAINING_RECORD(IoCsq, IOCTLHelperContext, IoCsq);
    KeReleaseSpinLock(&(context->SpinLock), Irql);
}

VOID
CsqCompleteCanceledIrp(
    _In_ PIO_CSQ    Csq,
    _In_ PIRP       Irp
)
{
    UNREFERENCED_PARAMETER(Csq);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CsqCompleteCanceledIrp\n");
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}