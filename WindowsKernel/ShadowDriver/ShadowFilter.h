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
	void EnablePacketModification();
	void DisablePacketModification();
	
	//过滤器接收到数据包的回调函数
	void (* NetPacketFilteringCallout)(NetLayer netLayer, NetPacketDirection direction, void * buffer, unsigned long long bufferSize, void * context);

private:
	bool _isModificationEnabled = false;
	void* _context;
	FilterCondition* _filteringConditions;
	int _conditionCount;
};