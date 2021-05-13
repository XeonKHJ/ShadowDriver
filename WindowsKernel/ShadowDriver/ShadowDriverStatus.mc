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
Ӧ��δע�ᡣ
.

MessageId=0x0002 Facility=UserModeApp Severity=Error SymbolicName=SHADOW_APP_REGISTERED
Language=English
The app has been registered.
.
Language=ChineseSimplified
��Ӧ���Ѿ�ע����ˡ�
.

MessageId=0x0003 Facility=UserModeApp Severity=Error SymbolicName=SHADOW_APP_NO_CONDITION
Language=English
The app dosn't provide any filtering condition.
.
Language=ChineseSimplified
Ӧ��û���ṩ����������
.

MessageId=0x0004 Facility=UserModeApp Severity=Error SymbolicName=SHADOW_APP_APPID_INVALID
Language=English
The appid is invalid.
.
Language=ChineseSimplified
�ṩ��AppID��Ч��
.

MessageId=0x0010 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_IS_RUNNING
Language=English
Filter is running, you need to stop filter before the operation.
.
Language=ChineseSimplified
�������ڽ��й��ˣ���������ֹͣ���˲��ܽ��в�����
.

MessageId=0x0020 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_NO_CONDITION
Language=English
No filtering condition.
.
Language=ChineseSimplified
��������û�й���������
.

MessageId=0x0030 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_NOT_IMPLEMENTED
Language=English
The feature is not yet implemented.
.
Language=ChineseSimplified
�ù��ܻ�δʵ�֡�
.

MessageId=0x0040 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_NOT_READY
Language=English
The filter is not ready.
.
Language=ChineseSimplified
��������δ׼���á�
.

MessageId=0x0050 Facility=Filter Severity=Error SymbolicName=SHADOW_FILTER_MODIFICATION_HAS_BEEN_ENABLED
Language=English
Packet modfication has been enabled.
.
Language=ChineseSimplified
���ݰ��޸��Ѿ��������ˡ�
.
