#include "RakNetSocket.h"
#include "RakMemoryOverride.h"

using namespace RakNet;

#if defined(__native_client__)
using namespace pp;
#endif

RakNetSocket::RakNetSocket() {
	#if !defined(WINDOWS_STORE_RT)
		s = 0;
	#endif
	remotePortRakNetWasStartedOn_PS3_PSP2 = 0;
	userConnectionSocketIndex = (unsigned int) -1;
	socketFamily = 0;
	blockingSocket = 0;
	extraSocketOptions = 0;
	chromeInstance = 0;

#if defined (_WIN32) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
	recvEvent=INVALID_HANDLE_VALUE;
#endif

	#ifdef __native_client__
		s = 0;
		sendInProgress = false;
		nextSendSize = 0;
	#endif
}
RakNetSocket::~RakNetSocket() 
{
	#ifdef __native_client__
		if(s != 0)
			((PPB_UDPSocket_Private_0_3*) pp::Module::Get()->GetBrowserInterface(PPB_UDPSOCKET_PRIVATE_INTERFACE_0_3))->Close(s);
#elif defined(WINDOWS_STORE_RT)
		WinRTClose(s);
	#else
		if ((__UDPSOCKET__)s != 0)
			closesocket__(s);
	#endif
	

#if defined (_WIN32) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
	if (recvEvent!=INVALID_HANDLE_VALUE)
	{
		CloseHandle( recvEvent );
		recvEvent = INVALID_HANDLE_VALUE;
	}
#endif
}
// 
// void RakNetSocket::Accept(
// struct sockaddr *addr,
// 	int *addrlen)
// {
// 	accept__(s, addr, addrlen);
// }

// 
// void RakNetSocket::Close( void )
// {
// 	closesocket__(s);
// }

RakNetSocket* RakNetSocket::Create
#ifdef __native_client__
	(_PP_Instance_ _chromeInstance)
#else
	(int af,
	int type,
	int protocol)
#endif
{
	__UDPSOCKET__ sock;

	#ifndef __native_client__
		RakAssert(type==SOCK_DGRAM);
	#endif

	#ifdef __native_client__
		sock = ((PPB_UDPSocket_Private_0_3*) Module::Get()->GetBrowserInterface(PPB_UDPSOCKET_PRIVATE_INTERFACE_0_3))->Create(_chromeInstance);


	#elif defined(WINDOWS_STORE_RT)
		sock = WinRTCreateDatagramSocket(af,type,protocol);


	#else
		sock = socket__(af, type, protocol);
	#endif

	#if !defined(WINDOWS_STORE_RT)
	if (sock<0)
		return 0;
	#endif
	RakNetSocket *rns = RakNet::OP_NEW<RakNetSocket>(_FILE_AND_LINE_);
	rns->s = sock;
	#ifdef __native_client__
		rns->chromeInstance = _chromeInstance;
	#endif
	return rns;
}

int RakNetSocket::Bind(		
		  const struct sockaddr *addr,
		  int namelen)
{
	return bind__(s,addr,namelen);
}

int RakNetSocket::IOCTLSocket(		
				 long cmd,
				 unsigned long *argp)
{
	#if defined(_WIN32)
		return ioctlsocket__(s,cmd,argp);
	#else
		return 0;
	#endif
}

int RakNetSocket::Listen (		
			 int backlog)
{
	return listen__(s,backlog);
}

int RakNetSocket::SetSockOpt(		
				int level,
				int optname,
				const char * optval,
				int optlen)
{
	return setsockopt__(s,level,optname,optval,optlen);
}

int RakNetSocket::Shutdown(		
			  int how)
{

		return shutdown__(s,how);



}
