#pragma once
#include "fwpsk.h"
#include "NetFilteringCondition.h"

class PacketHelper
{
public:
	static void FilterFunc(NetLayer netLayer, NetPacketDirection direction, void* buffer, unsigned long long bufferSize, void* context);
};
