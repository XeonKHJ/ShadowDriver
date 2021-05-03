using System;
using System.Collections.Generic;
using System.Text;

namespace ShadowDriver.Core.Common
{
    public delegate byte[] PacketReceivedEventHandler(byte[] buffer);
    public delegate void FilterReadyEventHandler();
}
