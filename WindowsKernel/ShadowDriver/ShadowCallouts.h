#pragma once
#include <fwpsk.h>
#include "NetFilteringCondition.h"

class ShadowCallout
{
private:
	static NTSTATUS CalloutPreproecess(
		_Inout_opt_ void* layerData,
		_In_ const FWPS_FILTER0* filter,
		_Inout_ FWPS_CLASSIFY_OUT0* classifyOut,
		NetLayer layer,
		NetPacketDirection direction
	);
public:
	static NTSTATUS PacketNotify(
		_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
		_In_ const GUID* filterKey,
		_Inout_ FWPS_FILTER0* filter
	);

	static void PacketFlowDeleteNotfy(
		_In_ UINT16 layerId,
		_In_ UINT32 calloutId,
		_In_ UINT64 flowContext
	);

	static VOID NTAPI NetworkInV4ClassifyFn(
		_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_ const FWPS_FILTER0* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
	);

	static VOID NTAPI NetworkOutV4ClassifyFn(
		_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_ const FWPS_FILTER0* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
	);

	static VOID NTAPI NetworkInV6ClassifyFn(
		_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_ const FWPS_FILTER0* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
	);

	static VOID NTAPI NetworkOutV6ClassifyFn(
		_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_ const FWPS_FILTER0* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
	);

	static VOID NTAPI LinkOutClassifyFn(
		_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_ const FWPS_FILTER0* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
	);

	static VOID NTAPI LinkInClassifyFn(
		_In_ const FWPS_INCOMING_VALUES0* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_ const FWPS_FILTER0* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT0* classifyOut
	);
};