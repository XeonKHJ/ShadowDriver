using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.WPFDemo
{
    public class FiltreStatusViewModel
    {
        public string DeviceConnectStatus { get; internal set; }
        public string AppRegisterStatus { get; internal set; }
        public int QueuedIOCTLCount { get; internal set; }
    }
}