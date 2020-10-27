#pragma once
typedef struct ShaDrTcpRawHeader
{
	SHORT SourcePort;
	SHORT DestPort;
	UINT32 SequenceNumber;
	UINT32 AcknowLedgmentNumber;
	SHORT HeaderLengthAndFlags;
	SHORT WindowSize;
	SHORT CheckSum;
	SHORT UrgentPoint;
}ShaDrTcpRawHeader;
