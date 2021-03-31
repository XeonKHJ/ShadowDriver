#pragma once

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
};
class NetFilteringCondition
{
public:
	NetLayer FilterLayer;
	NetPacketDirection FilterPath;
	IpAddrFamily IPAddressType;
	union
	{
		unsigned int IPv4;
		unsigned long long IPv6Addr;
	};
};
