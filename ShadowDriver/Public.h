/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_ShadowDriver,
    0x45f22bb7,0x6bc3,0x4545,0x96,0xed,0x73,0xde,0x89,0xc4,0x6e,0x7d);
// {45f22bb7-6bc3-4545-96ed-73de89c46e7d}
