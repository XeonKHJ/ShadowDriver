#pragma once
#include <fwpsk.h>
#include <fwpmk.h>
#include "NetFilteringCondition.h"
#include "ShadowFilterContext.h"
class WfpHelper
{
	// Arrange in order.
public:
	~WfpHelper();
	NTSTATUS InitializeWfpEngine(ShadowFilterContext* context);
	NTSTATUS InitializeSublayer(ShadowFilterContext* context);
	NTSTATUS AllocateConditionGroups(NetFilteringCondition* conditionsToGroup, int conditionCount);
	NTSTATUS RegisterCalloutsToDevice(_Inout_ ShadowFilterContext* context);
	NTSTATUS AddCalloutsToWfp(_Inout_ ShadowFilterContext* context);
	
	NTSTATUS AddFwpmFiltersToWpf(ShadowFilterContext* context);
private:
	NetFilteringCondition** _conditionsByCode[8];
	FWPM_FILTER_CONDITION* _fwpmConditionsByCode[8];
	void** _addressesByCode[8];
	int _groupCounts[8] = { 0 };
	static UINT8 CalculateFilterLayerAndPathCode(NetFilteringCondition* currentCondition);
	static GUID GetLayerKeyByCode(UINT8 code);
	static void AddCalloutsAccrodingToCode(FWPS_CALLOUT* callout, UINT8 code);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="condition"></param>
	/// <param name="status">A pointer to NTSTATUS. During converting process, if error occures, this function will set status in the pointer.</param>
	/// <returns>Address field in return value is dynamic allocated.</returns>
	static FWPM_FILTER_CONDITION ConvertToFwpmCondition(NetFilteringCondition* condition, _Inout_ NTSTATUS* status);
};

