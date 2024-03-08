#include <WinSock2.h>
#include <windows.h>
#include <Ws2tcpip.h>
#include <stdio.h>
void main_sockets(void)
{
	WSADATA winsockInfo;
	WSAStartup( MAKEWORD( 2, 2 ), &winsockInfo );
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in serverAddr;
	memset(&serverAddr,0,sizeof(sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = 0;
	int j = bind(sock,(struct sockaddr *) &serverAddr,sizeof(serverAddr));
	struct hostent * phe = gethostbyname( "masterserver2.raknet.com" );
	memcpy( &serverAddr.sin_addr.s_addr, phe->h_addr_list[ 0 ], sizeof( struct in_addr ) );
	serverAddr.sin_port        = htons(80);
	connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	const char *postRequest =
		"POST /testServer HTTP/1.1\r\n"
		"Content-Length: 83\r\n"
		"Content-Type: text/plain; charset=UTF-8\r\n"
		"Host: masterserver2.raknet.com\r\n"
		"Connection: Keep-Alive\r\n"
		"\r\n"
		"{'__gameId': 'myGame','__clientReqId': '0','__timeoutSec': '60','mapname': 'myMap'}\r\n";
	send(sock, postRequest, strlen(postRequest), 0);
	char outputBuffer[512];
	memset(outputBuffer,0,512);
	recv(sock, outputBuffer, 512, 0);
	printf(outputBuffer);
}

#include "TCPInterface.h"
#include "RakString.h"
#include "RakSleep.h"
using namespace RakNet;
void main_RakNet(void)
{
	TCPInterface *tcp = RakNet::OP_NEW<TCPInterface>(__FILE__,__LINE__);
	tcp->Start(0, 64);
	tcp->Connect("masterserver2.raknet.com", 80, true);
	RakString rspost = RakString::FormatForPOST(
		RakString("masterserver2.raknet.com/testServer"),
		RakString("text/plain; charset=UTF-8"),
		RakString("{'__gameId': 'myGame','__clientReqId': '0','__timeoutSec': '60','mapname': 'myMap'}"));
	RakSleep(100);
	SystemAddress serverAddr = tcp->HasCompletedConnectionAttempt();
	tcp->Send(rspost.C_String(), rspost.GetLength(), serverAddr, false);
	RakSleep(1000);
	Packet *p = tcp->Receive();
	if (p) printf((const char*) p->data);
}

void main(void)
{
//	main_sockets();
	main_RakNet();
}
