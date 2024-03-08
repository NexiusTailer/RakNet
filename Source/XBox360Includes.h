#if defined(_XBOX) || defined(X360)

#ifndef XBox360Includes_h__
#define XBox360Includes_h__

#include <Xtl.h>
#include <stdio.h>
#include "RakSleep.h"

inline void X360Startup(void)
{
	XNetStartupParams xnsp;
	memset(&xnsp,0,sizeof(xnsp));
	xnsp.cfgSizeOfStruct = sizeof(xnsp);
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
	int result = XNetStartup(&xnsp);
	if (result != 0)
	{
		printf("XNetStartup failed: %i\n", result);
	}
}

inline void X360Shutdown(void) {}

inline void GetMACAddress(unsigned int buff[6])
{
	XNADDR xna;
	while (XNetGetTitleXnAddr(&xna)==XNET_GET_XNADDR_PENDING)
		RakSleep(1);
	for (int i=0; i < 6; i++)
		buff[i]=xna.abEnet[i];
}

#endif // XBox360Includes_h__

#endif
