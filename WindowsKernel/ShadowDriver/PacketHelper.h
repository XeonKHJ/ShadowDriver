#pragma once
#include "fwpsk.h"
#include "NetFilteringCondition.h"

void FilterFunc(NetLayer netLayer, NetPacketDirection direction, void* buffer, unsigned long long bufferSize, void * context);