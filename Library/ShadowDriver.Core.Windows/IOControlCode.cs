using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Windows
{
    internal class IOControlCode
    {
        //
        // 摘要:
        //     控件代码。
        //
        // 参数:
        //   deviceType:
        //     设备类型。
        //
        //   function:
        //     设备功能。
        //
        //   accessMode:
        //     访问模式。
        //
        //   bufferingMethod:
        //     缓冲方法。
        public IOControlCode(ushort deviceType, ushort function, IOControlAccessMode accessMode, IOControlBufferingMethod bufferingMethod)
        {
            AccessMode = accessMode;
            BufferingMethod = bufferingMethod;
            DeviceType = deviceType;
            Function = function;
        }

        //
        // 摘要:
        //     访问模式。
        //
        // 返回结果:
        //     访问模式。
        public IOControlAccessMode AccessMode { get; }
        //
        // 摘要:
        //     缓冲方法。
        //
        // 返回结果:
        //     缓冲方法。
        public IOControlBufferingMethod BufferingMethod { get; }
        //
        // 摘要:
        //     控件代码。
        //
        // 返回结果:
        //     控件代码。
        public uint ControlCode
        {
            get
            {
                uint controlCode = (((uint)DeviceType) << 16) | (((uint)AccessMode) << 14) | (((uint)Function) << 2) | ((uint)BufferingMethod);
                return controlCode;
            }
        }
        //
        // 摘要:
        //     设备类型。
        //
        // 返回结果:
        //     设备类型。
        public ushort DeviceType { get; }
        //
        // 摘要:
        //     函数。
        //
        // 返回结果:
        //     函数。
        public ushort Function { get; }
    }
}
