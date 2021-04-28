using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.UWPDemo.ViewModel
{
    public class NetworkInterfaceViewModel
    {
        public NetworkInterface NetworkInterface;
        public string Id { set; get; }
        public string Name { set; get; }
        public string MacAddress { set; get; }

        public override string ToString()
        {
            return string.Format("{0} {1}", NetworkInterface.Name, NetworkInterface.GetPhysicalAddress().ToString());
        }
    }
}
