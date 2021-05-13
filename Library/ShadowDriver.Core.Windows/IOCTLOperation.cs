using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Windows
{
    public static class IOCTLOperation
    {
        //DeviceIoControl在C#中的引用和定义
        [DllImport("kernel32.dll", EntryPoint = "DeviceIoControl", SetLastError = true)]
        internal static extern int DeviceIoControl(
            IntPtr hDevice,
            uint dwIoControlCode,
            byte[] lpInBuffer,
            int nInBufferSize,
            byte[] lpOutbuffer,
            int nOutBufferSize,
            ref int lpByteReturned,
            IntPtr lpOverlapped
            );

        [DllImport("kernel32.dll")]
        internal static extern IntPtr CreateFile(
            string lpFileName,
            uint dwDesireAccess,
            uint dwShareMode,
            int lpSecurityAttributes,
            uint dwCreationDisposition,
            int dwFlagsAndAttributes,
            int hTemplateFile
            );

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern int CloseHandle(IntPtr hObject);

        internal const uint GENERIC_READ = 0x80000000;
        internal const uint GENERIC_WRITE = 0x40000000;
        internal const uint OPEN_EXISTING = 3;

        internal const uint FILE_SHARE_READ = 0x00000001;
        internal const uint FILE_SHARE_WRITE = 0x00000002;
    }
}
