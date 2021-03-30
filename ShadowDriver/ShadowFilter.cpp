#include "ShadowFilter.h"
#include <fwpmk.h>
#include <fwpsk.h>
#include "WinSpecific.h"

HANDLE EngineHandler = NULL;

NTSTATUS InitializeWfpEngine(ShadowFilterContext* context)
{
	auto pSession = &(context->WfpSession);
	FWPM_SESSION0 session;
	FWPM_PROVIDER0 provider;
	memset(&session, 0, sizeof(session));
	session.displayData.name = L"ShadowDriver Session";
	session.txnWaitTimeoutInMSec = 0xFFFFFFFF;
	//status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, pEngineHandler);;
}

NTSTATUS RegisterCalloutFuntions(ShadowFilterContext* context, NetFilteringCondition * conditions)
{
	if (conditions != nullptr)
	{

	}
	
}

ShadowFilter::ShadowFilter(void* enviromentContexts)
{
	_context = enviromentContexts;
	ShadowFilterContext* shadowFilterContext = NULL;
	NTSTATUS status;
	if (_context)
	{
		shadowFilterContext = (ShadowFilterContext*)_context;
		InitializeWfpEngine(shadowFilterContext);
	}
	else
	{
		//取消构造
	}
	
}

int ShadowFilter::AddFilterCondition(NetFilteringCondition* conditions)
{
	return 0;
}

int ShadowFilter::StartFiltering()
{
	NTSTATUS status = STATUS_SUCCESS;
	ShadowFilterContext* shadowFilterContext = _context;
	if (!NT_SUCCESS(status))
	{
		status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &(shadowFilterContext->WfpSession), &(shadowFilterContext->WfpEngineHandle));;
	}

	if (!NT_SUCCESS(status))
	{
		status = RegisterCalloutFuntions(shadowFilterContext);
	}

	return status;
}

void ShadowFilter::EnablePacketModification()
{

}

void ShadowFilter::DisablePacketModification()
{
}
