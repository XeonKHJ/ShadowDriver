#include "ShadowUtilities.h"

size_t CaculateHexStringLength(size_t bytesCount)
{
	//"0A:0B:0C"
	//"123123123
	return bytesCount * 3;
}

void ConvertBytesArrayToHexString(char* bytes, size_t bytesLength, char* outputString, size_t ouputLength)
{
	size_t hexLength = 2 * sizeof(char);
	size_t seprateLength = 1 * sizeof(char);
	size_t endSymbolLength = 1 * sizeof(char);
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
unsigned short CalculateCheckSum(char* bytes, char* fakeHeader, int byteCounts, int fakeHeaderCounts)
{
	unsigned int sum = 0;

	for (int i = 0; i < fakeHeaderCounts; i += 2)
	{
		unsigned int perSum = (unsigned int)(fakeHeader[i + 1] & 0xff) + (((unsigned int)((fakeHeader[i])) << 8) & 0xff00);
		sum += perSum;
	}

	for (int i = 0; i < byteCounts; i += 2)
	{
		unsigned int perSum = (unsigned int)(bytes[i + 1] & 0xff) + (((unsigned int)((bytes[i])) << 8) & 0xff00);
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