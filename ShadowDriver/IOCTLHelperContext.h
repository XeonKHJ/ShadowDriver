#pragma once
#include "Public.h"
#include "IrpLinkEntry.h"


struct IOCTLHelperContext
{
	AppRegisterContext AppContext;
	KSPIN_LOCK SpinLock;
	IO_CSQ IoCsq;
	IRP_LINK_ENTRY IrpLinkHeadEntry;
};
