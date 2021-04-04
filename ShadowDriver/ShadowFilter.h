#pragma once
#include "NetFilteringCondition.h"

class ShadowFilter
{
public:
	ShadowFilter(void* enviromentContexts);
	int AddFilterCondition(NetFilteringCondition * conditions, int length);
	int StartFiltering();
	void EnablePacketModification();
	void DisablePacketModification();
private:
	bool _isModificationEnabled = false;
	void* _context;
};

