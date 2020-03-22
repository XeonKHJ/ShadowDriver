#include "ShadowUtilities.h"

void ConvertBytesArrayToHexString(char* bytes, int bytesCount, char* outputString, int outputStringLength)
{
    int i;
    char* buf2 = outputString;
    char* endofbuf = outputString + sizeof(outputString);
    for (i = 0; i < bytesCount; i++)
    {
        /* i use 5 here since we are going to add at most
           3 chars, need a space for the end '\n' and need
           a null terminator */
        if (buf2 + 5 < endofbuf)
        {
            if (i > 0)
            {
                buf2 += sprintf(buf2, ":");
            }
            buf2 += sprintf(buf2, "%02X", outputString[i]);
        }
    }
    buf2 += sprintf(buf2, "\n");
}