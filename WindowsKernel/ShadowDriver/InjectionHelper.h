#pragma once
#include <fwpsk.h>
#include "ShadowDriverStatus.h"
#include "ShadowFilterContext.h"

class InjectionHelper
{
public:
	static NTSTATUS CreateInjector();
	static UINT32 Inject(ShadowFilterContext * context,NetPacketDirection direction, NetLayer layer, void * buffer, SIZE_T size);
	static NTSTATUS Inject(void* context, NetPacketDirection direction, NetLayer layer, PNET_BUFFER_LIST bufferList);
	static HANDLE NDISPoolHandle;
	static void DeleteInjectors();
	static HANDLE InjectionHandles[8];
	static void SendInjectCompleted(
		void* context,
		NET_BUFFER_LIST* netBufferList,
		BOOLEAN dispatchLevel
	);
	static void ModificationCompleted(
		void* context,
		NET_BUFFER_LIST* netBufferList,
		BOOLEAN dispatchLevel
	);
	static void (* InjectCompleted)(PNET_BUFFER_LIST injectedBuffer, void* context);
private: 
	static UINT32 GetInjectionFlagByCode(unsigned int code);
	static UINT32 InjectByCode(unsigned int code);
};

