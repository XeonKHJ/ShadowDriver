using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core
{
    static class DriverRelatedInformation
    {
        static public Guid InterfaceGuid { get; } = new Guid("45f22bb7-6bc3-4545-96ed-73de89c46e7d");
        static public int AppRegisterContextMaxSize { get; } = sizeof(int) + 40;
    }
}
