using System;
using System.Collections.Generic;
using System.Text;

namespace ShadowDriver.Core.Common
{
    public class PacketInjectionArgs
    {
        public FilteringLayer Layer { set; get; }
        public NetPacketDirection Direction { set; get; }
        public IpAddrFamily AddrFamily { set; get; }
        public ulong Identifier { set; get; }
        public int FragmentIndex { set; get; }
    }
}
