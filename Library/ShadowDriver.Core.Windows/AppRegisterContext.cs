using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Storage.Streams;
using ShadowDriver.Core.Interface;

namespace ShadowDriver.Core
{
    public class AppRegisterContext : IAppRegisterContext
    {
        public int AppId { set; get; }

        private string _appName;
        public string AppName
        {
            set
            {
                if (value != null)
                {
                    if (value.Length > 20)
                    {
                        throw new Exception("App name too long");
                    }
                    _appName = value;
                }
            }
            get
            {
                return _appName;
            }
        }
        internal Guid SublayerKey { set; get; } = Guid.NewGuid();
        internal SortedDictionary<int, Guid> CalloutsKey { set; get; } = new SortedDictionary<int, Guid>
        { 
            { 0, Guid.NewGuid()}, // 网络层IPv4发送通道回调
            { 1, Guid.NewGuid() }, // 链路层发送通道
            { 2, Guid.NewGuid() }, // 网络层IPv4接收通道
            { 3, Guid.NewGuid() }, // 链路层接收通道
            { 4, Guid.NewGuid() }, // 网络层IPv6发送通道
            { 6, Guid.NewGuid() }  // 网络层IPv6接收通道
        };

        /// 要注意到一般X86是小端机，ARM是大端机
        /// </summary>
        /// <param name="context"></param>
        /// <param name="stream"></param>
        /// <returns></returns>
        internal byte[] SeralizeToByteArray()
        {
            // appid + 40 byte of name + 16 byte of sublayer key + 16 byte of callout key
            byte[] result = new byte[DriverRelatedInformation.AppRegisterContextMaxSize + 16 + 16 * CalloutsKey.Count];
            byte[] intBytes = BitConverter.GetBytes(AppId);
            byte[] nameBytes = Encoding.Unicode.GetBytes(AppName);
            byte[] sublayerKeyBytes = SublayerKey.ToByteArray();
            intBytes.CopyTo(result, 0);
            nameBytes.CopyTo(result, sizeof(int));
            sublayerKeyBytes.CopyTo(result, DriverRelatedInformation.AppRegisterContextMaxSize);
            var currentIndex = DriverRelatedInformation.AppRegisterContextMaxSize + 16;
            foreach (var guid in CalloutsKey.Values)
            {
                var guidBytes = guid.ToByteArray();
                guidBytes.CopyTo(result, currentIndex);
                currentIndex += 16;
            }

            return result;
        }

        internal byte[] SeralizeAppIdToByteArray()
        {
            byte[] intBytes = BitConverter.GetBytes(AppId);
            return intBytes;
        }
    }
}
