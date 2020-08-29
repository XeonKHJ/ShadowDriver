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

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(size)
void* __RPC_USER MIDL_user_allocate(_In_ size_t size)
{
	return(malloc(size));
}

void __RPC_USER midl_user_free(_Pre_maybenull_ _Post_invalid_ void* ptr)
{
	free(ptr);
}
