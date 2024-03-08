#if defined(_PS3) || defined(__PS3__) || defined(SN_TARGET_PS3)


#ifndef __PS3_INCLUDES_H
#define __PS3_INCLUDES_H

//#include <stdint.h> // GetTime.cpp
#include <sys/time_util.h> // GetTime.cpp
#include <sys/sys_time.h> // GetTime.cpp
#include <sys/timer.h> // RakSleep.cpp
#include <netex/libnetctl.h> // SocketLayer::GetMyIP
#include <netex/errno.h>
#include <netex/net.h>
#include <netinet/in.h> // htonl
#include <cell/sysmodule.h>
#include <sys/paths.h>

#include <netdb.h>
//#include <sys/ppu_thread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netex/net.h>
#include <netex/errno.h>
#include <netex/libnetctl.h>
#include <np/common.h>
#include <alloca.h>
#include "sys/time.h"
#define INVALID_SOCKET -1

#define _PS3_usleep sys_timer_usleep
#define _PS3_GetTicksPerSecond sys_time_get_timebase_frequency
#define _PS3_GetElapsedTicks SYS_TIMEBASE_GET
#define _PS3_LoadFmodLibraries cellSysmoduleLoadModule(CELL_SYSMODULE_AUDIO); \
	cellSysmoduleLoadModule(CELL_SYSMODULE_IO);
#define _PS3_UnloadFmodLibraries cellSysmoduleLoadModule(CELL_SYSMODULE_AUDIO); \
	cellSysmoduleLoadModule(CELL_SYSMODULE_IO);

#define _PS3_InitFmod \
	FMOD_PS3_EXTRADRIVERDATA extradriverdata;\
	memset(&extradriverdata, 0, sizeof(FMOD_PS3_EXTRADRIVERDATA));\
	extradriverdata.spurs                               = 0; /* Not using SPURS */\
	extradriverdata.spu_mixer_elfname_or_spursdata      = SYS_APP_HOME"/fmodex_spu.self";\
	extradriverdata.spu_streamer_elfname_or_spursdata   = SYS_APP_HOME"/fmodex_spu_mpeg.self";\
	extradriverdata.spu_priority_mixer                  = 16;  /* Default */\
	extradriverdata.spu_priority_streamer               = 200; /* Default */\
	extradriverdata.spu_priority_at3                    = 200; /* Default */\
	extradriverdata.force5point1                        = 0; /* NO LONGER REQUIRED, set to 0 */\
	extradriverdata.attenuateDDLFE                      = 0; /* NO LONGER REQUIRED, set to 0 */\
	fmodSystem->init(100, FMOD_INIT_NORMAL, (void *)&extradriverdata);

#include <string.h>
inline void GetMACAddress(unsigned int buff[6])
{
	CellNetCtlInfo info;
	memset(&info,0,sizeof(info));
	int ret;
	ret=cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
	ret=cellSysmoduleLoadModule(CELL_SYSMODULE_NETCTL);
	ret=sys_net_initialize_network();
	ret=cellNetCtlInit();
	ret=cellNetCtlGetInfo(CELL_NET_CTL_INFO_ETHER_ADDR, &info);
	if (ret==0)
	{
		for (int i=0; i < 6; i++)
		{
			buff[i]=info.ether_addr.data[i];
		}
	}
}

#endif

#endif