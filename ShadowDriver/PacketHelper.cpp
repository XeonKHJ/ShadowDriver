#include "PacketHelper.h"
#include "IOCTLHelper.h"

void FilterFunc(NetLayer netLayer, NetPacketDirection direction, void* buffer, unsigned long long bufferSize)
{
	IOCTLHelper::NotifyUserApp(buffer, bufferSize);
}