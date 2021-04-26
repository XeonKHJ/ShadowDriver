#include "PacketHelper.h"
#include "IOCTLHelper.h"

void FilterFunc(NetLayer netLayer, NetPacketDirection direction, void* buffer, unsigned long long bufferSize, void * context)
{
	auto helper = (IOCTLHelper*)context;

	if (helper != nullptr)
	{
		helper->NotifyUserApp(buffer, bufferSize);
	}
}