//This file declare the functions that being used to simulate notify function.
#pragma once
#include <wdm.h>

IRP_LINK_ENTRY * InitializeIrpLinkEntry();

NTSTATUS
CsqInsertIrpEx(
    _In_ struct _IO_CSQ* Csq,
    _In_ PIRP              Irp,
    _In_ PVOID             InsertContext
);

VOID
CsqRemoveIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP    Irp
);

PIRP
CsqPeekNextIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP    Irp,
    _In_ PVOID   PeekContext
);

VOID CsqAcquireLock(PIO_CSQ IoCsq, PKIRQL PIrql);

VOID CsqReleaseLock(PIO_CSQ IoCsq, KIRQL Irql);

VOID
CsqCompleteCanceledIrp(
    _In_ PIO_CSQ    Csq,
    _In_ PIRP       Irp
);

