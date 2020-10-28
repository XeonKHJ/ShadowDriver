#pragma once

//定义相关结构体
typedef struct PacketDataBuffer
{
	void* CurrentBuffer;
	unsigned long long Bytes;
	struct PacketDataBuffer* NextBuffer;
	struct PacketDataBuffer* Previousfer;
}PacketDataBuffer;

typedef struct ShadowTcpRawPacket
{
	unsigned short SourcePort;
	unsigned short DestPort;
	unsigned __int32 SequenceNumber;
	unsigned __int32 AcknowLedgmentNumber;
	unsigned short  HeaderLengthAndFlags;
	unsigned short  WindowSize;
	unsigned short  CheckSum;
	unsigned short  UrgentPoint;
	PacketDataBuffer* Data;
}ShadowTcpRawPacket;

unsigned long long CaculateHexStringLength(unsigned long long bytesCount);
void ConvertBytesArrayToHexString(char* bytes, unsigned long long bytesCount, char* outputString, unsigned long long outputStringLength);
unsigned short CalculateCheckSum(char* bytes, char* fakeHeader, int byteCounts, int fakeHeaderCounts, int marginBytes);