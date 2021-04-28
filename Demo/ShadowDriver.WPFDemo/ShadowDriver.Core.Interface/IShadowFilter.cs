using System;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Interface
{
    public interface IShadowFilter
    {
        public Task StartFilteringAsync();
    }
}
