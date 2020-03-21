using System;
using System.Net;

namespace ShadowDriver.Common
{
    public class IPPacket
    {
        public IPAddress DestinationIP { set; get; }
        public IPAddress SourceIP { set; get; }
    }
}
