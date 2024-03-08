#include "RakTimer.h"

RakTimer::RakTimer(void)
{
	timerLength=1000;
	Start();
}

RakTimer::RakTimer(int lengthInMilliseconds)
{
	timerLength=lengthInMilliseconds;
	Start();
}

RakTimer::~RakTimer(void)
{
}

    void RakTimer::SetTimerLength(int lengthInMilliseconds)
	{
	timerLength=lengthInMilliseconds;
	}
	void RakTimer::Start()
	{
		startTime=RakNet::GetTime();

	}
    void RakTimer::Pause()
	{
	pauseOffset=(int)(RakNet::GetTime()-startTime);
	}
	void RakTimer::Resume()
	{
	startTime=RakNet::GetTime()-pauseOffset;
	}

	bool RakTimer::IsExpired()
	{

	return (RakNet::GetTime()-startTime>timerLength);
	
	}
