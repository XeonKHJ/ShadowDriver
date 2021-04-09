#pragma once
#include <wdm.h>

struct IRP_LINK_ENTRY
{
    LIST_ENTRY ListEntry;
    PIRP Irp;
};