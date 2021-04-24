#pragma once
#include <wdm.h>
#include "IOCTLHelperContext.h"
#include "Public.h"
#include "ShadowFilter.h"

struct IOCTLHelperLinkEntry;

class IOCTLHelper
{
public:
	IOCTLHelper(IOCTLHelperContext context);
	~IOCTLHelper();
	static void InitializeDriverObjectForIOCTL(_In_ PDRIVER_OBJECT driverObject);
	static int _helperCount;
	static NTSTATUS NotifyUserApp(void* buffer, size_t size);
	static void SetDeviceObject(PDEVICE_OBJECT deviceObject);
private:
	static void AddHelper(IOCTLHelper * helper);
	static void RemoveHelper(IOCTLHelper* helper);
	static NTSTATUS ShadowDriverIrpIoControl(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp);
	static NTSTATUS RegisterAppForIOCTLCalls(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static IOCTLHelperLinkEntry _helperListHeader;
	static AppRegisterContext GetAppContextFromIoctl(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static IOCTLHelper* GetHelperByAppId(int id);
	static void NotifyUserByDequeuingIoctl(IOCTLHelperContext* context, void* outputBuffer, size_t outputLength);
	static NTSTATUS DeregisterAppForIOCTLCalls(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static NTSTATUS GetQueuedIoctlCount(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static NTSTATUS IoctlStartFiltering(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static NTSTATUS IoctlAddCondition(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static PDEVICE_OBJECT _deviceObject;
	NTSTATUS InitializeIRPNotificationSystem();
	IOCTLHelperContext _context;
};

struct IOCTLHelperLinkEntry
{
	LIST_ENTRY ListEntry;
	IOCTLHelper* Helper;
};



