/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/
#pragma once
//
// Include addtional status.
//
#include "ShadowDriverStatus.h"

//
// Define an Interface Guid so that apps can find the device and talk to it.
//
DEFINE_GUID (GUID_DEVINTERFACE_ShadowDriver,
    0x45f22bb7,0x6bc3,0x4545,0x96,0xed,0x73,0xde,0x89,0xc4,0x6e,0x7d);
// {45f22bb7-6bc3-4545-96ed-73de89c46e7d}

//
// Define IOCTLs allowing user-mode application to communicate.
//
#define IOCTL_SHADOWDRIVER_APP_REGISTER					CTL_CODE(FILE_DEVICE_NETWORK, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_APP_DEREGISTER               CTL_CODE(FILE_DEVICE_NETWORK, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_GET_LOCAL_STATUS             CTL_CODE(FILE_DEVICE_NETWORK, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_START_FILTERING              CTL_CODE(FILE_DEVICE_NETWORK, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_STOP_FILTERING               CTL_CODE(FILE_DEVICE_NETWORK, 0x912, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_REQUIRE_PACKET_INFO          CTL_CODE(FILE_DEVICE_NETWORK, 0x910, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_DEQUEUE_NOTIFICATION         CTL_CODE(FILE_DEVICE_NETWORK, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_REQUIRE_VERSION_INFO         CTL_CODE(FILE_DEVICE_NETWORK, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_QUEUE_NOTIFICATION           CTL_CODE(FILE_DEVICE_NETWORK, 0x921, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_GET_QUEUE_INFO               CTL_CODE(FILE_DEVICE_NETWORK, 0x923, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_GET_DRIVER_VERSION           CTL_CODE(FILE_DEVICE_NETWORK, 0x922, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SHADOWDRIVER_ADD_CONDITION                CTL_CODE(FILE_DEVICE_NETWORK, 0x924, METHOD_BUFFERED, FILE_ANY_ACCESS)

const static int StatusSize = sizeof(int);
const static int AppRegisterContextMaxSize = StatusSize + sizeof(WCHAR) * 20;
struct AppRegisterContext
{
    int AppId;
    WCHAR AppName[20];
};