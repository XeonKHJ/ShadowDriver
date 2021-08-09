#pragma once
#include "NetFilteringCondition.h"

class Configuration
{
public:
	union
	{
		unsigned int ProxyIPv4Address;
		unsigned char ProxyIPv6Address[16];
	};

	unsigned short ProxyPort;

	FilterCondition * Conditions;

	static Configuration * ParseConfigration()
	{
		return new Configuration();
	}

private:
	Configuration();
};

