#pragma once
#include "NetFilteringCondition.h"

class ShadowFilter
{
public:
	ShadowFilter(void* enviromentContexts);
	~ShadowFilter();
	unsigned int AddFilterConditions(FilterCondition * conditions, int length);
	unsigned int StartFiltering();
	unsigned int StopFiltering();
	unsigned int EnablePacketModification();
	void DisablePacketModification();
	void* GetContext();
	bool GetModificationStatus();
	static unsigned int InjectPacket(void* context, NetPacketDirection direction, NetLayer layer, void* buffer, unsigned long long size);
	static unsigned int InjectPacket(void* context, NetPacketDirection direction, NetLayer layer, void* buffer, unsigned long long size, unsigned long long identifier, int fragmentIndex);
	//过滤器接收到数据包的回调函数
	void (* NetPacketFilteringCallout)(NetLayer netLayer, NetPacketDirection direction, void * buffer, unsigned long long bufferSize, void * context);

private:
	bool _isModificationEnabled = false;
	void* _context;
	FilterCondition* _filteringConditions;
	int _conditionCount;
};