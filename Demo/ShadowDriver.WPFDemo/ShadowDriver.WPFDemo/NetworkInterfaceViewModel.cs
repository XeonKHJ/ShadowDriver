using System.Net.NetworkInformation;

namespace ShadowDriver.WPFDemo
{
    public class NetworkInterfaceViewModel
    {
        public string Id { get; set; }
        public string MacAddress { get; set; }
        public string Name { get; set; }
        public NetworkInterface NetworkInterface { get; set; }

        public override string ToString()
        {
            return string.Format("{0} {1}", NetworkInterface.Name, NetworkInterface.GetPhysicalAddress().ToString());
        }
    }
}