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
#include "PacketizedTCP.h"

// Client only includes
#include "FileListTransferCBInterface.h"
#include "FileListTransfer.h"
#include "AutopatcherClient.h"
#include "AutopatcherPatchContext.h"

#include "RakSleep.h"

class TestCB : public FileListTransferCBInterface
{
public:
	virtual bool OnFile(OnFileStruct *onFileStruct)
	{
		if (onFileStruct->context.op==PC_HASH_WITH_PATCH)
			printf("Patched: ");
		else if (onFileStruct->context.op==PC_WRITE_FILE)
			printf("Written: ");
		else if (onFileStruct->context.op==PC_ERROR_FILE_WRITE_FAILURE)
			printf("Write Failure: ");
		else if (onFileStruct->context.op==PC_ERROR_PATCH_TARGET_MISSING)
			printf("Patch target missing: ");
		else if (onFileStruct->context.op==PC_ERROR_PATCH_APPLICATION_FAILURE)
			printf("Patch process failure: ");
		else if (onFileStruct->context.op==PC_ERROR_PATCH_RESULT_CHECKSUM_FAILURE)
			printf("Patch checksum failure: ");
		else if (onFileStruct->context.op==PC_NOTICE_WILL_COPY_ON_RESTART)
			printf("Copy pending restart: ");
		else if (onFileStruct->context.op==PC_NOTICE_FILE_DOWNLOADED)
			printf("Downloaded: ");
		else if (onFileStruct->context.op==PC_NOTICE_FILE_DOWNLOADED_PATCH)
			printf("Downloaded Patch: ");
		else
			RakAssert(0);


		printf("%i. (100%%) %i/%i %s %ib->%ib / %ib->%ib\n", onFileStruct->setID, onFileStruct->fileIndex+1, onFileStruct->setCount,
			onFileStruct->fileName, onFileStruct->compressedTransmissionLength, onFileStruct->finalDataLength,
			onFileStruct->setTotalCompressedTransmissionLength, onFileStruct->setTotalFinalLength);

		// Return false for the file data to be deallocated automatically
		return false;
	}

	virtual void OnFileProgress(OnFileStruct *onFileStruct,unsigned int partCount,unsigned int partTotal,unsigned int partLength, char *firstDataChunk)
	{
		printf("Downloading: %i. (%i%%) %i/%i %s %ib->%ib / %ib->%ib\n", onFileStruct->setID, (int) (100.0*(double)partCount/(double)partTotal),
			onFileStruct->fileIndex+1, onFileStruct->setCount, onFileStruct->fileName, onFileStruct->compressedTransmissionLength,
			onFileStruct->finalDataLength, onFileStruct->setTotalCompressedTransmissionLength, onFileStruct->setTotalFinalLength);
	}

} transferCallback;

#define USE_TCP
#define LISTEN_PORT 60001
#define MAX_INCOMING_CONNECTIONS 8

int main(int argc, char **argv)
{
	printf("A simple client interface for the advanced autopatcher.\n");
	printf("Use DirectoryDeltaTransfer for a simpler version of an autopatcher.\n");
	printf("Difficulty: Intermediate\n\n");

	printf("Client starting...");
	SystemAddress serverAddress;
	AutopatcherClient autopatcherClient;
	FileListTransfer fileListTransfer;
	autopatcherClient.SetFileListTransferPlugin(&fileListTransfer);
#ifdef USE_TCP
	PacketizedTCP packetizedTCP;
	if (packetizedTCP.Start(LISTEN_PORT,MAX_INCOMING_CONNECTIONS)==false)
	{
		printf("Failed to start TCP. Is the port already in use?");
		return 1;
	}
	packetizedTCP.AttachPlugin(&autopatcherClient);
	packetizedTCP.AttachPlugin(&fileListTransfer);
#else
	RakPeerInterface *rakPeer;
	rakPeer = RakNetworkFactory::GetRakPeerInterface();
	SocketDescriptor socketDescriptor(0,0);
	rakPeer->Startup(1,0,&socketDescriptor, 1);
	// Plugin will send us downloading progress notifications if a file is split to fit under the MTU 10 or more times
	rakPeer->SetSplitMessageProgressInterval(10);
	rakPeer->AttachPlugin(&autopatcherClient);
	rakPeer->AttachPlugin(&fileListTransfer);
#endif
	printf("started\n");
	char buff[512];
	printf("Enter server IP: ");
	gets(buff);
	if (buff[0]==0)
		strcpy(buff, "127.0.0.1");
#ifdef USE_TCP
	packetizedTCP.Connect(buff,60000,false);
#else
	rakPeer->Connect(buff, 60000, 0, 0);
#endif

	printf("Connecting...\n");
	printf("Enter application directory: ");
	char appDir[512];
	gets(appDir);
	if (appDir[0]==0)
	{
		strcpy(appDir, "C:/temp/AutopatcherClient");
	}
	printf("Enter application name: ");
	char appName[512];
	gets(appName);
	if (appName[0]==0)
		strcpy(appName, "TestApp");

	printf("Hit 'q' to quit, 'p' to patch, 'c' to cancel the patch.\n");
	char ch;
	Packet *p;
	while (1)
	{
#ifdef USE_TCP
		SystemAddress notificationAddress;
		notificationAddress=packetizedTCP.HasCompletedConnectionAttempt();
		if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
		{
			printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
			serverAddress=notificationAddress;
		}
		notificationAddress=packetizedTCP.HasNewIncomingConnection();
		if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
			printf("ID_NEW_INCOMING_CONNECTION\n");
		notificationAddress=packetizedTCP.HasLostConnection();
		if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
			printf("ID_CONNECTION_LOST\n");

		p=packetizedTCP.Receive();
		while (p)
		{
			if (p->data[0]==ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR)
			{
				char buff[256];
				RakNet::BitStream temp(p->data, p->length, false);
				temp.IgnoreBits(8);
				stringCompressor->DecodeString(buff, 256, &temp);
				printf("ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR\n");
				printf("%s\n", buff);
			}
			else if (p->data[0]==ID_AUTOPATCHER_FINISHED)
				printf("ID_AUTOPATCHER_FINISHED\n");
			else if (p->data[0]==ID_AUTOPATCHER_RESTART_APPLICATION)
				printf("Launch \"AutopatcherClientRestarter.exe autopatcherRestart.txt\"\nQuit this application immediately after to unlock files.\n");

			packetizedTCP.DeallocatePacket(p);
			p=packetizedTCP.Receive();
		}
#else
		p=rakPeer->Receive();
		while (p)
		{
			if (p->data[0]==ID_DISCONNECTION_NOTIFICATION)
				printf("ID_DISCONNECTION_NOTIFICATION\n");
			else if (p->data[0]==ID_CONNECTION_LOST)
				printf("ID_CONNECTION_LOST\n");
			else if (p->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{
				printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
				serverAddress=p->systemAddress;
			}
			else if (p->data[0]==ID_CONNECTION_ATTEMPT_FAILED)
				printf("ID_CONNECTION_ATTEMPT_FAILED\n");
			else if (p->data[0]==ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR)
			{
				char buff[256];
				RakNet::BitStream temp(p->data, p->length, false);
				temp.IgnoreBits(8);
				stringCompressor->DecodeString(buff, 256, &temp);
				printf("ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR\n");
				printf("%s\n", buff);
			}
			else if (p->data[0]==ID_AUTOPATCHER_FINISHED)
				printf("ID_AUTOPATCHER_FINISHED\n");
			else if (p->data[0]==ID_AUTOPATCHER_RESTART_APPLICATION)
				printf("Launch \"AutopatcherClientRestarter.exe autopatcherRestart.txt\"\nQuit this application immediately after to unlock files.\n");

			rakPeer->DeallocatePacket(p);
			p=rakPeer->Receive();
		}
#endif

		if (kbhit())
		{
			ch=getch();
			if (ch=='q')
				break;
			else if (ch=='p')
			{
				char lastUpdateDate[128];
				char restartFile[512];
				strcpy(restartFile, appDir);
				strcat(restartFile, "/autopatcherRestart.txt");
				printf("Enter last update date (only newer updates retrieved) or nothing to get all updates\n");
				gets(lastUpdateDate);

				if (autopatcherClient.PatchApplication(appName, appDir, lastUpdateDate, serverAddress, &transferCallback, restartFile, argv[0]))
				{
					printf("Patching process starting.\n");
				}
				else
				{
					printf("Failed to start patching.\n");
				}
			}
			else if (ch=='c')
			{
				autopatcherClient.Clear();
				printf("Autopatcher cleared.\n");
			}

		}

		RakSleep(30);
	}

#ifdef USE_TCP
	packetizedTCP.Stop();
#else
	rakPeer->Shutdown(500,0);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
#endif
	return 1;
}
