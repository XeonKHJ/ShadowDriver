#pragma once
#include <ntstatus.h>
#include <ntstrsafe.h>

void ConvertBytesArrayToHexString(char* bytes, size_t bytesCount, char* outputString, size_t outputStringLength);