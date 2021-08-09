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

DEFINE_GUID (GUID_DEVINTERFACE_PureShadowDriver,
    0x69c800f2,0x9f54,0x4fe8,0xb3,0xbd,0xee,0x5b,0x54,0x20,0xdf,0xa0);
// {69c800f2-9f54-4fe8-b3bd-ee5b5420dfa0}
