using ShadowDriver.Core.Common;
using ShadowDriver.Core.Interface;
using System;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Windows
{
    public class ShadowFilter : IShadowFilter
    {
        public ShadowFilter(int appId, string appName)
        {
            
        }

        public event PacketReceivedEventHandler PacketReceived;
        public event FilterReadyEventHandler FilterReady;

        public Task AddConditionAsync(FilterCondition condition)
        {
            throw new NotImplementedException();
        }

        public void DeregisterApp()
        {
            throw new NotImplementedException();
        }

        public Task DeregisterAppAsync()
        {
            throw new NotImplementedException();
        }

        public Task RegisterAppAsync()
        {
            throw new NotImplementedException();
        }

        public Task StartFilteringAsync()
        {
            throw new NotImplementedException();
        }

        public void StartFilterWatcher()
        {
            throw new NotImplementedException();
        }

        public void StopFiltering()
        {
            throw new NotImplementedException();
        }

        public Task StopFilteringAsync()
        {
            throw new NotImplementedException();
        }

        public void SendIoctl()
        {

        }
    }
}
