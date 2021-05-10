using System;
using System.Collections.Generic;
using System.Text;

namespace ShadowDriver.Core.Common
{
    public class CapturedPacketArgs
    {
        public CapturedPacketArgs(ulong identifier, long packetSize, int fragmentCounts, int fragmentIndex)
        {
            Identifier = identifier;
            PacketSize = packetSize;
            FragmentCounts = fragmentCounts;
            FragmentIndex = fragmentIndex;
        }
        public long PacketSize { get; private set; }
        public int FragmentCounts { get; private set; }
        public int FragmentIndex { get; private set; }
        public ulong Identifier { get; private set; }
    }
}
