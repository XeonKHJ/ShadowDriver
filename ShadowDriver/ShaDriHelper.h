#pragma once
#include "ShadowUtilities.h"
#include "driver.h"
#include "fwpsk.h"

void ConvertNetBufferListToTcpRawPacket(_Out_ ShadowTcpRawPacket** tcpRawPacketPoint, _In_ PNET_BUFFER_LIST nbl, _In_ UINT32 nblOffset);
void CalculateTcpPacketCheckSum(short transportDataLength, PCHAR mdlCharBuffer, CHAR protocal, short ipHeaderLength, PNET_BUFFER_LIST packet);
void DeleteTcpRawPacket(_In_ ShadowTcpRawPacket* tcpRawPacket);
void PrintNetBufferList(PNET_BUFFER_LIST packet, ULONG level);