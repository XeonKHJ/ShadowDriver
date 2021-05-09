#pragma once
#include <fwpsk.h>

struct NetBufferListEntry
{
	LIST_ENTRY ListEntry;
	NET_BUFFER_LIST NetBufferList;
	bool* IsModifiedFragementReceived;
};

