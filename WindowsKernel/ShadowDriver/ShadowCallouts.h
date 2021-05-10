#pragma once
#include <fwpsk.h>
#include "ShadowFilterContext.h"
#include "NetFilteringCondition.h"
#include "NetBufferListEntry.h"

class ShadowCallout
{
private:
	static NTSTATUS CalloutPreproecess(
		_Inout_opt_ void* layerData,
		_In_ const FWPS_FILTER* filter,
		_Inout_ FWPS_CLASSIFY_OUT* classifyOut,
		NetLayer layer,
		NetPacketDirection direction
	);

	static void SendPacketToUserMode(
		NetLayer layer,
		NetPacketDirection direction,
		PNET_BUFFER_LIST netBufferList,
		ShadowFilterContext* context
	);

public:
	static NetBufferListEntry PendingNetBufferListHeader;
	static KSPIN_LOCK SpinLock;
	static NTSTATUS InitializeNBLListHeader();
	static NTSTATUS PacketNotify(
		_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
		_In_ const GUID* filterKey,
		_Inout_ FWPS_FILTER* filter
		);

	static void PacketFlowDeleteNotfy(
		_In_ UINT16 layerId,
		_In_ UINT32 calloutId,
		_In_ UINT64 flowContext
	);

	static VOID NTAPI NetworkInV4ClassifyFn(
		_In_ const FWPS_INCOMING_VALUES* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_opt_ const void* classifyContext,
		_In_ const FWPS_FILTER* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT* classifyOut
	);

	static VOID NTAPI NetworkOutV4ClassifyFn(
		_In_ const FWPS_INCOMING_VALUES* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_opt_ const void* classifyContext,
		_In_ const FWPS_FILTER* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT* classifyOut
	);

	static VOID NTAPI NetworkInV6ClassifyFn(
		_In_ const FWPS_INCOMING_VALUES* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_opt_ const void* classifyContext,
		_In_ const FWPS_FILTER* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT* classifyOut
	);

	static VOID NTAPI NetworkOutV6ClassifyFn(
		_In_ const FWPS_INCOMING_VALUES* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_opt_ const void* classifyContext,
		_In_ const FWPS_FILTER* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT* classifyOut
	);

	static VOID NTAPI LinkOutClassifyFn(
		_In_ const FWPS_INCOMING_VALUES* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_opt_ const void* classifyContext,
		_In_ const FWPS_FILTER* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT* classifyOut
	);

	static VOID NTAPI LinkInClassifyFn(
		_In_ const FWPS_INCOMING_VALUES* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_opt_ const void* classifyContext,
		_In_ const FWPS_FILTER* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT* classifyOut
	);
};