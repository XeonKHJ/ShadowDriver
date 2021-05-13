using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Windows
{
    internal enum IOControlAccessMode
    {
        //
        // 摘要:
        //     任何模式。
        Any = 0,
        //
        // 摘要:
        //     读取模式。
        Read = 1,
        //
        // 摘要:
        //     写模式。
        Write = 2,
        //
        // 摘要:
        //     读/写模式。
        ReadWrite = 3
    }

    internal enum IOControlBufferingMethod
    {
        //
        // 摘要:
        //     已缓冲。
        Buffered = 0,
        //
        // 摘要:
        //     直接输入。
        DirectInput = 1,
        //
        // 摘要:
        //     直接输出。
        DirectOutput = 2,
        //
        // 摘要:
        //     二者都不是。
        Neither = 3
    }
}
