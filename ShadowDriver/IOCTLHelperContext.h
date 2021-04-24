#pragma once
#include "Public.h"
#include "IrpLinkEntry.h"
#include "ShadowFilter.h"

struct IOCTLHelperContext
{
	AppRegisterContext AppContext;
	ShadowFilter * Filter;
	KSPIN_LOCK SpinLock;
	IO_CSQ IoCsq;
	IRP_LINK_ENTRY IrpLinkHeadEntry;
};
