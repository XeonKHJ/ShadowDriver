using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.UWPDemo.ViewModel
{
    public class NetPacketViewModel
    {
        public string Content { set; get; } = string.Empty;

        public override string ToString()
        {
            return Content;
        }
    }
}
