#include "ShadowFilterContext.h"
#include "public.h"
#include "ShadowFilter.h"
#include "ShadowCallouts.h"
#include "WfpHelper.h"
#include "InjectionHelper.h"

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

unsigned int ShadowFilter::AddFilterConditions(FilterCondition* conditions, int length)
{
	NTSTATUS statusCode = 0;
	if (conditions != nullptr && length != 0)
	{
		FilterCondition* newConditions = new FilterCondition[_conditionCount + length];
		FilterCondition* oldConditions = _filteringConditions;
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

#ifdef DBG
			if (!NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error occuers in InitializeWfpEngine routine.\t\n");
			}
#endif

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.InitializeSublayer(context);
			}

#ifdef DBG
			if (!NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error occuers in InitializeSublayer routine.\t\n");
			}
#endif

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.AllocateConditionGroups(_filteringConditions, _conditionCount);
			}

#ifdef DBG
			if (!NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error occuers in AllocateConditionGroups routine.\t\n");
			}
#endif

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.RegisterCalloutsToDevice(context);
			}

#ifdef DBG
			if (!NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error occuers in RegisterCalloutsToDevice routine.\t\n");
			}
#endif

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.AddCalloutsToWfp(context);
			}

#ifdef DBG
			if (!NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error occuers in AddCalloutsToWfp routine.\t\n");
			}
#endif

			if (NT_SUCCESS(status))
			{
				status = wfpHelper.AddFwpmFiltersToWpf(context);
			}

#ifdef DBG
			if (!NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "Error occuers in AddFwpmFiltersToWpf routine.\t\n");
			}
#endif

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
	for (UINT8 currentCode = 0; currentCode < shadowFilterContext->FilterIdMaxNumber; ++currentCode)
	{
		if (shadowFilterContext->FilterIds[currentCode] != NULL)
		{
			// Delete FWPM_FILTERs.
			status = FwpmFilterDeleteById(shadowFilterContext->WfpEngineHandle, shadowFilterContext->FilterIds[currentCode]);

			// Delete FWPM_CALLOUTs.
			FwpmCalloutDeleteById(shadowFilterContext->WfpEngineHandle, (shadowFilterContext->WpmCalloutIds)[currentCode]);
			(shadowFilterContext->WpmCalloutIds)[currentCode] = NULL;

			// Delete FWPS_CALLOUTs.
			status = FwpsCalloutUnregisterById((shadowFilterContext->WpsCalloutIds)[currentCode]);
			(shadowFilterContext->WpsCalloutIds)[currentCode] = NULL;
		}
	}

	// Delete FWPM_SUBLAYER.
	status = FwpmSubLayerDeleteByKey(shadowFilterContext->WfpEngineHandle, &(shadowFilterContext->SublayerGuid));

	// Close FWPM_ENGINE session.
	status = FwpmEngineClose(shadowFilterContext->WfpEngineHandle);
	shadowFilterContext->WfpEngineHandle = NULL;

	shadowFilterContext->IsFilteringStarted = false;
	return status;
}

unsigned int ShadowFilter::EnablePacketModification()
{
	NTSTATUS status = STATUS_SUCCESS;
	//添加注入句柄之类的。

	if (!_isModificationEnabled)
	{
		ShadowFilterContext* context = (ShadowFilterContext*)_context;
		
		if (context != nullptr)
		{
			if (NT_SUCCESS(status))
			{
				_isModificationEnabled = true;
				context->IsModificationEnable = _isModificationEnabled;
			}
		}
	}
	else
	{
		status = SHADOW_FILTER_MODIFICATION_HAS_BEEN_ENABLED;
	}
	return status;
}

void ShadowFilter::DisablePacketModification()
{
	_isModificationEnabled = false;
}

void* ShadowFilter::GetContext()
{
	return _context;
}

bool ShadowFilter::GetModificationStatus()
{
	return _isModificationEnabled;
}

unsigned int ShadowFilter::InjectPacket(void* context, NetPacketDirection direction, NetLayer layer, void* buffer, unsigned long long size)
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(direction);
	UNREFERENCED_PARAMETER(layer);
	UNREFERENCED_PARAMETER(buffer);
	UNREFERENCED_PARAMETER(size);
	return false;
}

unsigned int ShadowFilter::InjectPacket(void* context, NetPacketDirection direction, NetLayer layer, void* buffer, unsigned long long size, unsigned long long identifier, int fragmentIndex)
{
	NTSTATUS status = ShadowCallout::ModifyPacket(context, direction, layer, buffer, size, identifier, fragmentIndex);

	return status;
}
