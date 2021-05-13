#pragma once
#include <fwpsk.h>

struct PacketModificationContext
{
	LIST_ENTRY ListEntry;
	PNET_BUFFER_LIST NetBufferList;
	int FragmentCounts;
	int ReceviedFragmentCounts;
	COMPARTMENT_ID CompartmentId;
	UINT32 InterfaceIndex;
	UINT32 SubInterfaceIndex;
};

