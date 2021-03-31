#pragma once
#include <stdint.h>
enum NetLayer
{
	NetworkLayer,
	LinkLayer
};
enum NetPacketDirection
{
	In,
	Out
};
enum IpAddrFamily
{
	IPv4,
	IPv6
}
class NetFilteringCondition
{
public:
	NetLayer FilterLayer;
	NetPacketDirection FilterPath;
	IpAddrFamily IPAddressType;
	union
	{
		uint32_t IPv4;
		uint64_t IPv6Addr;
	};
};
