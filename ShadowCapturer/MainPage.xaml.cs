using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Devices.Custom;
using Windows.Devices.Enumeration;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x804 上介绍了“空白页”项模板

namespace ShadowCapturer
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            var selector = CustomDevice.GetDeviceSelector(InterfaceGuid);

            // Create a device watcher to look for instances of the fx2 device interface
            var m_Fx2Watcher = Windows.Devices.Enumeration.DeviceInformation.CreateWatcher(
                            selector,
                            new string[] { "System.Devices.DeviceInstanceId" }
                            );

            m_Fx2Watcher.Added += M_Fx2Watcher_Added;
            m_Fx2Watcher.Removed += M_Fx2Watcher_Removed; ;
            m_Fx2Watcher.Start();
        }
        static public Guid InterfaceGuid { get; } = new Guid("45f22bb7-6bc3-4545-96ed-73de89c46e7d");
        private void M_Fx2Watcher_Removed(DeviceWatcher sender, DeviceInformationUpdate args)
        {
            throw new NotImplementedException();
        }

        private void M_Fx2Watcher_Added(DeviceWatcher sender, DeviceInformation args)
        {
            System.Diagnostics.Debug.WriteLine(args.Id);
            SendIOCTL(args.Id);
        }

        public static IOControlCode IOCTLShadowDriverStartWfp = new IOControlCode(0x00000012, 0x909, IOControlAccessMode.ReadWrite, IOControlBufferingMethod.DirectInput);
        public static IOControlCode IOCTLShadowDriverRequirePacketInfo = new IOControlCode(0x00000012, 0x910, IOControlAccessMode.Any, IOControlBufferingMethod.DirectInput);
        public static IOControlCode IOCTLShadowDriverRequirePacketInfoShit = new IOControlCode(0x00000012, 0x911, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);
        async void SendIOCTL(string id)
        {
            var device = await CustomDevice.FromIdAsync(id, DeviceAccessMode.ReadWrite, DeviceSharingMode.Exclusive);
            var status = await device.SendIOControlAsync(IOCTLShadowDriverRequirePacketInfo, null, null);
        }
    }
}
