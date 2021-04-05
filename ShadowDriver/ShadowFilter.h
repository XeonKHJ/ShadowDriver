#pragma once
#include "NetFilteringCondition.h"

class ShadowFilter
{
public:
	ShadowFilter(void* enviromentContexts);
	~ShadowFilter();
	int AddFilterCondition(NetFilteringCondition * conditions, int length);
	int StartFiltering();
	void EnablePacketModification();
	void DisablePacketModification();

	//过滤器接收到数据包的回调函数
	void (* NetPacketFilteringCallout)(NetLayer netLayer, NetPacketDirection direction, void * buffer, unsigned long long bufferSize);

private:
	bool _isModificationEnabled = false;
	void* _context;
	NetFilteringCondition* _filteringConditions;
	int _conditionCount;
};