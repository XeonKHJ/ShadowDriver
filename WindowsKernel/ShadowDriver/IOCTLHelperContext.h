#pragma once
#include "Public.h"
#include "IrpLinkEntry.h"
#include "ShadowFilter.h"
#include "ShadowFilterContext.h"

struct IOCTLHelperContext
{
	AppRegisterContext AppContext;
	ShadowFilter * Filter;
	ShadowFilterContext* FilterContext;
	KSPIN_LOCK SpinLock;
	IO_CSQ IoCsq;
	IRP_LINK_ENTRY IrpLinkHeadEntry;
	PFILE_OBJECT OriginalFileObject;
};
