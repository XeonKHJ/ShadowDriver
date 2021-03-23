#pragma once
#include <fwpsk.h>
#include <fwpmk.h>

NTSTATUS
ShadowDriver_IRP_IoControl(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
);

NTSTATUS
ShadowDriver_IRP_Create(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
);