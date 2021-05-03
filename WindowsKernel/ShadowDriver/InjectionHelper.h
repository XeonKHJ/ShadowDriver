#pragma once
#include <fwpsk.h>
#include "ShadowDriverStatus.h"
#include "ShadowFilterContext.h"

class InjectionHelper
{
public:
	static NTSTATUS CreateInjector(ShadowFilterContext* context);
	static UINT32 Inject(ShadowFilterContext * context,NetPacketDirection direction, NetLayer layer, void * buffer, SIZE_T size);
	static void DeleteInjectors(ShadowFilterContext* context);
private: 
	static UINT32 GetInjectionFlagByCode(unsigned int code);
	static UINT32 InjectByCode(unsigned int code);
	static void SendInjectCompleted(
		void* context,
		NET_BUFFER_LIST* netBufferList,
		BOOLEAN dispatchLevel
	);
};

