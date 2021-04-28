#pragma once
#include "NetFilteringCondition.h"

class ShadowFilter
{
public:
	ShadowFilter(void* enviromentContexts);
	~ShadowFilter();
	unsigned int AddFilterConditions(NetFilteringCondition * conditions, int length);
	unsigned int StartFiltering();
	unsigned int StopFiltering();
	void EnablePacketModification();
	void DisablePacketModification();
	
	//���������յ����ݰ��Ļص�����
	void (* NetPacketFilteringCallout)(NetLayer netLayer, NetPacketDirection direction, void * buffer, unsigned long long bufferSize, void * context);

private:
	bool _isModificationEnabled = false;
	void* _context;
	NetFilteringCondition* _filteringConditions;
	int _conditionCount;
};