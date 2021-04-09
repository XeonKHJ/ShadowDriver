#pragma once
#include "IrpLinkEntry.h"


struct IOCTLHelperContext
{
	KSPIN_LOCK SpinLock;
	IO_CSQ IoCsq;
	IRP_LINK_ENTRY IrpLinkHeadEntry;
};
