// Common includes
#include <stdio.h>
#include <stdlib.h>
#include "Kbhit.h"
#include "RakNetworkFactory.h"
#include "GetTime.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "StringCompressor.h"
#include "FileListTransfer.h"
#include "FileList.h" // FLP_Printf
#include "AutopatcherServer.h"
#include "AutopatcherMySQLRepository.h"
#include "PacketizedTCP.h"

#ifdef _WIN32
#include "WindowsIncludes.h" // Sleep
#else
#include <unistd.h> // usleep
#endif

#define USE_TCP
#define LISTEN_PORT 60000
#define MAX_INCOMING_CONNECTIONS 8

int main(int argc, char **argv)
{
	// Avoids the Error: Got a packet bigger than 'max_allowed_packet' bytes
	printf("Important: Requires that you first set the DB schema and the max packet size on the server.\n");
	printf("See DependentExtensions/AutopatcherMySQLRepository/readme.txt\n");

	printf("Server starting... ");
	AutopatcherServer autopatcherServer;
	FLP_Printf progressIndicator;
	FileListTransfer fileListTransfer;
	// So only one thread runs per connection, we create an array of connection objects, and tell the autopatcher server to use one thread per item
	static const int sqlConnectionObjectCount=4;
	AutopatcherMySQLRepository connectionObject[sqlConnectionObjectCount];
	AutopatcherRepositoryInterface *connectionObjectAddresses[sqlConnectionObjectCount];
	for (int i=0; i < sqlConnectionObjectCount; i++)
		connectionObjectAddresses[i]=&connectionObject[i];
	fileListTransfer.SetCallback(&progressIndicator);
	autopatcherServer.SetFileListTransferPlugin(&fileListTransfer);
#ifdef USE_TCP
	PacketizedTCP packetizedTCP;
	if (packetizedTCP.Start(LISTEN_PORT,MAX_INCOMING_CONNECTIONS)==false)
	{
		printf("Failed to start TCP. Is the port already in use?");
		return 1;
	}
	packetizedTCP.AttachPlugin(&autopatcherServer);
	packetizedTCP.AttachPlugin(&fileListTransfer);
#else
	RakPeerInterface *rakPeer;
	rakPeer = RakNetworkFactory::GetRakPeerInterface();
	SocketDescriptor socketDescriptor(LISTEN_PORT,0);
	rakPeer->Startup(8,0,&socketDescriptor, 1);
	rakPeer->SetMaximumIncomingConnections(MAX_INCOMING_CONNECTIONS);
	rakPeer->AttachPlugin(&autopatcherServer);
	rakPeer->AttachPlugin(&fileListTransfer);
#endif
	printf("started.\n");

	printf("Enter database password:\n");
	char password[128];
	char username[256];
	strcpy(username, "root");
	gets(password);
	if (password[0]==0)
		strcpy(password,"aaaa");
	char db[256];
	printf("Enter DB schema: ");
	// To create the schema, go to the command line client and type create schema autopatcher;
	// You also have to add 
	// max_allowed_packet=128M
	// Where 128 is the maximum size file in megabytes you'll ever add
	// to MySQL\MySQL Server 5.1\my.ini in the [mysqld] section
	// Be sure to restart the service after doing so
	gets(db);
	if (db[0]==0)
		strcpy(db,"autopatcher");
	for (int conIdx=0; conIdx < sqlConnectionObjectCount; conIdx++)
	{
		if (!connectionObject[conIdx].Connect("localhost", username, password, db, 0, NULL, 0))
		{
			printf("Database connection failed.\n");
			return 1;
		}
	}
	printf("Database connection suceeded.\n");


	printf("Starting threads\n");
	autopatcherServer.StartThreads(sqlConnectionObjectCount, connectionObjectAddresses);
	printf("System ready for connections\n");

	printf("(D)rop database\n(C)reate database.\n(A)dd application\n(U)pdate revision.\n(R)emove application\n(Q)uit\n");

	char ch;
	Packet *p;
	while (1)
	{
#ifdef USE_TCP
		SystemAddress notificationAddress;
		notificationAddress=packetizedTCP.HasCompletedConnectionAttempt();
		if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
			printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
		notificationAddress=packetizedTCP.HasNewIncomingConnection();
		if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
			printf("ID_NEW_INCOMING_CONNECTION\n");
		notificationAddress=packetizedTCP.HasLostConnection();
		if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
			printf("ID_CONNECTION_LOST\n");

		p=packetizedTCP.Receive();
		while (p)
		{
			packetizedTCP.DeallocatePacket(p);
			p=packetizedTCP.Receive();
		}
#else
		p=rakPeer->Receive();
		while (p)
		{
			if (p->data[0]==ID_NEW_INCOMING_CONNECTION)
				printf("ID_NEW_INCOMING_CONNECTION\n");
			else if (p->data[0]==ID_DISCONNECTION_NOTIFICATION)
				printf("ID_DISCONNECTION_NOTIFICATION\n");
			else if (p->data[0]==ID_CONNECTION_LOST)
				printf("ID_CONNECTION_LOST\n");

			rakPeer->DeallocatePacket(p);
			p=rakPeer->Receive();
		}
#endif
		if (kbhit())
		{
			ch=getch();
			if (ch=='q')
				break;
			else if (ch=='c')
			{
				if (connectionObject[0].CreateAutopatcherTables()==false)
					printf("Error: %s\n", connectionObject[0].GetLastError());
				else
					printf("Created\n");
			}
			else if (ch=='d')
			{
				if (connectionObject[0].DestroyAutopatcherTables()==false)
					printf("Error: %s\n", connectionObject[0].GetLastError());
				else
					printf("Destroyed\n");
			}
			else if (ch=='a')
			{
				printf("Enter application name to add: ");
				char appName[512];
				gets(appName);
				if (appName[0]==0)
					strcpy(appName, "TestApp");

				if (connectionObject[0].AddApplication(appName, username)==false)
					printf("Error: %s\n", connectionObject[0].GetLastError());
				else
					printf("Done\n");
			}
			else if (ch=='r')
			{
				printf("Enter application name to remove: ");
				char appName[512];
				gets(appName);
				if (appName[0]==0)
					strcpy(appName, "TestApp");

				if (connectionObject[0].RemoveApplication(appName)==false)
					printf("Error: %s\n", connectionObject[0].GetLastError());
				else
					printf("Done\n");
			}
			else if (ch=='u')
			{
				printf("Enter application name: ");
				char appName[512];
				gets(appName);
				if (appName[0]==0)
					strcpy(appName, "TestApp");

				printf("Enter application directory: ");
				char appDir[512];
				gets(appDir);
				if (appDir[0]==0)
					strcpy(appDir, "C:/temp");

				if (connectionObject[0].UpdateApplicationFiles(appName, appDir, username, &progressIndicator)==false)
				{
					printf("Error: %s\n", connectionObject[0].GetLastError());
				}
				else
				{
					printf("Update success.\n");
				}
			}
		}

		RakSleep(30);


	}

#ifdef USE_TCP
	packetizedTCP.Stop();
#else
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
#endif
}
