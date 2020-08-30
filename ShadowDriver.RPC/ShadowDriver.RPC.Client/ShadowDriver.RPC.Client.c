#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "../ShadowDriver.RPC.Common/ShadowDriverRPC.h" 
#include <windows.h>


void main()
{
    RPC_STATUS status;
    LPWSTR pszUuid = NULL;
    LPWSTR pszProtocolSequence = L"ncacn_np";
    LPWSTR pszNetworkAddress = NULL;
    LPWSTR pszEndpoint = L"\\pipe\\ShadowDriverRPC";
    LPWSTR pszOptions = NULL;
    LPWSTR pszStringBinding = NULL;
    LPWSTR pszString = L"hello, world";
    unsigned long ulCode;

    status = RpcStringBindingCompose(pszUuid,
        pszProtocolSequence,
        pszNetworkAddress,
        pszEndpoint,
        pszOptions,
        &pszStringBinding);
    if (status) exit(status);

    status = RpcBindingFromStringBinding(pszStringBinding, &ShadowDriverRPC_IfHandle);

    if (status) exit(status);

    RpcTryExcept
    {
        RPCTest(pszString);
        Shutdown();
    }
        RpcExcept(1)
    {
        ulCode = RpcExceptionCode();
        printf("Runtime reported exception 0x%lx = %ld\n", ulCode, ulCode);
    }
    RpcEndExcept

        status = RpcStringFree(&pszStringBinding);

    if (status) exit(status);

    status = RpcBindingFree(&ShadowDriverRPC_IfHandle);

    if (status) exit(status);

    exit(0);
}

/******************************************************/
/*         MIDL allocate and free                     */
/******************************************************/

void __RPC_FAR* __RPC_USER midl_user_allocate(size_t len)
{
    return(malloc(len));
}

void __RPC_USER midl_user_free(void __RPC_FAR* ptr)
{
    free(ptr);
}