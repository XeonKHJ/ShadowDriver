#include "IrpFunctions.h"

NTSTATUS
ShadowDriver_IRP_IoControl(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ShadowDriver_IRP_IoControl\n");
    NTSTATUS status;
    //È¡³öIRP
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
            break;
        default:
            break;
        }
    }

    return 0;
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