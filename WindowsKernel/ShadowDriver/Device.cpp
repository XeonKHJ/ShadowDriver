/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"
#include "ShadowFilter.h"
#include "IOCTLHelper.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, ShadowDriverCreateDevice)
#endif
#include "ShadowFilterContext.h"

typedef struct _INVERTED_DEVICE_CONTEXT {
    WDFQUEUE    NotificationQueue;
    LONG       Sequence;
} INVERTED_DEVICE_CONTEXT, * PINVERTED_DEVICE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INVERTED_DEVICE_CONTEXT, InvertedGetContextFromDevice)

NTSTATUS
ShadowDriverCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);


    UNICODE_STRING deviceName;
    UNICODE_STRING deviceDosName;
    //RtlInitUnicodeString(&deviceName, L"\\Device\\ShadowDriver");
    RtlInitUnicodeString(&deviceDosName, L"\\DosDevices\\ShadowDriver");

    //status = WdfDeviceInitAssignName(DeviceInit, &deviceName);
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_NETWORK);
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfDeviceCreateSymbolicLink(device, &deviceDosName);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (NT_SUCCESS(status)) {
        //
        // Get a pointer to the device context structure that we just associated
        // with the device object. We define this structure in the device.h
        // header file. DeviceGetContext is an inline function generated by
        // using the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in device.h.
        // This function will do the type checking and return the device context.
        // If you pass a wrong object handle it will return NULL and assert if
        // run under framework verifier mode.
        //
        deviceContext = DeviceGetContext(device);

        //
        // Initialize the context.
        //
        deviceContext->PrivateDeviceData = 0;
        
        //
        // Create a device interface so that applications can find and talk
        // to us.
        //
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_ShadowDriver,
            NULL // ReferenceString
            );

        if (NT_SUCCESS(status)) {
            //
            // Initialize the I/O Package and any Queues
            //
            status = ShadowDriverQueueInitialize(device);
        }

#ifdef DBG
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ShadowDriverCreateDevice\n");
#endif
        PDEVICE_OBJECT deviceObject = WdfDeviceWdmGetDeviceObject(device);

        //初始化Windows筛选平台
        IOCTLHelper::SetDeviceObject(deviceObject);
        //status = InitializeWfp(deviceObject);
    }
    
    return status;
}
