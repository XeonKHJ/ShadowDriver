#include "../ShadowDriver.RPC.Common/ShadowDriverRPC.h"
#include <iostream>

void RPCTest(
    /* [string][in] */ unsigned char* pszString)
{
	std::cout << pszString << std::endl;
}

void GetShadowDriverVersion(void)
{

}

void Shutdown(void)
{

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