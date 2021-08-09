#pragma once
#include "NetFilteringCondition.h"

class ShadowFilter
{
public:
	~ShadowFilter();
	unsigned int AddFilterConditions(FilterCondition * conditions, int length);
	static unsigned int StartFiltering(void* enviromentContexts);
	static unsigned int StopFiltering();
	bool GetModificationStatus();
	static unsigned int InjectPacket(void* context, NetPacketDirection direction, void* buffer, unsigned long long size);
	static unsigned int InjectPacket(void* context, NetPacketDirection direction, void* buffer, unsigned long long size, unsigned long long identifier, int fragmentIndex);
	//���������յ����ݰ��Ļص�����
	void (* NetPacketFilteringCallout)(NetPacketDirection direction, void * buffer, unsigned long long bufferSize, void * context);

private:
	bool _isModificationEnabled = false;
	void* _context;
	FilterCondition* _filteringConditions;
	int _conditionCount;
};