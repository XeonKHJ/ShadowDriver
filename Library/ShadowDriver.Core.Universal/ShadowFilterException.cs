using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core
{
    public class ShadowFilterException : Exception
    {
        public ShadowFilterException(uint customizedNtstatus)
        {
            _ntStatus = customizedNtstatus;
        }

        private uint _ntStatus = 0;
        public override string Message
        {
            get
            {
                string message = "Unknown error!";
                switch((uint)_ntStatus)
                {
                    case 0xC0080001:
                        message = "The app is unregistered, access is not allowed.";
                        break;
                    case 0xC0080002:
                        message = "The app has been registered.";
                        break;
                    case 0xC0080003:
                        message = "The app dosn't provide any filtering condition.";
                        break;
                    case 0xC0080004:
                        message = "The appid is invalid.";
                        break;
                    case 0xC0090010:
                        message = "Filter is running, you need to stop filter before the operation.";
                        break;
                    case 0xC0090020:
                        message = "No filtering condition.";
                        break;
                    case 0xC0090030:
                        message = "The feature is not yet implemented.";
                        break;
                    case 0xC0090040:
                        message = "The filter is not ready.";
                        break;
                }

                return message;
            }
        }
    }
}
