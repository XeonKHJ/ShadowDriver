#pragma once
#include "IrpLinkEntry.h"

struct IRP_LINK_ENTRY
{
	LIST_ENTRY ListEntry;
	PIRP Irp;
};

struct IOCTLHelperContext
{
	KSPIN_LOCK SpinLock;
	IO_CSQ IoCsq;
	IRP_LINK_ENTRY IrpLinkHeadEntry;
};
