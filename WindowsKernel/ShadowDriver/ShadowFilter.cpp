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
	UNREFERENCED_PARAMETER(buffer);
	UNREFERENCED_PARAMETER(layer);
	UNREFERENCED_PARAMETER(direction);
	UNREFERENCED_PARAMETER(context);

	NTSTATUS status = STATUS_SUCCESS;
	PNET_BUFFER_LIST netBufferList = (PNET_BUFFER_LIST)identifier;
	if (netBufferList)
	{
		NetBufferListEntry * netBufferListHeader = &(ShadowCallout::PendingNetBufferListHeader);
		auto currentNetBufferListEntry = CONTAINING_RECORD(netBufferListHeader->ListEntry.Flink, NetBufferListEntry, ListEntry);
		PNET_BUFFER_LIST nblToSend = nullptr;
		
		while (currentNetBufferListEntry != netBufferListHeader)
		{
			if (currentNetBufferListEntry->NetBufferList == netBufferList)
			{
				nblToSend = netBufferList;
				(currentNetBufferListEntry->ReceviedFragmentCounts)++;
				break;
			}
			currentNetBufferListEntry = CONTAINING_RECORD(currentNetBufferListEntry->ListEntry.Flink, NetBufferListEntry, ListEntry);
		}

		if (nblToSend)
		{
			PNET_BUFFER firstNetBuffer = NET_BUFFER_LIST_FIRST_NB(nblToSend);

			if (firstNetBuffer)
			{
				// Get selected net buffer to modification.
				PNET_BUFFER currentBuffer = firstNetBuffer;
				for (int currentFragmentIndex = 0; currentBuffer != nullptr && currentFragmentIndex < fragmentIndex; ++currentFragmentIndex)
				{
					currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
				}

				// Start modification.
				if (currentBuffer)
				{
					//PMDL currentMdl = NET_BUFFER_CURRENT_MDL(currentBuffer);
					//PBYTE currentMdlBuffer = (PBYTE)MmGetMdlVirtualAddress(currentMdl);

					SIZE_T dataWritten = 0;
					for (PMDL currentMdl = NET_BUFFER_CURRENT_MDL(currentBuffer); currentMdl != nullptr && dataWritten < size; currentMdl = currentMdl->Next)
					{
#ifdef DBG
						DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "MDLByteCount: %d, MDLByteOffset: %d, DataToWrite: %d", currentMdl->ByteCount, currentMdl->ByteOffset, (int)size);
						dataWritten += size;
#endif
					}
				}
				// Indicate that fragment index is out of index.
				else
				{
					//status = 
				}
			}


			// REMEMBER TO DELETE IT LATER!
#ifdef DBG
			FwpsFreeCloneNetBufferList(nblToSend, 0);
#endif

		}

		// If all fragments are received, the driver injectst the NET_BUFFER_LIST to network stack and deque this NET_BUFFER_LIST from callout queue.
		if (nblToSend && currentNetBufferListEntry->ReceviedFragmentCounts == currentNetBufferListEntry->FragmentCounts)
		{
			//PNET_BUFFER firstNetBuffer = NET_BUFFER_LIST_FIRST_NB(nblToSend);
			//PNET_BUFFER currentNetBuffer = firstNetBuffer;
			for (size_t i = 0; i < 32; i++)
			{

			}
			RemoveEntryList(&currentNetBufferListEntry->ListEntry);
			delete currentNetBufferListEntry;

		}

	}
	return status;
}
