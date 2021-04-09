#pragma once
#include <wdm.h>
#include "IOCTLHelperContext.h"
class IOCTLHelper
{
public:
	IOCTLHelper(_In_ PDRIVER_OBJECT);
	IRP_LINK_ENTRY* InitializeIrpLinkEntry();
	static void InitializeDriverObjectForIOCTL(_In_ PDRIVER_OBJECT driverObject);
private:
	static IOCTLHelper* _helpers;
	IOCTLHelperContext _context;
	static NTSTATUS ShadowDriverIrpIoControl(
		_In_ struct _DEVICE_OBJECT* DeviceObject,
		_Inout_ struct _IRP* Irp
	);
	NTSTATUS InitializeIRPNotificationSystem();
};

