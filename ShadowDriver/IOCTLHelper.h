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
	static IOCTLHelperLinkEntry _helperListHeader;
	static AppRegisterContext GetAppContextFromIoctl(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static IOCTLHelper* GetHelperByAppId(int id);
	static void NotifyUserByDequeuingIoctl(IOCTLHelperContext* context, void* outputBuffer, size_t outputLength);
	static NTSTATUS IoctlRegisterApp(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static NTSTATUS IoctlDeregisterApp(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static NTSTATUS IoctlGetQueuedIoctlCount(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static NTSTATUS IoctlStartFiltering(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static NTSTATUS IoctlStopFiltering(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static NTSTATUS IoctlAddCondition(PIRP irp, PIO_STACK_LOCATION ioStackLocation);
	static int GetAppIdFromIoctl(PIRP irp, PIO_STACK_LOCATION ioStackLocation);

	/// <summary>
	/// Write status to output buffer. Irp->IoStatus.
	/// Note: Information will be added by the size of status (4 bytes).
	/// If you want to output any further data. You will need to use the pointer that this function returns, and then set add size of data you output to Irp->IoStatus.Information = dwDataWritten.
	/// Wait. Why the fuck this looks like bad idea right now.
	/// I will need to output all data before I can determern what status value is.
	/// Don't want to redesign this, let's keep this way.
	/// </summary>
	/// <param name="status">Status code.</param>
	/// <param name="irp"></param>
	/// <param name="ioStackLocation"></param>
	/// <returns>if output buffer length is smaller then size of int (4 bytes), then return nullptr, otherwise return a pointer that points to next byte location after the written value</returns>
	static void* WriteStatusToOutputBuffer(NTSTATUS* status, PIRP irp, PIO_STACK_LOCATION ioStackLocation);

	/// <summary>
	/// This function unify the output interface for the driver.
	/// It will write data from first parameter to the output buffer of irp, it will automaticly leave the first 4 byte of output buffer empty for status, and start writing the output buffer from 5th byte.
	/// irp->IoStatus.Information += bufferSize will be added by size of output data. So make sure that irp->IoStatus.Information is zero before calling this function.
	/// </summary>
	/// <param name="bufferToWrite">This buffer contains data that will be written to the output buffer.</param>
	/// <param name="bufferSize">Size of the buffer from first parameter</param>
	/// <param name="irp"></param>
	/// <param name="ioStackLocation"></param>
	/// <returns>If output buffer size is smaller then (buffersize + status size), then the return value is STATUS_BUFFER_TOO_SMALL. If everything goes well, then the return value is STATUS_SUCCESS.</returns>
	static NTSTATUS WriteDataToIrpOutputBuffer(PVOID bufferToWrite, SIZE_T bufferSize, PIRP irp, PIO_STACK_LOCATION ioStackLocation);

	static PDEVICE_OBJECT _deviceObject;
	NTSTATUS InitializeIRPNotificationSystem();
	IOCTLHelperContext _context;
};

struct IOCTLHelperLinkEntry
{
	LIST_ENTRY ListEntry;
	IOCTLHelper* Helper;
};



