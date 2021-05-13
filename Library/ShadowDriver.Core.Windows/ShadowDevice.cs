using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Windows
{
    struct DummySturcture
    {
        public uint Offset;
        public uint OffestHigh;
    };

    [StructLayout(LayoutKind.Explicit)]
    struct DummpyUnion
    {
        [FieldOffset(0)]
        public DummySturcture DUMMYSTRUCTNAME;
        [FieldOffset(0)]
        public IntPtr Pointer;
    }
    struct OVERLAPPED
    {
        public IntPtr Internal;
        public IntPtr InternalHigh;
        public DummpyUnion DUMMYUNIONNAME;
        public IntPtr hEvent;
    }

    internal class ShadowDevice
    {
        [DllImport("kernel32.dll", EntryPoint = "DeviceIoControl", SetLastError = true)]
        private static extern uint DeviceIoControl(IntPtr hDevice, uint dwIoControlCode, IntPtr lpInBuffer, int nInBufferSize, IntPtr lpOutbuffer, int nOutBufferSize, ref uint lpByteReturned, ref OVERLAPPED lpOverlapped);

        [DllImport("kernel32.dll")]
        private static extern IntPtr CreateFile(string lpFileName, uint dwDesireAccess, uint dwShareMode, int lpSecurityAttributes, uint dwCreationDisposition, uint dwFlagsAndAttributes, int hTemplateFile);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr CreateEvent(IntPtr lpEventAttributes, bool bManualReset, bool bInitialState, string lpName);

        [DllImport("kernel32.dll")]
        private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetOverlappedResult(IntPtr hFile, ref OVERLAPPED lpOverlapped, ref uint lpNumberOfBytesTransferred, bool bWait);

        [DllImport("kernel32.dll")]
        private static extern uint GetLastError();

        [DllImport("kernel32.dll")]
        private static extern bool ResetEvent(IntPtr hEvent);

        private const uint FILE_FLAG_OVERLAPPED = 0x40000000;

        internal const uint GENERIC_READ = 0x80000000;
        internal const uint GENERIC_WRITE = 0x40000000;
        internal const uint OPEN_EXISTING = 3;

        internal const uint FILE_SHARE_READ = 0x00000001;
        internal const uint FILE_SHARE_WRITE = 0x00000002;

        private const uint INFINITE = 0xFFFFFFFF;

        private static string _deviceName = "\\\\.\\ShadowDriver";
        private IntPtr _deviceHandle = IntPtr.Zero;
        public void OpenDevice()
        {
            _deviceHandle = CreateFile(_deviceName, 0, IOCTLOperation.FILE_SHARE_READ | IOCTLOperation.FILE_SHARE_WRITE, 0, IOCTLOperation.OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
        }
        public async Task<uint> SendIOControlAsync(IOControlCode ioControlCode, byte[] inputBuffer, byte[] outputBuffer)
        {
            uint result = 0;

            await Task.Run(() =>
            {
                IntPtr eventHandle = CreateEvent(IntPtr.Zero, true, false, null);
                uint bytesReturn = 0;

                bool overlappedResult = false;
                OVERLAPPED overlapped = new()
                {
                    hEvent = eventHandle,
                };

                IntPtr inputBufferPointer = Marshal.AllocHGlobal(inputBuffer.Length);
                IntPtr outputBufferPointer = Marshal.AllocHGlobal(outputBuffer.Length);
                //Marshal.Copy(outputBuffer, 0, outputBufferPointer, outputBuffer.Length);

                Marshal.Copy(inputBuffer, 0, inputBufferPointer, inputBuffer.Length);
                try
                {
                    result = DeviceIoControl(_deviceHandle, ioControlCode.ControlCode, inputBufferPointer, inputBuffer.Length, outputBufferPointer, outputBuffer.Length, ref bytesReturn, ref overlapped);
                    overlappedResult = GetOverlappedResult(_deviceHandle, ref overlapped, ref bytesReturn, true);
                }
                catch (Exception exception)
                {
                    System.Diagnostics.Debug.WriteLine(exception);
                }

                if (overlappedResult)
                {
                    System.Diagnostics.Debug.WriteLine("overlappedResult true");

                    if (bytesReturn != 0)
                    {

                    }
                    //else
                    //{
                    //    var resultResult = ResetEvent(eventHandle);
                    //}
                }
                else
                {
                    var errorCode = GetLastError();

                    //if (errorCode == 996)
                    //{
                    //    var resultResult = ResetEvent(eventHandle);
                    //}
                    //else
                    //{
                    //    isIoCompleted = true;
                    //}

                    System.Diagnostics.Debug.WriteLine("overlappedResult false, error code {0}.", errorCode);
                }

                System.Diagnostics.Debug.WriteLine("IOCTL Done");
                //}
                //else
                //{
                //    System.Diagnostics.Debug.WriteLine("IOCTL Error");
                //}

                Marshal.Copy(outputBufferPointer, outputBuffer, 0, outputBuffer.Length);
                Marshal.FreeHGlobal(inputBufferPointer);
                Marshal.FreeHGlobal(outputBufferPointer);

                CloseHandle(eventHandle);
                System.Diagnostics.Debug.WriteLine("Output data length is {0}.", bytesReturn);
            }).ConfigureAwait(false);

            return (uint)result;
        }

        public bool IsOpened { get; private set; }
    }
}
