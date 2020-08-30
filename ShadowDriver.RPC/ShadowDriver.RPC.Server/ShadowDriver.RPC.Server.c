// ShadowDriver.RPC.Server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "../ShadowDriver.RPC.Common/ShadowDriverRPC.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

int main()
{
	RPC_STATUS status;

	LPWSTR pszProtocolSequence = L"ncacn_np";
	LPWSTR pszSecurity = NULL;
	LPWSTR pszEndpoint = L"\\pipe\\ShadowDriverRPC";
	unsigned int    cMinCalls = 1;
	unsigned int    fDontWait = FALSE;

	status = RpcServerUseProtseqEp(pszProtocolSequence,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		pszEndpoint,
		pszSecurity);

	if (status) exit(status);

	status = RpcServerRegisterIf(ShadowDriverRPC_v1_0_s_ifspec,
		NULL,
		NULL);

	if (status) exit(status);

	status = RpcServerListen(cMinCalls,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		fDontWait);

	if (status) exit(status);
}

int InitializeRPCServer()
{
	RPC_STATUS status;

	LPWSTR pszProtocolSequence = L"ncacn_np";
	LPWSTR pszSecurity = NULL;
	LPWSTR pszEndpoint = L"\\pipe\\ShadowDriverRPC";
	unsigned int    cMinCalls = 1;
	unsigned int    fDontWait = FALSE;

	status = RpcServerUseProtseqEp(pszProtocolSequence,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		pszEndpoint,
		pszSecurity);

	if (status)
	{
		return -1;
	}

	status = RpcServerRegisterIf(ShadowDriverRPC_v1_0_s_ifspec,
		NULL,
		NULL);

	if (status)
	{
		return -1;
	}

	status = RpcServerListen(cMinCalls,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		fDontWait);

	if (status)
	{
		return -1;
	}
}
