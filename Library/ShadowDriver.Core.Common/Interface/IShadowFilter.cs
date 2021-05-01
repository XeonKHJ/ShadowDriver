using ShadowDriver.Core.Common;
using System;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Interface
{
    public interface IShadowFilter
    {
        void StartFilterWatcher();
        Task AddConditionAsync(FilterCondition condition);
        Task RegisterAppAsync();
        Task StartFilteringAsync();
        Task StopFilteringAsync();
        void StopFiltering();
        Task DeregisterAppAsync();
        void DeregisterApp();

        event PacketReceivedEventHandler PacketReceived;
        event FilterReadyEventHandler FilterReady;
    }
}
