#pragma once

#include <fwpsk.h>
#include <fwpmk.h>

#include "ShadowUtilities.h"

// {C21D8417-F64E-4669-A095-1DEB48ADD117}
static const GUID calloutsGuid =
{ 0xc21d8417, 0xf64e, 0x4669, { 0xa0, 0x95, 0x1d, 0xeb, 0x48, 0xad, 0xd1, 0x17 } };

DEFINE_GUID(WFP_SEND_ESTABLISHED_CALLOUT_GUID, 0xc21d8417, 0xf64e, 0x4669, 0xa0, 0x95, 0x1d, 0xeb, 0x48, 0xad, 0xd1, 0x17);

DEFINE_GUID(WFP_RECEIVE_ESTABLISHED_CALLOUT_GUID, 0x2f2f004c, 0xf40d, 0x46f5, 0x91, 0xa6, 0x80, 0xb7, 0x65, 0xc7, 0x84, 0xe0);

NTSTATUS RegisterCalloutFuntions(IN PDEVICE_OBJECT deviceObject);

NTSTATUS AddCalloutToWfp(IN HANDLE engineHandler);

VOID UnRegisterCallout(HANDLE engineHandle);

NTSTATUS CreateInjector();
