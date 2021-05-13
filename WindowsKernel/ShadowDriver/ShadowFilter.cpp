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
/// ����ֹͣ���ˡ�
/// STATUSֵ������
/// </summary>
/// <returns></returns>
unsigned int ShadowFilter::StopFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* context = (ShadowFilterContext*)_context;

	// Deregister callouts
	for (UINT8 code = 0; code < context->FilterIdMaxNumber; ++code)
	{
		if (context->FilterIds[code] != NULL)
		{
			// Delete FWPM_FILTERs.
			status = FwpmFilterDeleteById(context->WfpEngineHandle, context->FilterIds[code]);
			context->FilterIds[code] = NULL;
		}
		if (context->WpmCalloutIds[code] != NULL)
		{
			// Delete FWPM_CALLOUTs.
			FwpmCalloutDeleteById(context->WfpEngineHandle, (context->WpmCalloutIds)[code]);
			(context->WpmCalloutIds)[code] = NULL;
		}
		if (context->WpsCalloutIds[code] != NULL)
		{
			// Delete FWPS_CALLOUTs.
			status = FwpsCalloutUnregisterById((context->WpsCalloutIds)[code]);
			(context->WpsCalloutIds)[code] = NULL;
		}

	}

	// Delete FWPM_SUBLAYER.
	status = FwpmSubLayerDeleteByKey(context->WfpEngineHandle, &(context->SublayerGuid));

	// Close FWPM_ENGINE session.
	status = FwpmEngineClose(context->WfpEngineHandle);
	context->WfpEngineHandle = NULL;

	context->IsFilteringStarted = false;
	return status;
}

unsigned int ShadowFilter::EnablePacketModification()
{
	NTSTATUS status = STATUS_SUCCESS;
	//���ע����֮��ġ�

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
