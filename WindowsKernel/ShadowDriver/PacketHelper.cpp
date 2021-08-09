#include "PacketHelper.h"
#include "IOCTLHelper.h"

void PacketHelper::FilterFunc(NetPacketDirection direction, void* buffer, unsigned long long bufferSize, void * context)
{
	UNREFERENCED_PARAMETER(direction);

	auto helper = (IOCTLHelper*)context;
	
	if (helper != nullptr)
	{
		char* outputBuffer = new char[bufferSize + sizeof(unsigned long long)];
		RtlCopyMemory(outputBuffer, &bufferSize, sizeof(unsigned long long));
		RtlCopyMemory(outputBuffer + sizeof(unsigned long long), buffer, bufferSize);
		helper->NotifyUserApp(outputBuffer, bufferSize + sizeof(unsigned long long));
		delete outputBuffer;
	}
}