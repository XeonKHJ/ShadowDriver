using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Windows
{
    class IOCTLOperation
    {
        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
        internal static extern bool DeviceIoControl(
            IntPtr hFile,
            Int32 dwIoControlCode,
            ref Int64 lpInBuffer,
            Int32 nInBufferSize,
            ref Int64 lpOutBuffer,
            Int32 nOutBufferSize,
            out Int32 lpBytesReturned,
            IntPtr lpOverlapped
            );
    }
}
