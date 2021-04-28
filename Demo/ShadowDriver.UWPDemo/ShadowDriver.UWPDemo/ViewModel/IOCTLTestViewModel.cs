using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.UWPDemo.ViewModel
{
    public class IOCTLTestViewModel: INotifyPropertyChanged
    {
        private string _deviceConnectStatus;
        public string DeviceConnectStatus
        {
            set
            {
                _deviceConnectStatus = value;
                NotifyPropertyChanged();
            }
            get
            {
                return _deviceConnectStatus;
            }
        }

        private string _appRegisterStatus;
        public string AppRegisterStatus
        {
            set
            {
                _appRegisterStatus = value;
                NotifyPropertyChanged();
            }
            get
            {
                return _appRegisterStatus;
            }
        }

        private int _queuedIoctlCount = 0;
        public int QueuedIOCTLCount
        {
            set
            {
                _queuedIoctlCount = value;
                NotifyPropertyChanged();
            }
            get
            {
                return _queuedIoctlCount;
            }
        }
        private void NotifyPropertyChanged([CallerMemberName] String propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        public event PropertyChangedEventHandler PropertyChanged;
    }
}
