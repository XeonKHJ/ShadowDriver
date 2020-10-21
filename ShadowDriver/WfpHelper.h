#pragma once
#include "fwpsk.h"
#include "Callouts.h"
// {86E8C72A-0EBC-4B60-A4F6-3DC41B52AE3E}

DEFINE_GUID(WFP_SUBLAYER_GUID, 0x86e8c72a, 0xebc, 0x4b60, 0xa4, 0xf6, 0x3d, 0xc4, 0x1b, 0x52, 0xae, 0x3e);

NTSTATUS InitializeWfp(PDEVICE_OBJECT deviceObject);

UINT64 filterId;

UINT64 filterId2;