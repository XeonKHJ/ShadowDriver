#include "ShadowUtilities.h"

unsigned long long CaculateHexStringLength(unsigned long long bytesCount)
{
	//"0A:0B:0C"
	//"123123123
	return bytesCount * 3;
}

void ConvertBytesArrayToHexString(char* bytes, unsigned long long bytesLength, char* outputString, unsigned long long ouputLength)
{
	unsigned long long hexLength = 2 * sizeof(char);
	unsigned long long seprateLength = 1 * sizeof(char);
	unsigned long long endSymbolLength = 1 * sizeof(char);
	char* currentBytePos = bytes;
	char* currentOutputPos = outputString;
	for (int i = 0;
		i < bytesLength &&
		(currentOutputPos + hexLength + seprateLength + endSymbolLength <= outputString + ouputLength * sizeof(char));
		++i)
	{
		if (i > 0)
		{
			*(currentOutputPos++) = ':';
		}
		char byteToConvert = bytes[i];
		char higherBits = (byteToConvert >> 4) & 0xF;
		char lowerBits = byteToConvert & 0xF;

		if (higherBits >= 0xA)
		{
			higherBits = higherBits + 55;
		}
		else
		{
			higherBits = higherBits + 48;
		}
		*(currentOutputPos++) = higherBits;

		if (lowerBits >= 0xA)
		{
			lowerBits += 55;
		}
		else
		{
			lowerBits += 48;
		}
		*(currentOutputPos++) = lowerBits;
	}

	*currentOutputPos = 0;
}

/// <summary>
/// 计算TCP/IP校验和
/// </summary>
/// <param name="bytes">要计算校验和的缓冲区指针，记得把校验和所在的位置置零再传进来。</param>
/// <param name="byteCounts">缓冲区大小（字节）</param>
/// <returns>校验和</returns>
unsigned short CalculateCheckSum(char* bytes, char* fakeHeader, int byteCounts, int fakeHeaderCounts, int marginBytes)
{
	unsigned int sum = 0;
	int paddings = byteCounts % marginBytes;

	for (int i = 0; i < fakeHeaderCounts; i += 2)
	{
		unsigned int perSum = (unsigned int)(fakeHeader[i + 1] & 0xff) + (((unsigned int)((fakeHeader[i])) << 8) & 0xff00);
		sum += perSum;
	}

	for (int i = 0; i < (byteCounts + paddings); i += 2)
	{
		unsigned int perSum = 0;

		if (i < byteCounts)
		{
			perSum += (((unsigned int)((bytes[i])) << 8) & 0xff00);
		}
		else
		{
			perSum += 0;
		}

		if (i + 1 < byteCounts)
		{
			perSum += (unsigned int)(bytes[i + 1] & 0xff);
		}
		else
		{
			perSum += 0;
		}

		sum += perSum;
	}

	while (sum > 0xffff)
	{
		unsigned int exceedPart = (sum & (~0xFFFF)) >> 16;
		unsigned int remainPart = sum & 0xffff;
		sum = remainPart + exceedPart;
	}

	return ~(sum & 0xFFFF);
}

unsigned short CalculateCheckSum1(ShadowTcpRawPacket* packet, char* fakeHeader, unsigned long long fakeHeaderCounts, int marginBytes)
{
	if (marginBytes < 0)
	{
		return 0;
	}

	unsigned long long byteCounts = 0;
	unsigned int sum = 0;

	//计算总字节长
	for (PacketDataBuffer* currentBuffer = packet->Data; currentBuffer; currentBuffer = currentBuffer->NextBuffer)
	{
		byteCounts += currentBuffer->Bytes;
	}

	int paddings = byteCounts % marginBytes;

	for (int i = 0; i < fakeHeaderCounts; i += 2)
	{
		unsigned int perSum = (unsigned int)(fakeHeader[i + 1] & 0xff) + (((unsigned int)((fakeHeader[i])) << 8) & 0xff00);
		sum += perSum;
	}

	int totalOffset = 0;

	for (PacketDataBuffer* currentBuffer = packet->Data; currentBuffer; currentBuffer = currentBuffer->NextBuffer)
	{
		char* currentDataBuffer = (char*)(currentBuffer->CurrentBuffer);

		for (int i = 0; i < currentBuffer->Bytes; ++i, ++totalOffset)
		{
			if (totalOffset < byteCounts)
			{
				//位移为偶数，则要向左位移两位。
				if (totalOffset % 2)
				{
					sum += (((unsigned int)((currentDataBuffer[i])) << 8) & 0xff00);
				}
				else
				{
					sum += (unsigned int)(currentDataBuffer[i] & 0xff);
				}
			}
		}
	}


	while (sum > 0xffff)
	{
		unsigned int exceedPart = (sum & (~0xFFFF)) >> 16;
		unsigned int remainPart = sum & 0xffff;
		sum = remainPart + exceedPart;
	}

	return ~(sum & 0xFFFF);
}