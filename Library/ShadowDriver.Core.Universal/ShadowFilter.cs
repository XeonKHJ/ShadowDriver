using ShadowDriver.Core.Common;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading.Tasks;
using Windows.Devices.Custom;
using Windows.Devices.Enumeration;
using ShadowDriver.Core.Interface;
using ShadowDriver.Core.Status;

namespace ShadowDriver.Core
{
    public class ShadowFilter : IShadowFilter
    {
        private AppRegisterContext _shadowRegisterContext;
        private CustomDevice _shadowDevice;
        public ShadowFilter(int appId, string appName)
        {
            _shadowRegisterContext = new AppRegisterContext()
            {
                AppId = appId,
                AppName = appName
            };
        }
        public int AppId
        {
            get
            {
                return _shadowRegisterContext.AppId;
            }
        }
        public bool IsFilterReady { private set; get; } = false;
        private void ShadowDriverDeviceWatcher_Removed(DeviceWatcher sender, DeviceInformationUpdate args)
        {
            _shadowDevice = null;
        }

        private async void ShadowDriverDeviceWatcher_Added(DeviceWatcher sender, DeviceInformation args)
        {
            System.Diagnostics.Debug.WriteLine(args.Id);
            sender.Stop();
            _shadowDevice = await CustomDevice.FromIdAsync(args.Id, DeviceAccessMode.ReadWrite, DeviceSharingMode.Shared);
            IsFilterReady = true;
            FilterReady?.Invoke();
        }

        public void StartFilterWatcher()
        {
            var selector = CustomDevice.GetDeviceSelector(DriverRelatedInformation.InterfaceGuid);
            var shadowDriverDeviceWatcher = DeviceInformation.CreateWatcher(
                selector,
                new string[] { "System.Devices.DeviceInstanceId" }
                );

            shadowDriverDeviceWatcher.Added += ShadowDriverDeviceWatcher_Added;
            shadowDriverDeviceWatcher.Removed += ShadowDriverDeviceWatcher_Removed; ;
            shadowDriverDeviceWatcher.Start();
        }
    
        public async Task<int> GetRegisterAppCount()
        {
            byte[] outputBuffer = new byte[2 * sizeof(int)];
            try
            {
                await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverGetRegisterdAppCount, null, outputBuffer.AsBuffer());
            }
            catch (NullReferenceException)
            {
                throw new ShadowFilterException(0xC0090040);
            }
            catch (Exception exception)
            {
                throw exception;
            }

            var status = BitConverter.ToUInt32(outputBuffer, 0);
            if (status != 0)
            {
                HandleError(status);
            }

            var count = BitConverter.ToInt32(outputBuffer, sizeof(int));
            return count;
        }
        public async Task RegisterAppToDeviceAsync()
        {
            var contextBytes = _shadowRegisterContext.SeralizeToByteArray();
            byte[] outputBuffer = new byte[sizeof(int)];

            try
            {
                await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverAppRegister, contextBytes.AsBuffer(), outputBuffer.AsBuffer());
            }
            catch (NullReferenceException)
            {
                throw new ShadowFilterException(0xC0090040);
            }
            catch (Exception exception)
            {
                throw exception;
            }

            var status = BitConverter.ToUInt32(outputBuffer, 0);
            if (status != 0)
            {
                HandleError(status);
            }
        }
        public async Task AddFilteringConditionAsync(FilterCondition condition)
        {
            byte[] encodedNetLayer = BitConverter.GetBytes((int)condition.FilteringLayer);
            byte[] encodedMatchType = BitConverter.GetBytes((int)condition.MatchType);
            byte[] encodedAddressLocation = BitConverter.GetBytes((int)condition.AddressLocation);
            byte[] encodedDirection = BitConverter.GetBytes((int)condition.PacketDirection);
            byte[] encodedAddressFamily = new byte[sizeof(int)];
            byte[] filteringAddressAndMask = new byte[32];
            switch (condition.FilteringLayer)
            {
                case FilteringLayer.LinkLayer:
                    condition.MacAddress.GetAddressBytes().CopyTo(filteringAddressAndMask, 0);
                    break;
                case FilteringLayer.NetworkLayer:
                    condition.IPAddress.GetAddressBytes().CopyTo(filteringAddressAndMask, 0);
                    switch(condition.IPAddress.AddressFamily)
                    {
                        case System.Net.Sockets.AddressFamily.InterNetwork:
                            if(BitConverter.IsLittleEndian)
                            {
                                Array.Reverse(filteringAddressAndMask, 0, 4);
                            }
                            condition.IPMask.GetAddressBytes().CopyTo(filteringAddressAndMask, 4);
                            encodedAddressFamily = BitConverter.GetBytes((int)IpAddrFamily.IPv4);
                            break;
                        case System.Net.Sockets.AddressFamily.InterNetworkV6:
                            condition.IPMask.GetAddressBytes().CopyTo(filteringAddressAndMask, 16);
                            encodedAddressFamily = BitConverter.GetBytes((int)IpAddrFamily.IPv4);
                            break;
                    }
                    break;
            }

            byte[] inputBuffer = new byte[6 * sizeof(int) + filteringAddressAndMask.Length];
            
            var contextBytes = _shadowRegisterContext.SeralizeAppIdToByteArray();

            var conditionBeginIndex = 0;
            contextBytes.CopyTo(inputBuffer, conditionBeginIndex);
            conditionBeginIndex += contextBytes.Length;
            encodedNetLayer.CopyTo(inputBuffer, conditionBeginIndex);
            conditionBeginIndex += encodedNetLayer.Length;
            encodedMatchType.CopyTo(inputBuffer, conditionBeginIndex);
            conditionBeginIndex += encodedMatchType.Length;
            encodedAddressLocation.CopyTo(inputBuffer, conditionBeginIndex);
            conditionBeginIndex += encodedAddressLocation.Length;
            encodedDirection.CopyTo(inputBuffer, conditionBeginIndex);
            conditionBeginIndex += encodedDirection.Length;
            encodedAddressFamily.CopyTo(inputBuffer, conditionBeginIndex);
            conditionBeginIndex += encodedAddressFamily.Length;
            filteringAddressAndMask.CopyTo(inputBuffer, conditionBeginIndex);

            byte[] outputBuffer = new byte[sizeof(int)];

            try
            {
                await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverAddCondition, inputBuffer.AsBuffer(), outputBuffer.AsBuffer());
            }
            catch (NullReferenceException)
            {
                throw new ShadowFilterException(0xC0090040);
            }
            catch (Exception exception)
            {
                throw exception;
            }

            var status = BitConverter.ToUInt32(outputBuffer, 0);
            if (status != 0)
            {
                throw new ShadowFilterException(status);
            }
        }
        public async Task StartFilteringAsync()
        {
            var outputBuffer = new byte[sizeof(int)];

            var inputBuffer = _shadowRegisterContext.SeralizeAppIdToByteArray();

            try
            {
                await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverStartFiltering, inputBuffer.AsBuffer(), outputBuffer.AsBuffer());
            }
            catch (NullReferenceException)
            {
                throw new ShadowFilterException(0xC0090040);
            }
            catch (Exception exception)
            {
                throw exception;
            }

            uint status = BitConverter.ToUInt32(outputBuffer, 0);

            if (status != 0)
            {
                throw new ShadowFilterException(status);
            }

            _isQueueingContinue = true;
            for (int i = 0; i < 20; ++i)
            {
                InqueueIOCTLForFurtherNotification();
            }

            _isFilteringStarted = true;
        }

        public async Task DeregisterAppFromDeviceAsync()
        {
            _isQueueingContinue = false;
            _isFilteringStarted = false;
            var contextBytes = _shadowRegisterContext.SeralizeAppIdToByteArray();
            byte[] outputBuffer = new byte[sizeof(int)];

            try
            {
                await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverAppDeregister, contextBytes.AsBuffer(), outputBuffer.AsBuffer());
            }
            catch (NullReferenceException)
            {
                throw new ShadowFilterException(0xC0090040);
            }
            catch (Exception exception)
            {
                throw exception;
            }

            var status = BitConverter.ToUInt32(outputBuffer, 0);
            if (status != 0)
            {
                HandleError(status);
            }
        }

        public async void DeregisterAppFromDevice()
        {
            _isQueueingContinue = false;
            _isFilteringStarted = false;
            var contextBytes = _shadowRegisterContext.SeralizeAppIdToByteArray();
            byte[] outputBuffer = new byte[sizeof(int)];

            if(_shadowDevice != null)
            {
                try
                {
                    await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverAppDeregister, contextBytes.AsBuffer(), outputBuffer.AsBuffer());
                }
                catch (Exception)
                {
                    
                }
            }
        }

        private bool _isQueueingContinue = false;
        private async void InqueueIOCTLForFurtherNotification()
        {
            var outputBuffer = new byte[2000];

            var inputBuffer = _shadowRegisterContext.SeralizeAppIdToByteArray();
            uint ioctlResult = 0;
            while(_isQueueingContinue)
            {
                try
                {
                    ioctlResult = await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverQueueNotification, inputBuffer.AsBuffer(), outputBuffer.AsBuffer());
                }
                catch(NullReferenceException)
                {
                    _isQueueingContinue = false;
                    //throw new ShadowFilterException(0xC0090040);
                }
                catch(Exception exception)
                {
                    _isQueueingContinue = false;
                    //System.Diagnostics.Debug.WriteLine(exception.Message);
                }

                uint status = BitConverter.ToUInt32(outputBuffer, 0);

                if (status != 0)
                {
                    _isQueueingContinue = false;
                    //if (status != 1)
                    //{
                    //    HandleError(status);
                    //}
                }

                var packetSize = BitConverter.ToInt64(outputBuffer, sizeof(int));
                byte[] packetBuffer = new byte[packetSize];
                Array.Copy(outputBuffer, sizeof(int) + sizeof(long), packetBuffer, 0, packetSize);

                if(_isFilteringStarted)
                {
                    PacketReceived?.Invoke(packetBuffer);
                }

            }
            return;
        }

        public async Task<int> CheckQueuedIOCTLCounts()
        {
            var outputBuffer = new byte[2 * sizeof(int)];
            var inputBuffer = _shadowRegisterContext.SeralizeAppIdToByteArray();

            try
            {
                await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverGetQueueInfo, inputBuffer.AsBuffer(), outputBuffer.AsBuffer());
            }
            catch (NullReferenceException)
            {
                throw new ShadowFilterException(0xC0090040);
            }
            catch (Exception exception)
            {
                throw exception;
            }

            var status = BitConverter.ToUInt32(outputBuffer, 0);
            if(status != 0)
            {
                HandleError(status);
            }
            var result = BitConverter.ToInt32(outputBuffer, sizeof(int));
            return result;
        }

        bool _isFilteringStarted = false;
        public async Task StopFilteringAsync()
        {
            _isFilteringStarted = false;
            var outputBuffer = new byte[sizeof(int)];
            _isQueueingContinue = false;
            var inputBuffer = _shadowRegisterContext.SeralizeAppIdToByteArray();

            try
            {
                await _shadowDevice.SendIOControlAsync(IOCTLs.IOCTLShadowDriverStopFiltering, inputBuffer.AsBuffer(), outputBuffer.AsBuffer());
            }
            catch (NullReferenceException)
            {
                throw new ShadowFilterException(0xC0090040);
            }
            catch (Exception exception)
            {
                throw exception;
            }

            uint status = BitConverter.ToUInt32(outputBuffer, 0);
            if (status != 0)
            {
                HandleError(status);
            }
        }

        private static void HandleError(uint errorCode)
        {
            if(errorCode != 0)
            {
                throw new ShadowFilterException(errorCode);
            }
        }
        public delegate void PacketReceivedEventHandler(byte[] buffer);
        public event PacketReceivedEventHandler PacketReceived;

        public delegate void FilterReadyEventHandler();
        public event FilterReadyEventHandler FilterReady;
    }
}
