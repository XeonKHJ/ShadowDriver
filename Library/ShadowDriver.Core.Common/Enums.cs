using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Common
{
	public enum FilteringLayer
	{
		NetworkLayer = 0,
		LinkLayer = 1
	}
	public enum NetPacketDirection
	{
		Out = 0,
		In = 1,
	}
	public enum IpAddrFamily
	{
		IPv4 = 0,
		IPv6 = 1
	}
	public enum AddressLocation
	{
		Local = 0,
		Remote = 1
	}
	public enum FilterMatchType
	{
		Equal = 0,
		Greater = 1,
		Less = 2,
		GreaterOrEqual = 3,
		LessOrEqual = 4,
		NotEqual = 5
	}
}
