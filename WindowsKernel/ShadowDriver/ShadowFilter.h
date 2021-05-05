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
	//���������յ����ݰ��Ļص�����
	void (* NetPacketFilteringCallout)(NetLayer netLayer, NetPacketDirection direction, void * buffer, unsigned long long bufferSize, void * context);

private:
	bool _isModificationEnabled = false;
	void* _context;
	FilterCondition* _filteringConditions;
	int _conditionCount;
};