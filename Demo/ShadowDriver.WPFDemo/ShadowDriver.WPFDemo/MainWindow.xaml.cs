using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using ShadowDriver.Core;
using ShadowDriver.Core.Common;
using ShadowDriver.Core.Interface;

namespace ShadowDriver.WPFDemo
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
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

            _filter = new ShadowFilter(App.RandomAppIdGenerator.Next(), "ShadowDriverWPFDemo");
            _filter.StartFilterWatcher();
            _filter.FilterReady += Filter_FilterReady;
            _filter.PacketReceived += Filter_PacketReceived;

            _dispatcherTimer = new DispatcherTimer()
            {
                Interval = new TimeSpan(0, 0, 3)
            };
            _dispatcherTimer.Tick += DispatcherTimer_Tick;

            MacAddressBox.ItemsSource = NetworkInterfaceViewModels;
            LayerBox.ItemsSource = Layers;
            PacketsView.ItemsSource = NetPacketViewModels;
            DirectionBox.ItemsSource = Directions;
            MatchTypeBox.ItemsSource = MatchTypes;
            LocationBox.ItemsSource = Locations;
            System.Diagnostics.Debug.WriteLine(string.Format("Filter {0} loaded.", _filter.AppId));
        }

        private DispatcherTimer _dispatcherTimer;
        private void Filter_FilterReady()
        {
                ViewModel.DeviceConnectStatus = "Connected";
        }
        public ObservableCollection<NetworkInterfaceViewModel> NetworkInterfaceViewModels { get; } = new ObservableCollection<NetworkInterfaceViewModel>();
        public List<string> Directions { get; } = new List<string> { "In", "Out" };
        public List<string> MatchTypes { get; } = new List<string> { "Equal" };
        public List<string> Locations { get; } = new List<string> { "Local", "Remote" };
        private void Filter_PacketReceived(byte[] buffer)
        {
            NetPacketViewModel netPacketViewModel = new NetPacketViewModel();
            for (int i = 0; i < buffer.Length; ++i)
            {
                netPacketViewModel.Content += buffer[i].ToString("X4") + " ";
            }
            NetPacketViewModels.Add(netPacketViewModel);
        }

        private ShadowFilter _filter;

        public List<string> Layers { get; } = new List<string>{ "Network Layer", "Link Layer"};
        public FiltreStatusViewModel ViewModel { get; } = new FiltreStatusViewModel()
        {
            DeviceConnectStatus = "Disconnected",
            AppRegisterStatus = "Unchecked"
        };
        //public CustomDevice ShadowDriverDevice { set; get; } = null;
        public ObservableCollection<NetPacketViewModel> NetPacketViewModels = new ObservableCollection<NetPacketViewModel>();
        private async void CheckQueuedIoctlCountButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var result = await _filter.CheckQueuedIOCTLCounts();

                ViewModel.QueuedIOCTLCount = result;
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

                ErrorMessageBlock.Text = exception.Message;
                ErrorMessageGrid.Visibility = Visibility.Visible;
           
            _dispatcherTimer.Start();
        }

        private void DispatcherTimer_Tick(object sender, object e)
        {
            ErrorMessageGrid.Visibility = Visibility.Collapsed;
            _dispatcherTimer.Stop();
        }

        private async void RegisterAppButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await _filter.RegisterAppToDeviceAsync();
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
                await _filter.DeregisterAppFromDeviceAsync();
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
                    filterCondition.MacAddress = ((NetworkInterfaceViewModel)MacAddressBox.SelectedItem).NetworkInterface.GetPhysicalAddress();
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
                await _filter.AddFilteringConditionAsync(filterCondition);
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
                        MacAddressBox.Visibility = Visibility.Visible;
                    }
                    if (IPAddressBox != null)
                    {
                        IPAddressBox.Visibility = Visibility.Collapsed;
                    }

                    break;
                case "Network Layer":
                    if (MacAddressBox != null)
                    {
                        MacAddressBox.Visibility = Visibility.Collapsed;
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
                var count = await _filter.GetRegisterAppCount();

                    GetAppCountBlock.Text = count.ToString();

            }
            catch (Exception exception)
            {
                DisplayException(exception);
            }
        }
    }
}
