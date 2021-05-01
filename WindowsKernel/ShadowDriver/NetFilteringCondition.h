#pragma once

enum NetLayer
{
	NetworkLayer = 0,
	LinkLayer = 1
};
enum NetPacketDirection
{
	Out = 0,
	In = 1,
};
enum IpAddrFamily
{
	IPv4 = 0,
	IPv6 = 1
};
enum AddressLocation
{
	Local = 0,
	Remote = 1
};
enum FilterMatchType
{
	Equal = 0,
	Greater = 1,
	Less = 2,
	GreaterOrEqual = 3,
	LessOrEqual = 4,
	NotEqual = 5
};

class NetFilteringCondition
{
public:
	NetLayer FilterLayer;
	NetPacketDirection FilterPath;
	IpAddrFamily IPAddressType;
	union
	{
		unsigned int IPv4Address;
		unsigned char IPv6Address[16];
	};
	union
	{
		unsigned int IPv4Mask;
		unsigned char IPv6Mask[16];
	};
	unsigned int InterfaceId;
	AddressLocation AddrLocation;
	FilterMatchType MatchType;
};
