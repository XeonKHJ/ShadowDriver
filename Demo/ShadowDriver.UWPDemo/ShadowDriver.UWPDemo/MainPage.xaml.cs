﻿using ShadowDriver.Core;
using ShadowDriver.Core.Common;
using ShadowDriver.Core.Status;
using ShadowDriver.UWPDemo.ViewModel;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Devices.Custom;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x804 上介绍了“空白页”项模板

namespace ShadowDriver.UWPDemo
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            var nics = NetworkInterface.GetAllNetworkInterfaces();
            foreach (var nic in nics)
            {
                NetworkInterfaceViewModels.Add(new NetworkInterfaceViewModel
                {
                    Id = nic.Id,
                    MacAddress = nic.GetPhysicalAddress().ToString(),
                    Name = nic.Name,
                    NetworkInterface = nic
                });
            }

            var view = ApplicationView.GetForCurrentView();
            view.Consolidated += View_Consolidated;

            _filter = new ShadowFilter(App.RandomAppIdGenerator.Next(), "ShadowDriver Demo");
            _filter.StartFilterWatcher();
            _filter.FilterReady += Filter_FilterReady;
            _filter.PacketReceived += Filter_PacketReceived;

            _dispatcherTimer = new DispatcherTimer()
            {
                Interval = new TimeSpan(0, 0, 3)
            };
            _dispatcherTimer.Tick += DispatcherTimer_Tick;

            System.Diagnostics.Debug.WriteLine(string.Format("Filter {0} loaded.", _filter.AppId));
        }

        private void View_Consolidated(ApplicationView sender, ApplicationViewConsolidatedEventArgs args)
        {
            System.Diagnostics.Debug.WriteLine(string.Format("Deregistering app id {0}", _filter.AppId));
            _filter.DeregisterApp();
        }

        private DispatcherTimer _dispatcherTimer;
        private async void Filter_FilterReady()
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                ViewModel.DeviceConnectStatus = "Connected";
            });
        }
        public ObservableCollection<NetworkInterfaceViewModel> NetworkInterfaceViewModels { get; } = new ObservableCollection<NetworkInterfaceViewModel>();
        private byte[] Filter_PacketReceived(byte[] buffer, CapturedPacketArgs args)
        {
            NetPacketViewModel netPacketViewModel = new NetPacketViewModel();
            for (int i = 0; i < buffer.Length; ++i)
            {
                netPacketViewModel.Content += buffer[i].ToString("X4") + " ";
            }
            NetPacketViewModels.Add(netPacketViewModel);

            byte[] modifiedBuffer = null;
            switch(args.Layer)
            {
                case FilteringLayer.NetworkLayer:
                    IPAddress address;
                    {
                        switch(args.Direction)
                        {
                            case NetPacketDirection.Out:
                                address = IPAddress.Parse("192.168.1.104"); 
                                address.GetAddressBytes().CopyTo(buffer, 16);

                                modifiedBuffer = buffer;
                                break;
                            case NetPacketDirection.In:
                                address = IPAddress.Parse("22.22.22.22");
                                address.GetAddressBytes().CopyTo(buffer, 12);
                                buffer[10] = 0;
                                buffer[11] = 0;
                                var checkSumCalculateBytes = (buffer[0] & 0xf) * 4;
                                var checkSum = CalculateChecksum(buffer, checkSumCalculateBytes);
                                buffer[10] = (byte)((checkSum & 0xff00) >> 8);
                                buffer[11] = (byte)(checkSum & 0xff);
                                modifiedBuffer = buffer;
                                break;
                        }
                    }
                    break;
            }


            return modifiedBuffer;
        }

        private ShadowFilter _filter;
        public IOCTLTestViewModel ViewModel { get; } = new IOCTLTestViewModel()
        {
            DeviceConnectStatus = "Disconnected",
            AppRegisterStatus = "Unchecked"
        };
        public CustomDevice ShadowDriverDevice { set; get; } = null;
        public ObservableCollection<NetPacketViewModel> NetPacketViewModels = new ObservableCollection<NetPacketViewModel>();
        private void ShadowDriverDeviceWatcher_Removed(Windows.Devices.Enumeration.DeviceWatcher sender, Windows.Devices.Enumeration.DeviceInformationUpdate args)
        {
            ShadowDriverDevice = null;
            ViewModel.DeviceConnectStatus = "Disconnected";
        }

        private async void ShadowDriverDeviceWatcher_Added(Windows.Devices.Enumeration.DeviceWatcher sender, Windows.Devices.Enumeration.DeviceInformation args)
        {
            ShadowDriverDevice = await CustomDevice.FromIdAsync(args.Id, DeviceAccessMode.ReadWrite, DeviceSharingMode.Exclusive);
            if (ShadowDriverDevice != null)
            {
                await this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    ViewModel.DeviceConnectStatus = "Connected";
                });
            }
        }

        private async void CheckQueuedIoctlCountButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var result = await _filter.CheckQueuedIOCTLCounts();
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    ViewModel.QueuedIOCTLCount = result;
                });
            }
            catch (ShadowFilterException exception)
            {
                DisplayException(exception);
            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }

        private async void StartFilterButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await _filter.StartFilteringAsync();
            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }

        public async void DisplayException(Exception exception)
        {
            DispatcherTimer_Tick(null, null);
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                ErrorMessageBlock.Text = exception.Message;
                ErrorMessageGrid.Visibility = Visibility.Visible;
            });
            _dispatcherTimer.Start();
        }

        private async void DispatcherTimer_Tick(object sender, object e)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                ErrorMessageGrid.Visibility = Visibility.Collapsed;
            });
            _dispatcherTimer.Stop();
        }

        private async void RegisterAppButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await _filter.RegisterAppAsync();
            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }

        private async void DeregisterAppButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await _filter.DeregisterAppAsync();
                NetPacketViewModels.Clear();
            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }

        private async void StopFilteringButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await _filter.StopFilteringAsync();
            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }
        static int countTest = 0;
        private async void AddConditionButton_Click(object sender, RoutedEventArgs e)
        {
            string layerString = (string)LayerBox.SelectedItem;
            string directionString = (string)DirectionBox.SelectedItem;
            string matchTypeString = (string)MatchTypeBox.SelectedItem;
            string locationString = (string)LocationBox.SelectedItem;
            FilterCondition filterCondition = new FilterCondition();
            switch (layerString)
            {
                case "Network Layer":
                    filterCondition.FilteringLayer = FilteringLayer.NetworkLayer;
                    filterCondition.IPAddress = IPAddress.Parse(IPAddressBox.Text);
                    filterCondition.IPMask = IPAddress.Parse(IPAddressMaskBox.Text);
                    break;
                case "Link Layer":
                    filterCondition.FilteringLayer = FilteringLayer.LinkLayer;
                        var networkInterface = ((NetworkInterfaceViewModel)MacAddressBox.SelectedItem).NetworkInterface;
                        var index = networkInterface.GetIPProperties().GetIPv4Properties().Index;
                        filterCondition.InterfaceIndex = (uint)index;

                    break;
            }
            switch (directionString)
            {
                case "In":
                    filterCondition.PacketDirection = NetPacketDirection.In;
                    break;
                case "Out":
                    filterCondition.PacketDirection = NetPacketDirection.Out;
                    break;
            }

            switch (locationString)
            {
                case "Remote":
                    filterCondition.AddressLocation = AddressLocation.Remote;
                    break;
                case "Local":
                    filterCondition.AddressLocation = AddressLocation.Local;
                    break;
            }

            switch (matchTypeString)
            {
                case "Equal":
                    filterCondition.MatchType = FilterMatchType.Equal;
                    break;
            }

            try
            {
                await _filter.AddConditionAsync(filterCondition);
            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }

        private void LayerBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch ((string)LayerBox.SelectedItem)
            {
                case "Link Layer":
                    if (MacAddressBox != null)
                    {
                        MacAddressPanel.Visibility = Visibility.Visible;
                    }
                    if (IPAddressBox != null)
                    {
                        IPAddressBox.Visibility = Visibility.Collapsed;
                    }

                    break;
                case "Network Layer":
                    if (MacAddressBox != null)
                    {
                        MacAddressPanel.Visibility = Visibility.Collapsed;
                    }
                    if (IPAddressBox != null)
                    {
                        IPAddressBox.Visibility = Visibility.Visible;
                    }
                    break;
            }
        }

        private async void GetAppCountButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var count = await _filter.GetRegisterAppCountAsync();
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    GetAppCountBlock.Text = count.ToString();
                });
            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
        }

        private async void EnableModificationButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await _filter.EnableModificationAsync();
            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }

        private ushort CalculateChecksum(byte[] buffer, int length)
        {
            UInt32 sum = 0;
            
            for(int i = 0; i < length; ++i)
            {
                if(i % 2 == 0)
                {
                    sum += (UInt32)((buffer[i]) << 8);
                }
                else
                {
                    sum += (UInt32)(buffer[i]);
                }

            }

            //for(int i = 0; i < buffer.Length; i+=2)
            //{
            //    UInt32 x = (UInt32)(buffer[i] << 8) + (UInt32)(buffer[i + 1]);

            //    sum += x;
            //}

            while (sum > 0xffff)
            {
                uint exceedPart = (sum & (~(0xFFFFu))) >> 16;
                uint remainPart = sum & 0xffffu;
                sum = remainPart + exceedPart;
            }

            UInt16 shortSum = (UInt16)~((UInt16)(sum & 0xFFFF));
            return shortSum;
        }
    }
}
