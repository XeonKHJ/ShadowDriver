#include "ShadowFilterContext.h"
#include "public.h"
#include "ShadowFilter.h"
#include "ShadowCallouts.h"
#include "WfpHelper.h"

ShadowFilter::ShadowFilter(void* enviromentContexts)
{
	_context = enviromentContexts;
	ShadowFilterContext* shadowFilterContext = NULL;
	NetPacketFilteringCallout = NULL;
	_conditionCount = 0;
	_filteringConditions = nullptr;
	NTSTATUS status = STATUS_SUCCESS;
	if (_context)
	{
		shadowFilterContext = (ShadowFilterContext*)_context;
		NetPacketFilteringCallout = shadowFilterContext->NetPacketFilteringCallout;
	}
	else
	{
		status = STATUS_CONTEXT_MISMATCH;
	}
	if (!NT_SUCCESS(status))
	{
		this->~ShadowFilter();
	}
}

ShadowFilter::~ShadowFilter()
{
	auto context = (ShadowFilterContext*)_context;
	if (context->IsFilteringStarted)
	{
		StopFiltering();
	}

	// Delete filtering conditions.
	delete[] _filteringConditions;
}

/*++++++++++++++++++++++++++++++++++++为添加过滤条件做准备的代码++++++++++++++++++++++++++++++++++++++++++++*/
struct NetFilteringConditionAndCode
{
	NetFilteringCondition* CurrentCondition;
	int Code;
	int Index;
	void* Address;
};

/*-----------------------------------为添加过滤条件做准备的代码---------------------------------------------*/

unsigned int ShadowFilter::AddFilterConditions(NetFilteringCondition* conditions, int length)
{
	NTSTATUS statusCode = 0;
	if (conditions != nullptr && length != 0)
	{
		NetFilteringCondition* newConditions = new NetFilteringCondition[_conditionCount + length];
		NetFilteringCondition* oldConditions = _filteringConditions;
		for (int i = 0; i < _conditionCount; ++i)
		{
			newConditions[i] = oldConditions[i];
		}
		for (int i = 0; i < length; ++i)
		{
			newConditions[_conditionCount + i] = conditions[i];
		}

		_conditionCount += length;
		_filteringConditions = newConditions;

		if (oldConditions != nullptr)
		{
			delete[] oldConditions;
		}
	}
	else
	{
		statusCode = SHADOW_APP_NO_CONDITION;
	}
	return statusCode;
}

unsigned int ShadowFilter::StartFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* context = (ShadowFilterContext*)_context;
	WfpHelper wfpHelper{};
	if (!context->IsFilteringStarted)
	{
		if (_conditionCount != 0 && _filteringConditions != nullptr)
		{
			if (NT_SUCCESS(status))
			{
				status = wfpHelper.InitializeWfpEngine(context);
			}

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.InitializeSublayer(context);
			}

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.AllocateConditionGroups(_filteringConditions, _conditionCount);
			}

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.RegisterCalloutsToDevice(context);
			}

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.AddCalloutsToWfp(context);
			}

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.AddFwpmFiltersToWpf(context);
			}

			if (NT_SUCCESS(status))
			{
				context->IsFilteringStarted = TRUE;
			}

			if (!NT_SUCCESS(status))
			{
#ifdef DBG
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error occuers in starting filter process.\t\n");
#endif
				StopFiltering();
			}
		}
		else
		{
			status = SHADOW_FILTER_NO_CONDITION;
		}
	}
	else
	{
		status = SHADOW_FILTER_IS_RUNNING;
	}
	return status;
}

/// <summary>
/// 用于停止过滤。
/// STATUS值待处理。
/// </summary>
/// <returns></returns>
unsigned int ShadowFilter::StopFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* shadowFilterContext = (ShadowFilterContext*)_context;

	// Deregister callouts
	for (UINT8 currentCode = 0; currentCode <= shadowFilterContext->FilterIdMaxNumber; ++currentCode)
	{
		if (shadowFilterContext->FilterIds[currentCode] != NULL)
		{
			// Delete FWPM_FILTERs.
			status = FwpmFilterDeleteById0(shadowFilterContext->WfpEngineHandle, shadowFilterContext->FilterIds[currentCode]);

			// Delete FWPM_CALLOUTs.
			FwpmCalloutDeleteById(shadowFilterContext->WfpEngineHandle, (shadowFilterContext->WpmCalloutIds)[currentCode]);
			(shadowFilterContext->WpmCalloutIds)[currentCode] = NULL;

			// Delete FWPS_CALLOUTs.
			status = FwpsCalloutUnregisterById0((shadowFilterContext->WpsCalloutIds)[currentCode]);
			(shadowFilterContext->WpsCalloutIds)[currentCode] = NULL;
		}
	}

	// Delete FWPM_SUBLAYER.
	status = FwpmSubLayerDeleteByKey0(shadowFilterContext->WfpEngineHandle, &(shadowFilterContext->SublayerGuid));

	// Close FWPM_ENGINE session.
	status = FwpmEngineClose0(shadowFilterContext->WfpEngineHandle);
	shadowFilterContext->WfpEngineHandle = NULL;

	shadowFilterContext->IsFilteringStarted = false;
	return status;
}

void ShadowFilter::EnablePacketModification()
{
	//添加注入句柄之类的。
	_isModificationEnabled = true;
}

void ShadowFilter::DisablePacketModification()
{
	_isModificationEnabled = false;
}
