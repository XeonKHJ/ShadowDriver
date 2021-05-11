#pragma once
#include <fwpsk.h>

struct NetBufferListEntry
{
	LIST_ENTRY ListEntry;
	PNET_BUFFER_LIST NetBufferList;
	int FragmentCounts;
	int ReceviedFragmentCounts;
	COMPARTMENT_ID CompartmentId;
};

