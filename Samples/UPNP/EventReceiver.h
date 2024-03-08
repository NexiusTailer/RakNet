#pragma once
#include "UPNPCallBackInterface.h"

class EventReceiver :
	public UPNPCallBackInterface
{
public:
	EventReceiver(int plannedThreadNum);
	EventReceiver(void);
	~EventReceiver(void);

	void UPNPPrint(RakNet::RakString stringToPrint){printf("%s\n",stringToPrint.C_String());}
	void NoRouterFoundOnAnyInterface(int threadNumber,int internalPort,int portToOpenOnRouter){printf("No router found\n");}
	void NoPortOpenedOnAnyInterface(int threadNumber,int internalPort,int portToOpenOnRouter){printf("No port opened\n");}
	bool IsRunningAnyThread(){return finishedThreads<plannedThreadNumber;}
	void ThreadFinished(int threadNumber,int internalPort,int portToOpenOnRouter,bool success){finishedThreads++;}
private:
	int finishedThreads;
	int plannedThreadNumber;
};
