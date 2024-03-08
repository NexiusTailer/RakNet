#include "CompressionConvertTest.h"

int CompressionConvertTest::RunAndSave(bool isVerbose,bool noPauses,FILE *serverToClientFrequencyTableFilePointer,FILE *clientToServerFrequencyTableFilePointer,bool destructionTest)
{

	// Pointers to the interfaces of our server and client.
	// Note we can easily have both in the same program

	char serverToClientFrequencyTableFilename[100];

	rakClient=RakNetworkFactory::GetRakPeerInterface();
	rakServer=RakNetworkFactory::GetRakPeerInterface();

	// Frequency table tracking takes a bit of CPU power under high loads so is off by default.  We need to turn it on
	// So RakNet will track data frequencies which we can then use to create the compression tables
	rakServer->SetCompileFrequencyTable(true);
	rakClient->SetCompileFrequencyTable(true);

	int i = rakServer->GetNumberOfAddresses();

	// We are generating compression layers for both the client and the server just to demonstrate it
	// in practice you would only do one or the other depending on whether you wanted to run as a client or as a server
	// Note that both the client and the server both need both frequency tables and they must be the same

	if (serverToClientFrequencyTableFilePointer!=0)
	{
		unsigned int frequencyData[256];
		int numRead;
		numRead=(int)fread(frequencyData, sizeof(unsigned int), 256, serverToClientFrequencyTableFilePointer);
		if (numRead != 256)
		{

			if (isVerbose)
				DebugTools::ShowError("Problem reading frequency data\n",!noPauses && isVerbose,__LINE__,__FILE__);

			return 2;
		}
		else
		{
			rakClient->GenerateCompressionLayer(frequencyData, true); // server to client is input for the client so the last parameter is true
			rakServer->GenerateCompressionLayer(frequencyData, false); // server to client is output for the server so the last parameter is false
			if (isVerbose)
				printf("Compression layer generated for server to client data\n");
		}

		fclose(serverToClientFrequencyTableFilePointer);
	}

	if (clientToServerFrequencyTableFilePointer!=0)
	{
		unsigned int frequencyData[256];
		int numRead;
		numRead=(int)fread(frequencyData, sizeof(unsigned int), 256, clientToServerFrequencyTableFilePointer);
		if (numRead != 256)
		{

			if (isVerbose)
				DebugTools::ShowError("Problem reading frequency data\n",!noPauses && isVerbose,__LINE__,__FILE__);

			return 2;
		}
		else
		{
			rakClient->GenerateCompressionLayer(frequencyData, false); // client to server is output for the client so the last parameter is false
			rakServer->GenerateCompressionLayer(frequencyData, true); // client to server is input for the server so the last parameter is true
			if (isVerbose)
				printf("Compression layer generated for server to client data\n");
		}

		fclose(clientToServerFrequencyTableFilePointer);
	}

	if (destructionTest)
	{
		if (!rakClient->DeleteCompressionLayer(true))
		{

			if (isVerbose)
				DebugTools::ShowError("Failed to delete client input layer\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 15;
		}
		if (!rakClient->DeleteCompressionLayer(false))
		{

			if (isVerbose)
				DebugTools::ShowError("Failed to delete client output layer\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 16;
		}

		if (!rakServer->DeleteCompressionLayer(true))
		{

			if (isVerbose)
				DebugTools::ShowError("Failed to delete server input layer\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 19;
		}
		if (!rakServer->DeleteCompressionLayer(false))
		{

			if (isVerbose)
				DebugTools::ShowError("Failed to delete server output layer\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 20;
		}
	}

	{
		// Holds user data
		char portstring[30];

		strcpy(portstring, "60000");
		if (isVerbose)
			printf("Starting server.\n");
		// Starting the server is very simple.  2 players allowed.
		// 0 means we don't care about a connectionValidationInteger, and false
		// for low priority threads
		SocketDescriptor socketDescriptor(atoi(portstring),0);
		bool b = rakServer->Startup(2, 30, &socketDescriptor, 1);
		rakServer->SetMaximumIncomingConnections(2);
		if (b)
		{
			if (isVerbose)
				printf("Server started, waiting for connections.\n");
		}
		else
		{ 
			if (isVerbose)
				DebugTools::ShowError("Server failed to start.  Terminating.\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 9;
		}
	}

	{

		bool b= rakClient->Startup(1, 30, &SocketDescriptor(60001,0), 1);

		if (b)
		{
			if (isVerbose)
				printf("Client started\n");
		}
		else
		{ 
			if (isVerbose)
				DebugTools::ShowError("Client failed to start.  Terminating.\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 10;
		}
	}

	SystemAddress serverAddress;
	serverAddress.SetBinaryAddress("127.0.0.1");
	serverAddress.port=60000;
	RakNetTime entryTime=RakNet::GetTime();

	while(!rakClient->IsConnected (serverAddress,false,false)&&RakNet::GetTime()-entryTime<5000)
	{

		if(!rakClient->IsConnected (serverAddress,true,true))
		{
			rakClient->Connect("127.0.0.1",serverAddress.port,0,0);
		}

		RakSleep(100);

	}

	if (!rakClient->IsConnected (serverAddress,false,false))
	{
		DebugTools::ShowError("Client could not connect after 5 seconds\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 14;
	}
	char message[400];

	entryTime=RakNet::GetTime();

	if (isVerbose)
	printf("Starting data collection\n");

	while (RakNet::GetTime()-entryTime<2000)//2 seconds
	{

		{
			// Notice what is not here: something to keep our network running.  It's
			// fine to block on gets or anything we want
			// Because the network engine was painstakingly written using threads.
			strcpy(message," Test data: ccccccccccccccccccc");

			// Message now holds what we want to broadcast

			{
				char message2[420];
				// Append Server: to the message so clients know that it ORIGINATED from the server
				// All messages to all clients come from the server either directly or by being
				// relayed from other cilents
				strcpy(message2, "Server: ");
				strcat(message2, message);

				// message2 is the data to send
				// strlen(message2)+1 is to send the null terminator
				// HIGH_PRIORITY doesn't actually matter here because we don't use any other priority
				// RELIABLE_ORDERED means make sure the message arrives in the right order
				// We arbitrarily pick 0 for the ordering stream
				// UNASSIGNED_SYSTEM_ADDRESS means don't exclude anyone from the broadcast
				// true means broadcast the message to everyone connected
				rakServer->Send(message2, (const int)strlen(message2)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			}

			{
				// message is the data to send
				// strlen(message)+1 is to send the null terminator
				// HIGH_PRIORITY doesn't actually matter here because we don't use any other priority
				// RELIABLE_ORDERED means make sure the message arrives in the right order
				message[0]=ID_USER_PACKET_ENUM;
				rakClient->Send(message, (const int)strlen(message)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			}
		}

		Packet *packet;
		packet = rakClient->Receive();
		if (packet)
		{
			if (packet->data[0]>=ID_USER_PACKET_ENUM)
			{
				// It's a client, so just show the message
				//printf("%s\n", packet->data);
			}
			rakClient->DeallocatePacket(packet);
		}
		packet = rakServer->Receive();
		if (packet)
		{

			//printf("%s\n", packet->data);
			if (packet->data[0]>=ID_USER_PACKET_ENUM)
			{
				char message[200];
				//printf("%s\n", packet->data+1);

				// Relay the message.  We prefix the name for other clients.  This demonstrates
				// That messages can be changed on the server before being broadcast
				// Sending is the same as before
				sprintf(message, "%s", packet->data+1);
				rakServer->Send(message, (const int)strlen(message)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			}
			if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{
				//printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
			}
			rakServer->DeallocatePacket(packet);
		}
	}

	// Output statistics

	rakServer->Shutdown(100);
	rakClient->Shutdown(100);

	if(destructionTest)//Because we destroyed the data that we pulled from file, the ratio should be 0 because we just generated scratch data and didn't have any to use
	{

		if (rakClient->GetCompressionRatio()!=0)
		{
			if (isVerbose)
				DebugTools::ShowError("Failed to delete a client compression layer\n",!noPauses && isVerbose,__LINE__,__FILE__);

			return 22;
		}

		if (rakServer->GetCompressionRatio()!=0)
		{
			if (isVerbose)
				DebugTools::ShowError("Failed to delete a server compression layer\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 23;
		}

	}

	if (!destructionTest){

		strcpy(serverToClientFrequencyTableFilename, "c2s");

		serverToClientFrequencyTableFilePointer = fopen(serverToClientFrequencyTableFilename, "wb");
		if (serverToClientFrequencyTableFilePointer==0)
		{

			if (isVerbose)
			{
				printf("Can't open %s for writing\n", serverToClientFrequencyTableFilename);
				DebugTools::ShowError("",!noPauses && isVerbose,__LINE__,__FILE__);

			}
			return 17;
		}
		else
		{
			unsigned int frequencyData[256];

			// Get the frequency table generated during the run

			rakClient->GetOutgoingFrequencyTable(frequencyData);

			fwrite(frequencyData, sizeof(unsigned int), 256, serverToClientFrequencyTableFilePointer);
			fclose(serverToClientFrequencyTableFilePointer);
		}

		strcpy(serverToClientFrequencyTableFilename, "s2c");

		serverToClientFrequencyTableFilePointer = fopen(serverToClientFrequencyTableFilename, "wb");
		if (serverToClientFrequencyTableFilePointer==0)
		{

			if (isVerbose)
			{
				printf("Can't open %s fow writing\n", serverToClientFrequencyTableFilename);
				DebugTools::ShowError("",!noPauses && isVerbose,__LINE__,__FILE__);

			}
			return 18;
		}
		else
		{
			unsigned int frequencyData[256];

			// Get the frequency table generated during the run

			rakServer->GetOutgoingFrequencyTable(frequencyData);

			fwrite(frequencyData, sizeof(unsigned int), 256, serverToClientFrequencyTableFilePointer);
			fclose(serverToClientFrequencyTableFilePointer);
		}

	}
	// We're done with the network
	//RakNetworkFactory::DestroyRakPeerInterface(rakServer);
	//RakNetworkFactory::DestroyRakPeerInterface(rakClient);

	return 0;
}
/*
Description:

Success conditions:
Generates and uses compressesion properly.

Failure conditions:
Problem reading frequency data
Problem with the server decompress ratio
Problem with the server Compress ratio
Problem with the client decompress ratio
Problem with the client Compress ratio
Server fails to start
Client fails to start
Cant open file c2s after it is generated
Cant open file s2s after it is generated
Client fails to connect after alloted time
Fails to delete input layer
Fails to delete output layer

RakPeerInterface Functions used, tested indirectly by its use:
Startup
SetMaximumIncomingConnections
Receive
DeallocatePacket
Send
Shutdown
IsConnected

RakPeerInterface Functions Explicitly Tested:
GenerateCompressionLayer
SetCompileFrequencyTable
GetOutgoingFrequencyTable
GetCompressionRatio
GetDecompressionRatio
DeleteCompressionLayer
*/
int CompressionConvertTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{
	bool returnValue;
	if (isVerbose)
	{
		printf("Compression data collection run 1\n");
	}
	returnValue=RunAndSave(isVerbose,noPauses,0,0,false)>0;

	if (returnValue!=0)
	{
		return returnValue;
	}
	DestroyPeers();

	FILE *serverToClientFrequencyTableFilePointer, *clientToServerFrequencyTableFilePointer;
	char serverToClientFrequencyTableFilename[100], clientToServerFrequencyTableFilename[100];

	strcpy(serverToClientFrequencyTableFilename, "s2c");
	serverToClientFrequencyTableFilePointer = fopen(serverToClientFrequencyTableFilename, "rb");

	strcpy(clientToServerFrequencyTableFilename, "c2s");
	clientToServerFrequencyTableFilePointer = fopen(clientToServerFrequencyTableFilename, "rb");
	if (isVerbose)
	{
		printf("Compression data collection run 2\n");
	}
	returnValue=RunAndSave(isVerbose,noPauses,serverToClientFrequencyTableFilePointer,clientToServerFrequencyTableFilePointer,false)>0;
	if (returnValue!=0)
	{
		return returnValue;
	}
	DestroyPeers();
	strcpy(serverToClientFrequencyTableFilename, "s2c");
	serverToClientFrequencyTableFilePointer = fopen(serverToClientFrequencyTableFilename, "rb");

	strcpy(clientToServerFrequencyTableFilename, "c2s");
	clientToServerFrequencyTableFilePointer = fopen(clientToServerFrequencyTableFilename, "rb");
	if (isVerbose)
	{
		printf("Compression data collection run 3\n");
	}
	returnValue=RunAndSave(isVerbose,noPauses,serverToClientFrequencyTableFilePointer,clientToServerFrequencyTableFilePointer,false)>0;
	if (returnValue!=0)
	{
		return returnValue;
	}
	DestroyPeers();

	strcpy(serverToClientFrequencyTableFilename, "s2c");
	serverToClientFrequencyTableFilePointer = fopen(serverToClientFrequencyTableFilename, "rb");

	strcpy(clientToServerFrequencyTableFilename, "c2s");
	clientToServerFrequencyTableFilePointer = fopen(clientToServerFrequencyTableFilename, "rb");
	if (isVerbose)
	{
		printf("Compression data collection run and destruction, no data save\n");
	}
	returnValue=RunAndSave(isVerbose,noPauses,serverToClientFrequencyTableFilePointer,clientToServerFrequencyTableFilePointer,true)>0;
	if (returnValue!=0)
	{
		return returnValue;
	}
	DestroyPeers();

	// Pointers to the interfaces of our server and client.
	// Note we can easily have both in the same program

	rakClient=RakNetworkFactory::GetRakPeerInterface();
	rakServer=RakNetworkFactory::GetRakPeerInterface();

	// Frequency table tracking takes a bit of CPU power under high loads so is off by default.  We need to turn it on
	// So RakNet will track data frequencies which we can then use to create the compression tables
	rakServer->SetCompileFrequencyTable(true);
	rakClient->SetCompileFrequencyTable(true);

	int i = rakServer->GetNumberOfAddresses();

	// Just so we can remember where the packet came from
	//bool isServer;

	// We are generating compression layers for both the client and the server just to demonstrate it
	// in practice you would only do one or the other depending on whether you wanted to run as a client or as a server
	// Note that both the client and the server both need both frequency tables and they must be the same

	strcpy(serverToClientFrequencyTableFilename, "s2c");
	serverToClientFrequencyTableFilePointer = fopen(serverToClientFrequencyTableFilename, "rb");
	if (serverToClientFrequencyTableFilePointer==0)
	{
		if (isVerbose)
			DebugTools::ShowError("Problem opening s2c,should be generated\n",!noPauses && isVerbose,__LINE__,__FILE__);

		return 12;

	}
	else
	{
		unsigned int frequencyData[256];
		int numRead;
		numRead=(int)fread(frequencyData, sizeof(unsigned int), 256, serverToClientFrequencyTableFilePointer);
		if (numRead != 256)
		{

			if (isVerbose)
				DebugTools::ShowError("Problem reading frequency data\n",!noPauses && isVerbose,__LINE__,__FILE__);

			return 2;
		}
		else
		{
			rakClient->GenerateCompressionLayer(frequencyData, true); // server to client is input for the client so the last parameter is true
			rakServer->GenerateCompressionLayer(frequencyData, false); // server to client is output for the server so the last parameter is false

			if (isVerbose)
				printf("Compression layer generated for server to client data\n");
		}

		fclose(serverToClientFrequencyTableFilePointer);
	}

	strcpy(clientToServerFrequencyTableFilename, "c2s");
	clientToServerFrequencyTableFilePointer = fopen(clientToServerFrequencyTableFilename, "rb");
	if (clientToServerFrequencyTableFilePointer==0)
	{
		if (isVerbose)
			DebugTools::ShowError("Problem opening c2s,should be generated\n",!noPauses && isVerbose,__LINE__,__FILE__);

		return 11;
	}
	else
	{
		unsigned int frequencyData[256];
		int numRead;
		numRead=(int)fread(frequencyData, sizeof(unsigned int), 256, clientToServerFrequencyTableFilePointer);
		if (numRead != 256)
		{

			if (isVerbose)
				DebugTools::ShowError("Problem reading frequency data\n",!noPauses && isVerbose,__LINE__,__FILE__);

			return 2;
		}
		else
		{
			rakClient->GenerateCompressionLayer(frequencyData, false); // client to server is output for the client so the last parameter is false
			rakServer->GenerateCompressionLayer(frequencyData, true); // client to server is input for the server so the last parameter is true
			if(isVerbose)
				printf("Compression layer generated for server to client data\n");
		}

		fclose(clientToServerFrequencyTableFilePointer);
	}

	{
		// Holds user data
		char portstring[30];

		strcpy(portstring, "60000");
		puts("Starting server.");
		// Starting the server is very simple.  2 players allowed.
		// 0 means we don't care about a connectionValidationInteger, and false
		// for low priority threads
		SocketDescriptor socketDescriptor(atoi(portstring),0);
		bool b = rakServer->Startup(2, 30, &socketDescriptor, 1);
		rakServer->SetMaximumIncomingConnections(2);
		if (b)
		{
			if (isVerbose)
				printf("Server started, waiting for connections.\n");
		}
		else
		{ 
			if (isVerbose)
				DebugTools::ShowError("Server failed to start.  Terminating.\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 9;
		}
	}

	{

		bool b= rakClient->Startup(1, 30, &SocketDescriptor(60001,0), 1);

		if (b)
		{
			if (isVerbose)
				printf("Client started.\n");
		}
		else
		{ 
			if (isVerbose)
				DebugTools::ShowError("Client failed to start.  Terminating.\n",!noPauses && isVerbose,__LINE__,__FILE__);
			return 10;
		}
	}

	SystemAddress serverAddress;
	serverAddress.SetBinaryAddress("127.0.0.1");
	serverAddress.port=60000;
	RakNetTime entryTime=RakNet::GetTime();

	while(!rakClient->IsConnected (serverAddress,false,false)&&RakNet::GetTime()-entryTime<5000)
	{

		if(!rakClient->IsConnected (serverAddress,true,true))
		{
			rakClient->Connect("127.0.0.1",serverAddress.port,0,0);
		}

		RakSleep(100);

	}

	if (!rakClient->IsConnected (serverAddress,false,false))
	{
		DebugTools::ShowError("Client could not connect after 5 seconds\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 14;
	}

	char message[400];

	entryTime=RakNet::GetTime();
	// Loop for input
	while (RakNet::GetTime()-entryTime<10000)//10 seconds
	{

		{
			// Notice what is not here: something to keep our network running.  It's
			// fine to block on gets or anything we want
			// Because the network engine was painstakingly written using threads.
			strcpy(message," Test data: ccccccccccccccccccc");

			// Message now holds what we want to broadcast

			{
				char message2[420];
				// Append Server: to the message so clients know that it ORIGINATED from the server
				// All messages to all clients come from the server either directly or by being
				// relayed from other cilents
				strcpy(message2, "Server: ");
				strcat(message2, message);

				// message2 is the data to send
				// strlen(message2)+1 is to send the null terminator
				// HIGH_PRIORITY doesn't actually matter here because we don't use any other priority
				// RELIABLE_ORDERED means make sure the message arrives in the right order
				// We arbitrarily pick 0 for the ordering stream
				// UNASSIGNED_SYSTEM_ADDRESS means don't exclude anyone from the broadcast
				// true means broadcast the message to everyone connected
				rakServer->Send(message2, (const int)strlen(message2)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			}

			{
				// message is the data to send
				// strlen(message)+1 is to send the null terminator
				// HIGH_PRIORITY doesn't actually matter here because we don't use any other priority
				// RELIABLE_ORDERED means make sure the message arrives in the right order
				message[0]=ID_USER_PACKET_ENUM;
				rakClient->Send(message, (const int)strlen(message)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			}
		}

		Packet *packet;
		packet = rakClient->Receive();
		if (packet)
		{
			if (packet->data[0]>=ID_USER_PACKET_ENUM)
			{
				// It's a client, so just show the message
				if(isVerbose)
					printf("%s\n", packet->data);
			}
			rakClient->DeallocatePacket(packet);
		}
		packet = rakServer->Receive();
		if (packet)
		{

			//printf("%s\n", packet->data);
			if (packet->data[0]>=ID_USER_PACKET_ENUM)
			{
				char message[200];
				if(isVerbose)
					printf("%s\n", packet->data+1);

				// Relay the message.  We prefix the name for other clients.  This demonstrates
				// That messages can be changed on the server before being broadcast
				// Sending is the same as before
				sprintf(message, "%s", packet->data+1);
				rakServer->Send(message, (const int)strlen(message)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			}
			if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{
				if(isVerbose)
					printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
			}
			rakServer->DeallocatePacket(packet);
		}
	}

	float serverCompress=rakServer->GetCompressionRatio();
	float serverDecompress= rakServer->GetDecompressionRatio();
	float clientCompress=rakClient->GetCompressionRatio();
	float clientDecompress=rakClient->GetDecompressionRatio();
	// Output statistics
	if (isVerbose)
	{
		printf("Server: Compression ratio=%f. Decompression ratio=%f\n",serverCompress ,serverDecompress);
		printf("Client: Compression ratio=%f. Decompression ratio=%f\n",clientCompress ,clientDecompress );
	}

	if(serverDecompress==1||serverDecompress==0)//Tested and always should be not 0 or 1
	{

		if (isVerbose)
			DebugTools::ShowError("Problem with the server decompress ratio\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 4;
	}

	if(serverCompress==1||serverCompress==0)//Tested and always should be not 0 or 1
	{
		if (isVerbose)
			DebugTools::ShowError("Problem with the server compress ratio\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 5;
	}

	if(clientCompress==1||clientCompress==0)//Tested and always should be not 0 or 1
	{
		if (isVerbose)
			DebugTools::ShowError("Problem with the client decompress ratio\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 6;
	}

	if(clientDecompress==1||clientDecompress==0)//Tested and always should be not 0 or 1
	{

		if (isVerbose)
			DebugTools::ShowError("Problem with the client compress ratio\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 7;
	}

	rakServer->Shutdown(100);
	rakClient->Shutdown(100);

	strcpy(serverToClientFrequencyTableFilename, "c2s");

	remove(serverToClientFrequencyTableFilename);

	strcpy(serverToClientFrequencyTableFilename, "s2c");

	remove(serverToClientFrequencyTableFilename);

	return 0;

}

void CompressionConvertTest::DestroyPeers()
{
	// We're done with the network
	RakNetworkFactory::DestroyRakPeerInterface(rakServer);
	RakNetworkFactory::DestroyRakPeerInterface(rakClient);
}

RakNet::RakString CompressionConvertTest::GetTestName()
{

	return "CompressionConvertTest";

}

RakNet::RakString CompressionConvertTest::ErrorCodeToString(int errorCode)
{

	switch (errorCode)
	{

	case 0:
		return "No error";
		break;

	case 2:
		return "Problem reading frequency data";
		break;
	case 4:
		return "Problem with the server decompress ratio";
		break;
	case 5:
		return "Problem with the server Compress ratio";
		break;
	case 6:
		return "Problem with the client decompress ratio";
		break;
	case 7:
		return "Problem with the client Compress ratio";
		break;
	case 9:
		return "Server failed to start";
		break;

	case 10:
		return "Client failed to start";
		break;
	case 11:
		return "Can't open file c2s for reading";
		break;
	case 12:
		return "Can't open file s2s for reading";
		break;
	case 14:
		return "Client failed to connect after alloted time";
		break;

	case 15:
		return "Failed to delete client input layer";
		break;
	case 16:
		return "Failed to delete client output layer";
		break;

	case 17:
		return "Can't open file c2s for writing";
		break;
	case 18:
		return "Can't open file s2s writing";
		break;

	case 19:
		return "Failed to delete server input layer";
		break;
	case 20:
		return "Failed to delete client output layer";
		break;
	case 22:
		return "Failed to delete a client compression layer";
		break;
	case 23:
		return "Failed to delete a server compression layer";
		break;

	default:
		return "Undefined Error";
	}

}

CompressionConvertTest::CompressionConvertTest(void)
{
}

CompressionConvertTest::~CompressionConvertTest(void)
{
}