#pragma once
#include <fwpsk.h>

struct PacketModificationContext
{
	//LIST_ENTRY ListEntry;
	PNET_BUFFER_LIST OriginalNBL;
	PNET_BUFFER_LIST NewNBL;
	int FragmentCounts;
	int ReceviedFragmentCounts;
	COMPARTMENT_ID CompartmentId;
	UINT32 InterfaceIndex;
	UINT32 SubInterfaceIndex;
	NDIS_PORT_NUMBER NdisPortNumber;
	UINT32 MacHeader;
};

