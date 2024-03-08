#pragma once

#include "RakString.h"

class DebugTools
{
public:
	DebugTools(void);
	~DebugTools(void);
	static void ShowError(RakNet::RakString errorString,bool pause, unsigned int lineNum,const char *fileName);
};
