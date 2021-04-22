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
// Define an Interface Guid so that apps can find the device and talk to it.
//
DEFINE_GUID (GUID_DEVINTERFACE_ShadowDriver,
    0x45f22bb7,0x6bc3,0x4545,0x96,0xed,0x73,0xde,0x89,0xc4,0x6e,0x7d);
// {45f22bb7-6bc3-4545-96ed-73de89c46e7d}

//static const GUID calloutsGuid = { 0xc21d8417, 0xf64e, 0x4669, { 0xa0, 0x95, 0x1d, 0xeb, 0x48, 0xad, 0xd1, 0x17 } };
DEFINE_GUID(SHADOWDRIVER_WFP_SEND_ESTABLISHED_CALLOUT_GUID, 0xc21d8417, 0xf64e, 0x4669, 0xa0, 0x95, 0x1d, 0xeb, 0x48, 0xad, 0xd1, 0x17);
DEFINE_GUID(SHADOWDRIVER_WFP_RECEIVE_ESTABLISHED_CALLOUT_GUID, 0x2f2f004c, 0xf40d, 0x46f5, 0x91, 0xa6, 0x80, 0xb7, 0x65, 0xc7, 0x84, 0xe0);
DEFINE_GUID(SHADOWDRIVER_WFP_SUBLAYER_GUID, 0x86e8c72a, 0xebc, 0x4b60, 0xa4, 0xf6, 0x3d, 0xc4, 0x1b, 0x52, 0xae, 0x3e);
//DEFINE_GUID(SHADOWDRIVER_WFP_SUBLAYER_GUID, 0x86e8c72a, 0xebc, 0x4b60, 0xa4, 0xf6, 0x3d, 0xc4, 0x1b, 0x52, 0xae, 0x3e);
DEFINE_GUID(SHADOWDRIVER_WFP_NETWORK_IPV4_SEND_ESTABLISHED_CALLOUT_GUID, 0xc21d8417, 0xf64e, 0x4669, 0xa0, 0x95, 0x1d, 0xeb, 0x48, 0xad, 0xd1, 0x17);
DEFINE_GUID(SHADOWDRIVER_WFP_NETWORK_IPV4_RECEIVE_ESTABLISHED_CALLOUT_GUID, 0x2f2f004c, 0xf40d, 0x46f5, 0x91, 0xa6, 0x80, 0xb7, 0x65, 0xc7, 0x84, 0xe0);
// {CD460F44-D0AF-4DD5-A26A-FAFF374282CA}
DEFINE_GUID(SHADOWDRIVER_WFP_NETWORK_IPV6_SEND_ESTABLISHED_CALLOUT_GUID, 0xcd460f44, 0xd0af, 0x4dd5, 0xa2, 0x6a, 0xfa, 0xff, 0x37, 0x42, 0x82, 0xca);
//// {5D72CC56-17CA-48F5-AA5D-FF955B5D8D9C}
DEFINE_GUID(SHADOWDRIVER_WFP_NETWORK_IPV6_RECEIVE_ESTABLISHED_CALLOUT_GUID, 0x5d72cc56, 0x17ca, 0x48f5, 0xaa, 0x5d, 0xff, 0x95, 0x5b, 0x5d, 0x8d, 0x9c);
//DEFINE_GUID(SHADOWDRIVER_WFP_LINK_SEND_ESTABLISHED_CALLOUT_GUID, 0x2e87f045, 0xb901, 0x4157, ,0x8b, 0xa1, 0x9b, 0xb3, 0x21, 0x9c, 0x99, 0xaa);
DEFINE_GUID(SHADOWDRIVER_WFP_LINK_RECEIVE_ESTABLISHED_CALLOUT_GUID, 0x268fc924, 0xf079, 0x40f6, 0x8e, 0x30, 0x7, 0xa8, 0xde, 0x4b, 0xe0, 0x14);

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


struct AppRegisterContext
{
    int AppId;
    WCHAR AppName[50];
};