﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowDriver.Core.Windows
{
    internal static class IOCTLs
    {
        //#define IOCTL_SHADOWDRIVER_START_WFP                    CTL_CODE(FILE_DEVICE_NETWORK, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS)
        //开始过滤用的IOCTL
        public static IOControlCode IOCTLShadowDriverStartFiltering = new IOControlCode(0x00000012, 0x909, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_STOP_FILTERING               CTL_CODE(FILE_DEVICE_NETWORK, 0x912, METHOD_BUFFERED, FILE_ANY_ACCESS)
        //停止过滤用的IOCTL
        public static IOControlCode IOCTLShadowDriverStopFiltering = new IOControlCode(0x00000012, 0x912, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        public static IOControlCode IOCTLShadowDriverRequirePacketInfo = new IOControlCode(0x00000012, 0x910, IOControlAccessMode.Any, IOControlBufferingMethod.DirectInput);

        public static IOControlCode IOCTLShadowDriverGetDriverVersion = new IOControlCode(0x00000012, 0x922, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        public static IOControlCode IOCTLShadowDriverDequeueNotification = new IOControlCode(0x00000012, 0x911, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        public static IOControlCode IOCTLShadowDriverQueueNotification = new IOControlCode(0x00000012, 0x921, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_APP_REGISTER					CTL_CODE(FILE_DEVICE_NETWORK, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverAppRegister = new IOControlCode(0x00000012, 0x901, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_APP_DEREGISTER                 CTL_CODE(FILE_DEVICE_NETWORK, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverAppDeregister = new IOControlCode(0x00000012, 0x902, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_GET_QUEUE_INFO               CTL_CODE(FILE_DEVICE_NETWORK, 0x923, METHOD_BUFFERED, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverGetQueueInfo = new IOControlCode(0x00000012, 0x923, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_GET_LOCAL_STATUS             CTL_CODE(FILE_DEVICE_NETWORK, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverGetLocalStatus = new IOControlCode(0x00000012, 0x903, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_ADD_CONDITION                CTL_CODE(FILE_DEVICE_NETWORK, 0x924, METHOD_BUFFERED, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverAddCondition = new IOControlCode(0x00000012, 0x924, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_GET_REGISTERED_APP_COUNT     CTL_CODE(FILE_DEVICE_NETWORK, 0x925, METHOD_BUFFERED, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverGetRegisterdAppCount = new IOControlCode(0x00000012, 0x925, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_ENABLE_MODIFICATION          CTL_CODE(FILE_DEVICE_NETWORK, 0x926, METHOD_BUFFERED, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverEnableModification = new IOControlCode(0x00000012, 0x926, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_INJECT_PACKET                CTL_CODE(FILE_DEVICE_NETWORK, 0X927, METHOD_BUFFERED, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverInjectPacket = new IOControlCode(0x00000012, 0x927, IOControlAccessMode.Any, IOControlBufferingMethod.Buffered);

        //#define IOCTL_SHADOWDRIVER_DIRECT_IN_TEST               CTL_CODE(FILE_DEVICE_NETWORK, 0X828, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverDirectInTest = new IOControlCode(0x00000012, 0X828, IOControlAccessMode.Any, IOControlBufferingMethod.DirectInput);

        //#define IOCTL_SHADOWDRIVER_DIRECT_OUT_TEST              CTL_CODE(FILE_DEVICE_NETWORK, 0X829, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverDirectOutTest = new IOControlCode(0x00000012, 0X829, IOControlAccessMode.Any, IOControlBufferingMethod.DirectOutput);

        //#define IOCTL_SHADOWDRIVER_NEITHER_IO_TEST              CTL_CODE(FILE_DEVICE_NETWORK, 0X830, METHOD_NEITHER, FILE_ANY_ACCESS)
        public static IOControlCode IOCTLShadowDriverNeitherTest = new IOControlCode(0x00000012, 0X830, IOControlAccessMode.Any, IOControlBufferingMethod.Neither);
    }
}
