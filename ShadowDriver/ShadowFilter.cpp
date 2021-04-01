#include "ShadowFilter.h"
#include "ShadowCommon.h"
#include "ShadowFilterWindowsSpecific.h"

HANDLE EngineHandler = NULL;

NTSTATUS InitializeWfpEngine(ShadowFilterContext *context)
{
	NTSTATUS status = STATUS_SUCCESS;

	auto pSession = &(context->WfpSession);
	FWPM_SESSION0 session;
	FWPM_PROVIDER0 provider;
	memset(&session, 0, sizeof(session));
	session.displayData.name = L"ShadowDriver Session";
	session.txnWaitTimeoutInMSec = 0xFFFFFFFF;
	//status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, pEngineHandler);;

	return status;
}

NTSTATUS RegisterCalloutFuntions(ShadowFilterContext *context, NetFilteringCondition *conditions)
{
	NTSTATUS status = STATUS_SUCCESS;
	return status;
}

ShadowFilter::ShadowFilter(void *enviromentContexts)
{
	_context = enviromentContexts;
	ShadowFilterContext *shadowFilterContext = NULL;
	NTSTATUS status;
	if (_context)
	{
		shadowFilterContext = (ShadowFilterContext *)_context;
		InitializeWfpEngine(shadowFilterContext);
	}
	else
	{
		//取消构造

	}
}

int ShadowFilter::AddFilterCondition(NetFilteringCondition* conditions, int length)
{
	if (conditions != nullptr)
	{
		NTSTATUS status = STATUS_SUCCESS;
		FWPM_FILTER_CONDITION0* wpmConditions = (FWPM_FILTER_CONDITION0*)ExAllocatePoolWithTag(NonPagedPool, sizeof(FWPM_FILTER_CONDITION0) * length, 'nfcs');
		if (wpmConditions != 0)
		{
			memset(wpmConditions, 0, sizeof(FWPM_FILTER_CONDITION0) * length);

			FWPM_FILTER0 sendFilter = { 0 };
			for (int currentIndex = 0; currentIndex < length; ++currentIndex)
			{
				FWPM_FILTER0 wpmFilter = { 0 };
				FWP_V4_ADDR_AND_MASK addrandMask = { 0 };
				addrandMask.addr = conditions[currentIndex].IPv4;
				switch (conditions[currentIndex].FilterLayer)
				{
				case NetLayer::LinkLayer:

					break;
				case NetLayer::NetworkLayer:
				{
					switch (conditions[currentIndex].IPAddressType)
					{
					case IpAddrFamily::IPv4:
						break;
					case IpAddrFamily::IPv6:
						break;
					}
				}
				break;
				}
				FWPM_FILTER0 sendFilter = { 0 };
				FWPM_FILTER_CONDITION0 condition[1] = { 0 };
				FWP_V4_ADDR_AND_MASK AddrandMask = { 0 };
			}
		}

		
	}
	return 0;
}

int ShadowFilter::StartFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext *shadowFilterContext = (ShadowFilterContext *)_context;
	if (!NT_SUCCESS(status))
	{
		//status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &(shadowFilterContext->WfpSession), &(shadowFilterContext->WfpEngineHandle));
		;
	}

	if (!NT_SUCCESS(status))
	{
		status = RegisterCalloutFuntions(shadowFilterContext, NULL);
	}

	return status;
}

void ShadowFilter::EnablePacketModification()
{
}

void ShadowFilter::DisablePacketModification()
{
}
