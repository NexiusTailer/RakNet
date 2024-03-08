#include "EventReceiver.h"

EventReceiver::EventReceiver(int intThreadNum)
{
	finishedThreads=0;
	plannedThreadNumber=intThreadNum;
}

EventReceiver::EventReceiver()
{
	finishedThreads=0;
	plannedThreadNumber=3;
}

EventReceiver::~EventReceiver()
{
}
