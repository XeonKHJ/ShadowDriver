MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Serial=0x6:FACILITY_SERIAL_ERROR_CODE
               Network=0x7:FACILITY_NETWORK_ERROR
               UserModeApp=0x8:FACILITY_USERMODEAPP_ERROR
               Filter=0x9:FACILITY_FILTER_ERROR
              )
LanguageNames =
    (
        English = 0x0409:Messages_ENU
        ChineseSimplified= 0x0804:Messages_CHN
    )


MessageId=0x0001 Facility=UserModeApp Severity=Error SymbolicName=SHADOW_APP_UNREGISTERED
Language=English
The app is unregistered, access is not allowed.
.
Language=ChineseSimplified
应用未注册。
.

MessageId=0x0002 Facility=UserModeApp Severity=Error SymbolicName=SHADOW_APP_REGISTERED
Language=English
The app has been registered.
.
Language=ChineseSimplified
该应用已经注册过了。
.

MessageId=0x0003 Facility=UserModeApp Severity=Error SymbolicName=SHADOW_APP_NO_CONDITION
Language=English
The app dosn't provide any filtering condition.
.
Language=ChineseSimplified
应用没有提供过滤条件。
.

MessageId=0x0004 Facility=UserModeApp Severity=Error SymbolicName=SHADOW_APP_APPID_INVALID
Language=English
The appid is invalid.
.
Language=ChineseSimplified
提供的AppID无效。
.

MessageId=0x0010 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_IS_RUNNING
Language=English
Filter is running, you need to stop filter before the operation.
.
Language=ChineseSimplified
驱动正在进行过滤，您必须先停止过滤才能进行操作。
.

MessageId=0x0020 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_NO_CONDITION
Language=English
No filtering condition.
.
Language=ChineseSimplified
过滤器中没有过滤条件。
.

MessageId=0x0030 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_NOT_IMPLEMENTED
Language=English
The feature is not yet implemented.
.
Language=ChineseSimplified
该功能还未实现。
.

MessageId=0x0040 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_NOT_READY
Language=English
The filter is not ready.
.
Language=ChineseSimplified
过滤器还未准备好。
.

MessageId=0x0050 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_MODIFICATION_HAS_BEEN_ENABLED
Language=English
Packet modfication has been enabled.
.
Language=ChineseSimplified
数据包修改已经被启用了。
.
