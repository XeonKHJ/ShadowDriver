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