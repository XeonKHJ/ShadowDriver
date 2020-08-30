#include "../ShadowDriver.RPC.Common/ShadowDriverRPC.h"
#include "ShadowDriver.RPC.Server.h"

int main()
{
	RPC_STATUS status = InitializeShadowDriverRPCServer();

	return status;
}