// How to patch
// 1. Create a new server instance using the most recent image (optional)
// 2. Connect to the new (or any existing) instance through remote desktop. Immediately set max concurrent users to 0. Wait for users to finish patching, if any started.
// 3. Update the patch
// 4. Image the server
// 5. Wait for the image to complete (press l to check, or use Rackspace control panel. Takes like half an hour).
// 6. When the image completes, set the image spawn from the 'l' option to the image that was created in step 4
// 6. Send terminate command. Servers running the old patch will shutdown automatically.
// 9. Set max concurrent users back above 0.
// 10. Optional: Manually call SpawnServers() if load is immediately anticipated

// Common includes
#include <stdio.h>
#include <stdlib.h>
#include "Kbhit.h"

#include "GetTime.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "StringCompressor.h"
#include "FileListTransfer.h"
#include "FileList.h" // FLP_Printf
#include "PacketizedTCP.h"
#include "Gets.h"
#include "CloudServerHelper.h"
#include "FullyConnectedMesh2.h"
#include "TwoWayAuthentication.h"
#include "CloudClient.h"
#include "DynDNS.h"
#include "RakPeerInterface.h"
#include "RakSleep.h"
#include "ConnectionGraph2.h"
#include "CloudServerHelper.h"
#include "HTTPConnection2.h"
#include "Rackspace2.h"
#include "GetTime.h"
// See http://www.digip.org/jansson/doc/2.4/
// This is used to make it easier to parse the JSON returned from the master server
#include "jansson.h"

// Server only includes
#include "AutopatcherServer.h"
// Replace this repository with your own implementation if you don't want to use PostgreSQL
#include "AutopatcherPostgreRepository.h"

#ifdef _WIN32
#include "WindowsIncludes.h" // Sleep
#else
#include <unistd.h> // usleep
#endif

#define LISTEN_PORT_TCP_PATCHER 60000

using namespace RakNet;

RakNet::RakPeerInterface *rakPeer;
RakNet::AutopatcherServer *autopatcherServer;

enum AppState
{
	AP_RUNNING,
	AP_TERMINATE_WHEN_ZERO_USERS_REACHED,
	AP_TERMINATE_IF_NOT_DNS_HOST,
	AP_TERMINATING,
	AP_TERMINATED,
} appState;

RakNet::Time timeSinceZeroUsers;


struct CloudServerHelper_RackspaceCloudDNS : public CloudServerHelper, public Rackspace2EventCallback
{
public:

	enum CSHState
	{
		AUTHENTICATING,
		AUTHENTICATED,
		CANNOT_FIND_PATCHER_DOMAIN,
		CANNOT_FIND_PATCHER_RECORD,
		GETTING_SERVERS,
		GOT_MY_SERVER,
		CANNOT_FIND_MY_SERVER,
		GETTING_DOMAINS,
		GOT_DOMAIN,
		GETTING_RECORDS,
		GOT_RECORDS,

	} cshState;

	CloudServerHelper_RackspaceCloudDNS() {
		rackspace2= RakNet::OP_NEW<Rackspace2>(_FILE_AND_LINE_);
		memset(&thisServerDetail, 0, sizeof(ServerDetail));
	}
	~CloudServerHelper_RackspaceCloudDNS() {
		RakNet::OP_DELETE(rackspace2,_FILE_AND_LINE_);
	}
	virtual bool Update(void) {
		UpdateTCP();
		return true;
	}
	void CopyServerDetails(json_t *arrayElement, const char *publicIPV4)
	{
		strcpy(thisServerDetail.publicIPV4, publicIPV4);
		strcpy(thisServerDetail.id, json_string_value(json_object_get(arrayElement, "id")));
		json_t *privateAddressesArray = json_object_get(json_object_get(arrayElement, "addresses"),"private");
		size_t privateAddressesArraySize = json_array_size(privateAddressesArray);
		for (size_t l=0; l < privateAddressesArraySize; l++)
		{
			json_t *privateAddressElement = json_array_get(privateAddressesArray, l);
			if (json_integer_value(json_object_get(privateAddressElement, "version"))==4)
			{
				strcpy(thisServerDetail.privateIPV4,json_string_value(json_object_get(privateAddressElement, "addr")));
				break;
			}
		}

		strcpy(thisServerDetail.flavorId,json_string_value(json_object_get(json_object_get(arrayElement, "flavor"),"id")));
		strcpy(thisServerDetail.hostId,json_string_value(json_object_get(arrayElement, "hostId")));
		strcpy(thisServerDetail.id,json_string_value(json_object_get(arrayElement, "id")));
		strcpy(thisServerDetail.imageId,json_string_value(json_object_get(json_object_get(arrayElement, "image"),"id")));
		strcpy(thisServerDetail.name,json_string_value(json_object_get(arrayElement, "name")));
		strcpy(spawningImageId, thisServerDetail.imageId);
		strcpy(spawningFlavorId, thisServerDetail.flavorId);
		printf("Using image id %s for new servers\n", spawningImageId);
	}
	virtual void OnTCPFailure(void){ printf("--ERROR--: OnTCPFailure()\n"); }
	virtual void OnTransmissionFailed(RakString postStr, RakString authURLDomain){ printf("--ERROR--: OnTransmissionFailed(). postStr=%s authURLDomain=%s\n", postStr.C_String(), authURLDomain.C_String()); }
	virtual void OnEmptyResponse(RakString stringTransmitted) {
		printf("--ERROR--: OnEmptyResponse(). stringTransmitted=%s", stringTransmitted.C_String());
	}
	virtual void OnMessage(const char *message, RakString responseReceived, RakString stringTransmitted, int contentOffset)
	{
		printf("--WARNING--: OnMessage(). message=%s\nstringTransmitted=%s", message, stringTransmitted.C_String());
	}
	virtual void OnResponse(Rackspace2ResponseCode r2rc, RakString responseReceived, int contentOffset){
		if (r2rc==R2RC_AUTHENTICATED)
		{
			printf("Authenticated with Rackspace\nX-Auth-Token: %s\n", rackspace2->GetAuthToken());
			cshState=AUTHENTICATED;

			json_error_t error;
			json_t *root = json_loads(strstr(responseReceived.C_String() + contentOffset, "{"), JSON_REJECT_DUPLICATES | JSON_DISABLE_EOF_CHECK, &error);
			json_t *accessJson = json_object_get(root, "access");
			json_t *userJson  = json_object_get(accessJson, "user");
			json_t *regionJson = json_object_get(userJson, "RAX-AUTH:defaultRegion");
			const char *userRegionStr = json_string_value(regionJson);
			json_t *serviceCatalogArray = json_object_get(accessJson, "serviceCatalog");
			size_t arraySize = json_array_size(serviceCatalogArray);
			size_t i;
			printf("Services:\n");
			for (i=0; i < arraySize; i++)
			{
				json_t *arrayElement = json_array_get(serviceCatalogArray, i);
				json_t *elementNameJson = json_object_get(arrayElement, "name");
				const char *elementNameStr = json_string_value(elementNameJson);
				json_t *elementTypeJson = json_object_get(arrayElement, "type");
				const char *elementTypeStr = json_string_value(elementTypeJson);

				printf("%i. name=%s type=%s ", i+1, elementNameStr, elementTypeStr);

				json_t *endpointArray = json_object_get(arrayElement, "endpoints");
				size_t endpointArraySize = json_array_size(endpointArray);
				size_t j;
				char *myPublicURLStr=0;
				for (j=0; j < endpointArraySize; j++)
				{
					json_t *endpointArrayElement = json_array_get(endpointArray, j);
					json_t *publicURLJson = json_object_get(endpointArrayElement, "publicURL");
					const char *publicURLStr = json_string_value(publicURLJson);
					json_t *regionJson = json_object_get(endpointArrayElement, "region");
					if (regionJson)
					{
						const char *regionStr = json_string_value(regionJson);
						if (strcmp(regionStr, userRegionStr)==0)
						{
							printf("My URL=%s", publicURLStr);
							myPublicURLStr=(char*) publicURLStr;
							break;
						}
					}
					else
					{
						printf("My URL=%s", publicURLStr);
						myPublicURLStr=(char*) publicURLStr;
						break;
					}
				}
				if (j==endpointArraySize)
					printf("My URL=<unknown>");
				if (myPublicURLStr)
				{
					if (strcmp(elementNameStr, "cloudDNS")==0)
						strcpy(rackspaceDNSURL, myPublicURLStr);
					else if (strcmp(elementNameStr, "cloudServersOpenStack")==0)
						strcpy(rackspaceServersURL, myPublicURLStr);
				}
				printf("\n");
			}



			json_decref(root);
		}
		else if (r2rc==R2RC_GOT_SERVERS)
		{
			json_error_t error;
			json_t *root = json_loads(strstr(responseReceived.C_String() + contentOffset, "{"), JSON_REJECT_DUPLICATES | JSON_DISABLE_EOF_CHECK, &error);
			json_t *serversArray = json_object_get(root, "servers");
			size_t arraySize = json_array_size(serversArray);
			size_t i;

			for (i=0; i < arraySize && cshState==GETTING_SERVERS; i++)
			{
				json_t *arrayElement = json_array_get(serversArray, i);
				//json_t *publicAddressesArray = json_object_get(json_object_get(arrayElement, "addresses"),"public");
				//size_t publicAddressesArraySize = json_array_size(publicAddressesArray);
				//for (size_t j=0; j < publicAddressesArraySize; j++)
				//{
				for (int k=0; k < rakPeer->GetNumberOfAddresses(); k++)
				{
					if (strcmp(rakPeer->GetLocalIP(k), json_string_value(json_object_get(arrayElement, "accessIPv4")))==0)
					{
						CopyServerDetails(arrayElement, rakPeer->GetLocalIP(k));

						cshState=GOT_MY_SERVER;

						break;
					}
				}
				//}



			}		

			if (cshState==GETTING_SERVERS)
			{
				printf("Could not find a server with this IP address\n");
				printf("Continue anyway (y/n)?\n");
				if (getch()=='y')
				{
					json_t *arrayElement = json_array_get(serversArray, 0);
					CopyServerDetails(arrayElement, rakPeer->GetLocalIP(0));
					cshState=GOT_MY_SERVER;;
				}
				else
					cshState=CANNOT_FIND_MY_SERVER;
			}

			json_decref(root);
		}
		else if (r2rc==R2RC_GOT_IMAGES)
		{
			json_error_t error;
			json_t *root = json_loads(strstr(responseReceived.C_String() + contentOffset, "{"), JSON_REJECT_DUPLICATES | JSON_DISABLE_EOF_CHECK, &error);
			json_t *domainsArray = json_object_get(root, "images");
			size_t arraySize = json_array_size(domainsArray);
			size_t i;
			printf("Got %i images:\n", arraySize);
			for (i=0; i < arraySize; i++)
			{
				json_t *arrayElement = json_array_get(domainsArray, i);
				printf("%i. name=%s ", i+1, json_string_value(json_object_get(arrayElement, "name")));
				printf("progress=%i ", json_integer_value(json_object_get(arrayElement, "progress")));
				printf("status=%s ", json_string_value(json_object_get(arrayElement, "status")));
				printf("created=%s ", json_string_value(json_object_get(arrayElement, "created")));
				printf("id=%s\n", json_string_value(json_object_get(arrayElement, "id")));
			}

			printf("\n");
			if (arraySize>0)
			{
				printf("Select image to spawn for new servers?: ");
				char str[32];
				Gets(str, sizeof(str));
				if (str[0])
				{
					int idx = atoi(str);
					if (idx>=1 && idx <= arraySize)
					{
						json_t *arrayElement = json_array_get(domainsArray, idx-1);
						strcpy(spawningImageId, json_string_value(json_object_get(arrayElement, "id")));
						// Use spawningFlavorId of whatever it was when the server was started, I don't care about flavor
						printf("Using image id %s for new servers\n", spawningImageId);
					}
				}
				else
					printf("Aborted.");
			}

			json_decref(root);
		}
		else if (r2rc==R2RC_GOT_RECORDS)
		{
			json_error_t error;
			json_t *root = json_loads(strstr(responseReceived.C_String() + contentOffset, "{"), JSON_REJECT_DUPLICATES | JSON_DISABLE_EOF_CHECK, &error);
			json_t *domainsArray = json_object_get(root, "records");
			size_t arraySize = json_array_size(domainsArray);
			size_t i;
			for (i=0; i < arraySize; i++)
			{
				json_t *arrayElement = json_array_get(domainsArray, i);
				json_t *elementNameJson = json_object_get(arrayElement, "name");
				const char *elementNameStr = json_string_value(elementNameJson);
				if (strcmp(elementNameStr, patcherHostSubdomainURL)==0)
				{
					json_t *arrayElement = json_array_get(domainsArray, i);
					json_t *elementIPJson = json_object_get(arrayElement, "data");
					strcpy(dnsHostIP, json_string_value(elementIPJson));
					json_t *elementIDJson = json_object_get(arrayElement, "id");
					strcpy(patchHostRecordID,json_string_value(elementIDJson));


					timeOfLastDNSHostCheck=RakNet::GetTime();
					if (appState==AP_TERMINATE_IF_NOT_DNS_HOST)
					{
						if (strcmp(dnsHostIP, rakPeer->GetLocalIP(0))==0)
						{
							// We are dns host. Just keep running
							appState=AP_RUNNING;
						}
						else
						{
							if (timeSinceZeroUsers!=0)
								Terminate();
							else
								appState=AP_TERMINATE_WHEN_ZERO_USERS_REACHED;
						}
					}

					cshState=GOT_RECORDS;
					break;
				}
			}
			if (i==arraySize)
			{
				printf("Can't find %s in record list\n", patcherHostSubdomainURL);
				cshState=CANNOT_FIND_PATCHER_RECORD;
			}
			json_decref(root);
		}
		else if (r2rc==R2RC_GOT_DOMAINS)
		{
			json_error_t error;
			json_t *root = json_loads(strstr(responseReceived.C_String() + contentOffset, "{"), JSON_REJECT_DUPLICATES | JSON_DISABLE_EOF_CHECK, &error);
			json_t *domainsArray = json_object_get(root, "domains");
			size_t arraySize = json_array_size(domainsArray);
			size_t i;
			for (i=0; i < arraySize; i++)
			{
				json_t *arrayElement = json_array_get(domainsArray, i);
				json_t *elementNameJson = json_object_get(arrayElement, "name");
				const char *elementNameStr = json_string_value(elementNameJson);
				if (strcmp(elementNameStr, "raknetpatcher.com")==0)
				{
					// Primary domain
					json_t *idNameJson = json_object_get(arrayElement, "id");
					raknetPatcherDomainId = (int) json_integer_value(idNameJson);
					RakString url("%s/domains/%i/records", rackspaceDNSURL, raknetPatcherDomainId );
					rackspace2->AddOperation(url,Rackspace2::OT_GET,0, true);

					cshState=GETTING_RECORDS;

					break;
				}
			}
			json_decref(root);
			if (i==arraySize)
			{
				printf("Can't find raknetpatcher.com in domain list\n");
				cshState=CANNOT_FIND_PATCHER_DOMAIN;
			}
		}
		else
		{
			if (r2rc==R2RC_UNAUTHORIZED)
				printf("Unauthorized\n");
			else
				printf("General response failure: %s\n", responseReceived.C_String());
		}

	}

	virtual bool UpdateTCP() {
		rackspace2->Update();
		return false;
	}

	void Terminate(void)
	{
		appState=AP_TERMINATING;

		// http://docs.rackspace.com/servers/api/v2/cs-devguide/content/Delete_Server-d1e2883.html
		RakString url("%s/servers", rackspaceServersURL);
		rackspace2->AddOperation(url,Rackspace2::OT_DELETE,0, true);
	}
	void GetCustomImages(void)
	{
		// curl -k https://ord.servers.api.rackspacecloud.com/v2/570016/images?type=SNAPSHOT --header "X-Auth-Token: 97d8335b-7b9e-4a74-96a1-2a3a043c1000" --header "Content-Type: application/json"

		// List images
		// http://docs.rackspace.com/servers/api/v2/cs-devguide/content/List_Images-d1e4435.html
		RakString url("%s/images/detail?type=SNAPSHOT", rackspaceServersURL);
		rackspace2->AddOperation(url,Rackspace2::OT_GET,0, true);
	}
	void ImageThisServer(void)
	{
		// Create image
		// date
		// http://docs.rackspace.com/servers/api/v2/cs-devguide/content/Create_Image-d1e4655.html
		time_t aclock;
		time( &aclock );   // Get time in seconds
		tm *newtime = localtime( &aclock );   // Convert time to struct tm form 
		char text[1024];
		sprintf(text, "%s_%s", patcherHostSubdomainURL, asctime( newtime ));

		RakString url("%s/servers/%s/action", rackspaceServersURL, thisServerDetail.id);
		json_t *jsonObjectRoot = json_object();
		json_t *jsonObjectCreateImage = json_object();
		json_t *jsonMetaData = json_object();
		json_object_set(jsonObjectCreateImage, "name", json_string(text));
		json_object_set(jsonObjectRoot, "createImage", jsonObjectCreateImage);
		json_object_set(jsonObjectCreateImage, "metadata", jsonMetaData);
		json_object_set(jsonMetaData, "ImageType",  json_string("snapshot"));

		printf("Imaging server with name %s\n", text);
		rackspace2->AddOperation(url,Rackspace2::OT_POST,jsonObjectRoot, true);
	}

	void SpawnServers(int count)
	{
		if (count==0)
			return;
		RakAssert(count <= 4);

		// http://docs.rackspace.com/servers/api/v2/cs-devguide/content/CreateServers.html

		RakString url("%s/servers", rackspaceServersURL);

		time_t aclock;
		time( &aclock );   // Get time in seconds
		tm *newtime = localtime( &aclock );   // Convert time to struct tm form 
		char text[1024];
		for (unsigned int i=0; i < count; i++)
		{
			json_t *jsonObjectRoot = json_object();
			json_t *jsonObjectServer = json_object();
			json_object_set(jsonObjectRoot, "server", jsonObjectServer);
			sprintf(text, "%s_%s_%i", patcherHostSubdomainURL, asctime( newtime ), i);
			json_object_set(jsonObjectServer, "name", json_string(text));
			json_object_set(jsonObjectServer, "imageRef", json_string(spawningImageId));
			json_object_set(jsonObjectServer, "flavorRef", json_string(spawningFlavorId));
			rackspace2->AddOperation(url,Rackspace2::OT_POST,jsonObjectRoot, true);
		}
	}


	void GetThisServer(void)
	{
		// http://docs.rackspace.com/servers/api/v2/cs-devguide/content/List_Servers-d1e2078.html
		cshState=GETTING_SERVERS;

		RakString url("%s/servers/detail?status=ACTIVE", rackspaceServersURL);
		//RakString url("%s/servers/detail", rackspaceServersURL);
		rackspace2->AddOperation(url,Rackspace2::OT_GET,0, true);
	}

	void GetDomainRecords(void) {
		cshState=GETTING_DOMAINS;
		RakString url("%s/domains/?name=%s", rackspaceDNSURL, "raknetpatcher.com");
		rackspace2->AddOperation(url,Rackspace2::OT_GET,0, true);
	}

	bool AuthenticateWithRackspaceBlocking(void) {
		rackspace2->SetEventCallback(this);
		rackspace2->Authenticate(rackspaceAuthenticationURL, rackspaceCloudUsername, apiAccessKey);
		cshState=AUTHENTICATING;

		while (cshState==AUTHENTICATING)
		{
			UpdateTCP();
			RakSleep(30);
		}
		if (cshState!=AUTHENTICATED)
			return false;

		GetThisServer();
		while (cshState==GETTING_SERVERS)
		{
			UpdateTCP();
			RakSleep(30);
		}
		if (cshState!=GOT_MY_SERVER)
		{
			return false;
		}

		// http://docs.rackspace.com/cdns/api/v1.0/cdns-devguide/content/list_domains.html
		GetDomainRecords();
		while (cshState==GETTING_DOMAINS || cshState==GETTING_RECORDS)
		{
			UpdateTCP();
			RakSleep(30);
		}
		if (cshState!=GOT_RECORDS)
			return false;

		return true;
	}
	void UpdateHostIPAsynch(void) {
		if (appState!=AP_RUNNING)
			return;

		// RakString url("%s/domains/domainId/records/recordId");
		// rackspace2->AddOperation()

		// http://docs.rackspace.com/cdns/api/v1.0/cdns-devguide/content/Modify_Records-d1e5033.html
		//rackspace2;

		RakString url("%s/domains/%i/records/%s", rackspaceDNSURL, raknetPatcherDomainId, patchHostRecordID);

		json_t *jsonObject = json_object();
		json_object_set(jsonObject, "data", json_string(rakPeer->GetLocalIP(0)));
		printf("Setting DNS to %s\n", rakPeer->GetLocalIP(0));
		// TESTING HACK
		// json_object_set(jsonObject, "data", json_string("198.61.202.20"));
		json_object_set(jsonObject, "name", json_string(patcherHostSubdomainURL));
		rackspace2->AddOperation(url,Rackspace2::OT_PUT,jsonObject, true);
	}
	
	virtual void PrintHelp(void)
	{
		printf("Distributed authenticated CloudServer using DNS based host migration.\n");
		printf("Query running servers with CloudClient::Get() on key CloudServerList,0\n\n");
		printf("Query load with key CloudConnCount,0. Read row data as unsigned short.\n\n");
		printf("Usage:\n");
		printf("CloudServer.exe S2SPWD [Port] [ConnIn] [ConnOut]\n\n");
		printf("Parameters:\n");
		printf("authenticationURL - See http://docs.rackspace.com/cdns/api/v1.0/cdns-devguide/content/Authentication-d1e647.html\n");
		printf("dnsURL - See http://docs.rackspace.com/cdns/api/v1.0/cdns-devguide/content/Service_Access_Endpoints-d1e753.html\n");
		printf("Username - Rackspace username.\n");
		printf("APIAccessKey - Rackspace API Access key, from the control panel.\n");
		printf("S2SPWD - Server to server password for servers on the cloud.\n");
		printf("Port - RakPeer listen port. Default is 60000\n");
		printf("ConnIn - Max incoming connections for clients. Default is 1024\n");
		printf("ConnIn - Max outgoing connections, used for server to server. Default 128\n\n");
		printf("Example:\n");
		printf("AutopatcherServer_SelfScaling.exe randomServerPassword1234 60000 1024 128 \n\n");
	}
	virtual bool ParseCommandLineParameters(int argc, char **argv) {

		if (argc==1)
			PrintHelp();

		const unsigned short DEFAULT_SERVER_PORT=60000;
		const unsigned short DEFAULT_ALLOWED_INCOMING_CONNECTIONS=1024;
		const unsigned short DEFAULT_ALLOWED_OUTGOING_CONNECTIONS=128;

		serverToServerPassword=argv[1];

		if (argc<3) rakPeerPort=DEFAULT_SERVER_PORT;
		else rakPeerPort=atoi(argv[2]);

		if (argc<4) allowedIncomingConnections=DEFAULT_ALLOWED_INCOMING_CONNECTIONS;
		else allowedIncomingConnections=atoi(argv[3]);

		if (argc<5) allowedOutgoingConnections=DEFAULT_ALLOWED_OUTGOING_CONNECTIONS;
		else allowedOutgoingConnections=atoi(argv[4]);

		FILE *fp;
		fp = fopen("patcherHostSubdomainURL.txt", "rb");
		if (fp)
		{
			fgets(patcherHostSubdomainURL,128,fp);
			fclose(fp);
		}
		else
		{
			// Example: mygame.raknetpatcher.com
			printf("Enter patcher host subdomain URL: ");
			Gets(patcherHostSubdomainURL, sizeof(patcherHostSubdomainURL));
			if (patcherHostSubdomainURL[0]==0)
				return false;
		}
		
		fp = fopen("patcherHostDomainURL.txt", "rb");
		if (fp)
		{
			fgets(patcherHostDomainURL,128,fp);
			fclose(fp);
		}
		else
		{
			// Example: raknetpatcher.com
			printf("Enter patcher host domain URL: ");
			Gets(patcherHostDomainURL, sizeof(patcherHostDomainURL));
			if (patcherHostDomainURL[0]==0)
				return false;
		}

		fp = fopen("rackspaceAuthenticationURL.txt", "rb");
		if (fp)
		{
			fgets(rackspaceAuthenticationURL,128,fp);
			fclose(fp);
		}
		else
		{
			// See http://docs.rackspace.com/cdns/api/v1.0/cdns-devguide/content/Authentication-d1e647.html
			// Example: https://identity.api.rackspacecloud.com/v2.0
			printf("Enter Rackspace authentication URL: ");
			Gets(rackspaceAuthenticationURL, sizeof(rackspaceAuthenticationURL));
			if (rackspaceAuthenticationURL[0]==0)
				return false;
		}

		/*
		fp = fopen("rackspaceServersURL.txt", "rb");
		if (fp)
		{
			fgets(rackspaceServersURL,128,fp);
			fclose(fp);
		}
		else
		{
			
			printf("Enter Rackspace Servers URL: ");
			Gets(rackspaceServersURL, sizeof(rackspaceServersURL));
			if (rackspaceServersURL[0]==0)
				return false;
		}
		*/
	
		fp = fopen("rackspaceCloudUsername.txt", "rb");
		if (fp)
		{
			fgets(rackspaceCloudUsername,128,fp);
			fclose(fp);
		}
		else
		{
			// This is what you signed up to Rackspace with
			printf("Enter Rackspace username: ");
			Gets(rackspaceCloudUsername, sizeof(rackspaceCloudUsername));
			if (rackspaceCloudUsername[0]==0)
				return false;
		}

		fp = fopen("apiAccessKey.txt", "rb");
		if (fp)
		{
			fgets(apiAccessKey,128,fp);
			fclose(fp);
		}
		else
		{
			// You get this from your Rackspace control panel
			printf("Enter Rackspace API Access key: ");
			Gets(apiAccessKey, sizeof(apiAccessKey));
			if (apiAccessKey[0]==0)
				return false;
		}


		return true;
	}
	// Call when you get ID_FCM2_NEW_HOST
	virtual void OnFCMNewHost(Packet *packet, RakPeerInterface *rakPeer) {
		RakAssert(packet->data[0]==ID_FCM2_NEW_HOST);
		RakNet::BitStream bsIn(packet->data, packet->length, false);
		bsIn.IgnoreBytes(sizeof(MessageID));
		RakNetGUID oldHost;
		bsIn.Read(oldHost);
		RakNetGUID newHost = packet->guid;
		if (newHost==rakPeer->GetMyGUID() && oldHost!=newHost)
		{
			printf("Assuming host. Updating DNS\n");

			UpdateHostIPAsynch();
		}
	}

	const char *GetCloudHostAddress(void) const {return dnsHostIP;}
	RakNet::Time GetTimeOfLastDNSHostCheck(void) {return timeOfLastDNSHostCheck;}
	void SetTimeOfLastDNSHostCheck(RakNet::Time t) {timeOfLastDNSHostCheck=t;}

	virtual int OnJoinCloudResult(
		Packet *packet,
		RakNet::RakPeerInterface *rakPeer,
		RakNet::CloudServer *cloudServer,
		RakNet::CloudClient *cloudClient,
		RakNet::FullyConnectedMesh2 *fullyConnectedMesh2,
		RakNet::TwoWayAuthentication *twoWayAuthentication,
		RakNet::ConnectionGraph2 *connectionGraph2,
		const char *rakPeerIpOrDomain,
		char myPublicIP[32]
	) {
		if (packet->data[0]==ID_CONNECTION_ATTEMPT_FAILED)
		{
			printf("Failed connection. Changing DNS to point to this system.\n");

			UpdateHostIPAsynch();
		}

		return CloudServerHelper::OnJoinCloudResult(packet, rakPeer, cloudServer, cloudClient, fullyConnectedMesh2, twoWayAuthentication, connectionGraph2, rakPeerIpOrDomain, myPublicIP);
	}
	char patcherHostSubdomainURL[128];
	char patcherHostDomainURL[128];
	char dnsHostIP[128];
	char rackspaceAuthenticationURL[128];
	char rackspaceDNSURL[128];
	char rackspaceServersURL[128];
	char rackspaceCloudUsername[128];
	char apiAccessKey[128];
	Rackspace2 *rackspace2;
	char patchHostRecordID[128];
protected:

	int raknetPatcherDomainId;
	RakNet::Time timeOfLastDNSHostCheck;

	struct ServerDetail
	{
		char publicIPV4[32];
		char privateIPV4[32];
		char flavorId[128];
		char hostId[128];
		char id[128];
		char imageId[128];
		char name[128];
	} thisServerDetail;

	char spawningImageId[128];
	char spawningFlavorId[128];

} *cloudServerHelper;


class AutopatcherLoadNotifier : public RakNet::AutopatcherServerLoadNotifier_Printf
{
	void AutopatcherLoadNotifier::OnQueueUpdate(SystemAddress remoteSystem,
		AutopatcherServerLoadNotifier::RequestType requestType,
		AutopatcherServerLoadNotifier::QueueOperation queueOperation,
		AutopatcherServerLoadNotifier::AutopatcherState *autopatcherState)
	{
		AutopatcherServerLoadNotifier_Printf::OnQueueUpdate(remoteSystem, requestType, queueOperation, autopatcherState);
	}

	void AutopatcherLoadNotifier::OnGetChangelistCompleted(
		SystemAddress remoteSystem,
		AutopatcherServerLoadNotifier::GetChangelistResult getChangelistResult,
		AutopatcherServerLoadNotifier::AutopatcherState *autopatcherState)
	{
		AutopatcherServerLoadNotifier_Printf::OnGetChangelistCompleted(remoteSystem, getChangelistResult, autopatcherState);
	}

	void AutopatcherLoadNotifier::OnGetPatchCompleted(SystemAddress remoteSystem,
		AutopatcherServerLoadNotifier::PatchResult patchResult,
		AutopatcherServerLoadNotifier::AutopatcherState *autopatcherState)
	{
		AutopatcherServerLoadNotifier_Printf::OnGetPatchCompleted(remoteSystem, patchResult, autopatcherState);

		if (autopatcherState->requestsWorking==0)
		{
			if (autopatcherState->requestsQueued==0)
				timeSinceZeroUsers=RakNet::GetTime();

			if (appState==AP_TERMINATE_WHEN_ZERO_USERS_REACHED)
			{
				cloudServerHelper->Terminate();
			}
		}
		else
		{
			timeSinceZeroUsers=0;
		}
	}
};

void SetMaxConcurrentUsers(int i)
{
	autopatcherServer->SetMaxConurrentUsers(i);
}

int main(int argc, char **argv)
{
#if OPEN_SSL_CLIENT_SUPPORT==0
#error RakNet must be compiled with OPEN_SSL_CLIENT_SUPPORT=1 to connect to Rackspace
	return 0;
#endif

	cloudServerHelper = RakNet::OP_NEW<CloudServerHelper_RackspaceCloudDNS>(_FILE_AND_LINE_);
	if (!cloudServerHelper->ParseCommandLineParameters(argc, argv))
	{
		return 1;
	}

	rakPeer = RakNet::RakPeerInterface::GetInstance();
	RakNet::PacketizedTCP packetizedTCP;
	if (packetizedTCP.Start(LISTEN_PORT_TCP_PATCHER,cloudServerHelper->allowedIncomingConnections)==false)
	{
		printf("Failed to start TCP. Is the port already in use?");
		return 1;
	}

	char databasePassword[128];
	FILE *fp = fopen("databasePassword.txt", "rb");
	if (fp)
	{
		fgets(databasePassword,128,fp);
		fclose(fp);
	}
	else
	{
		// This is what you signed up to Rackspace with
		printf("Enter database password: ");
		Gets(databasePassword, sizeof(databasePassword));
		if (databasePassword[0]==0)
			return false;
	}


	int workerThreadCount;
	int sqlConnectionObjectCount;

	if (argc<7) workerThreadCount=4;
	else workerThreadCount=atoi(argv[6]);

	if (argc<8) sqlConnectionObjectCount=8;
	else sqlConnectionObjectCount=atoi(argv[7]);

	int allowDownloadingUnmodifiedFiles;
	if (argc<9) allowDownloadingUnmodifiedFiles=0;
	else allowDownloadingUnmodifiedFiles=atoi(argv[8]);

	RakNet::Time timeSinceLastLoadCheck=RakNet::GetTime();

	if (!cloudServerHelper->AuthenticateWithRackspaceBlocking())
	{
		printf("Exiting due to failed startup with Rackspace\n");
		return 1;
	}

	appState=AP_RUNNING;
	timeSinceZeroUsers=0;
	printf("Server starting... ");
	autopatcherServer = RakNet::OP_NEW<AutopatcherServer>(_FILE_AND_LINE_);
	// RakNet::FLP_Printf progressIndicator;
	RakNet::FileListTransfer fileListTransfer;
	RakNet::AutopatcherPostgreRepository *connectionObject;
	connectionObject = RakNet::OP_NEW_ARRAY<AutopatcherPostgreRepository>(sqlConnectionObjectCount, _FILE_AND_LINE_);
	// RakNet::AutopatcherRepositoryInterface **connectionObjectAddresses[sqlConnectionObjectCount];
	AutopatcherRepositoryInterface **connectionObjectAddresses = RakNet::OP_NEW_ARRAY<AutopatcherRepositoryInterface *>(sqlConnectionObjectCount, _FILE_AND_LINE_);

	for (int i=0; i < sqlConnectionObjectCount; i++)
		connectionObjectAddresses[i]=&connectionObject[i];
	autopatcherServer->SetFileListTransferPlugin(&fileListTransfer);
	// PostgreSQL is fast, so this may not be necessary, or could use fewer threads
	// This is used to read increments of large files concurrently, thereby serving users downloads as other users read from the DB
	fileListTransfer.StartIncrementalReadThreads(sqlConnectionObjectCount);
	autopatcherServer->SetMaxConurrentUsers(workerThreadCount); // More users than this get queued up
	AutopatcherLoadNotifier autopatcherLoadNotifier;
	autopatcherServer->SetLoadManagementCallback(&autopatcherLoadNotifier);
	packetizedTCP.AttachPlugin(autopatcherServer);
	packetizedTCP.AttachPlugin(&fileListTransfer);

	autopatcherServer->SetAllowDownloadOfOriginalUnmodifiedFiles(allowDownloadingUnmodifiedFiles!=0);

	// ---- PLUGINS -----
	// Used to load balance clients, allow for client to client discovery
	RakNet::CloudServer cloudServer;
	// Used to update the local cloudServer
	RakNet::CloudClient cloudClient;
	// Used to determine the host of the server fully connected mesh, as well as to connect servers automatically
	RakNet::FullyConnectedMesh2 fullyConnectedMesh2;
	// Used for servers to verify each other - otherwise any system could pose as a server
	// Could also be used to verify and restrict clients if paired with the MessageFilter plugin
	RakNet::TwoWayAuthentication twoWayAuthentication;
	// Used to tell servers about each other
	RakNet::ConnectionGraph2 connectionGraph2;

	rakPeer->AttachPlugin(&cloudServer);
	rakPeer->AttachPlugin(&cloudClient);
	rakPeer->AttachPlugin(&fullyConnectedMesh2);
	rakPeer->AttachPlugin(&twoWayAuthentication);
	rakPeer->AttachPlugin(&connectionGraph2);
	
	if (!cloudServerHelper->StartRakPeer(rakPeer))
		return 1;

	RakNet::CloudServerHelperFilter sampleFilter; // Keeps clients from updating stuff to the server they are not supposed to
	sampleFilter.serverGuid=rakPeer->GetMyGUID();
	cloudServerHelper->SetupPlugins(&cloudServer, &sampleFilter, &cloudClient, &fullyConnectedMesh2, &twoWayAuthentication,&connectionGraph2, cloudServerHelper->serverToServerPassword);

	int ret;
	do 
	{
		ret = cloudServerHelper->JoinCloud(rakPeer, &cloudServer, &cloudClient, &fullyConnectedMesh2, &twoWayAuthentication, &connectionGraph2, cloudServerHelper->GetCloudHostAddress());
	} while (ret==2);
	if (ret==1)
		return 1;

	printf("started.\n");

	printf("Enter database password:\n");
	char connectionString[256];
	char username[256];
	bool connectionSuccess=false;
	while (connectionSuccess==false)
	{
		strcpy(username, "postgres");
		strcpy(connectionString, "user=");
		strcat(connectionString, username);
		strcat(connectionString, " password=");
		strcat(connectionString, databasePassword);
		int conIdx;
		for (conIdx=0; conIdx < sqlConnectionObjectCount; conIdx++)
		{
			if (connectionObject[conIdx].Connect(connectionString)==false)
			{
				printf("Database connection failed.\n");
				printf("Try a different password? (y/n)\n");
				if (getch()!='y')
				{
					return 1;
				}
				else
				{
					connectionSuccess=false;
					printf("Enter password: ");
					Gets(databasePassword, sizeof(databasePassword));
					break;
				}
			}
		}
		if (conIdx==sqlConnectionObjectCount)
			connectionSuccess=true;
	}

	printf("Database connection suceeded.\n");
	printf("Starting threads\n");
	// 4 Worker threads, which is CPU intensive
	// A greater number of SQL connections, which read files incrementally for large downloads
	autopatcherServer->StartThreads(workerThreadCount,sqlConnectionObjectCount, connectionObjectAddresses);
	autopatcherServer->CacheMostRecentPatch(0);
	printf("System ready for connections\n");

	printf("(D)rop database\n(C)reate database.\n(U)pdate revision.\n(S)pawn a clone of this server\n(M)ax concurrent users (this server only)\n(I)mage the current state of this server\n(L)ist images\n(T)erminate cloud\n(Q)uit\n");

	char ch;
	RakNet::Packet *p;
	while (appState!=AP_TERMINATED)
	{
		RakNet::SystemAddress notificationAddress;
		notificationAddress=packetizedTCP.HasCompletedConnectionAttempt();
		if (notificationAddress!=RakNet::UNASSIGNED_SYSTEM_ADDRESS)
			printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
		notificationAddress=packetizedTCP.HasNewIncomingConnection();
		if (notificationAddress!=RakNet::UNASSIGNED_SYSTEM_ADDRESS)
			printf("ID_NEW_INCOMING_CONNECTION\n");
		notificationAddress=packetizedTCP.HasLostConnection();
		if (notificationAddress!=RakNet::UNASSIGNED_SYSTEM_ADDRESS)
			printf("ID_CONNECTION_LOST\n");

		p=packetizedTCP.Receive();
		while (p)
		{
			packetizedTCP.DeallocatePacket(p);
			p=packetizedTCP.Receive();
		}

		p=rakPeer->Receive();
		while (p)
		{
			if (p->data[0]==ID_NEW_INCOMING_CONNECTION)
				printf("ID_NEW_INCOMING_CONNECTION (TCP) from %s\n", p->systemAddress.ToString(true));
			else if (p->data[0]==ID_DISCONNECTION_NOTIFICATION)
				printf("ID_DISCONNECTION_NOTIFICATION (TCP) from %s\n", p->systemAddress.ToString(true));
			else if (p->data[0]==ID_CONNECTION_LOST)
				printf("ID_CONNECTION_LOST (TCP) from %s\n", p->systemAddress.ToString(true));
			else if (p->data[0]==ID_FCM2_NEW_HOST)
			{
				if (appState==AP_RUNNING)
				{
					RakNet::BitStream bs(p->data,p->length,false);
					bs.IgnoreBytes(1);
					RakNetGUID oldHost;
					bs.Read(oldHost);

					if (p->guid==rakPeer->GetMyGUID())
					{
						if (oldHost!=UNASSIGNED_RAKNET_GUID)
						{
							printf("ID_FCM2_NEW_HOST: Taking over as host from the old host.\n");
						}
						else
						{
							// Room not hosted if we become host the first time since this was done in CreateRoom() already
							printf("ID_FCM2_NEW_HOST: We have become host for the first time.\n");
						}
					}

					timeSinceLastLoadCheck=RakNet::GetTime();
				}
			}
			else if (p->data[0]==ID_USER_PACKET_ENUM)
			{
				RakNet::BitStream bsIn(p->data, p->length, false);
				bsIn.IgnoreBytes((sizeof(MessageID)));
				RakString incomingPw;
				bsIn.Read(incomingPw);
				if (incomingPw==cloudServerHelper->serverToServerPassword)
				{
					SetMaxConcurrentUsers(0);

					if (appState==AP_RUNNING)
					{
						if (timeSinceZeroUsers!=0)
							cloudServerHelper->Terminate();
						else
							appState=AP_TERMINATE_WHEN_ZERO_USERS_REACHED;
					}

					// Disconnect from the cloud, and do not accept new connections from users
					// The autopatcher will still run since it is on TCP
					rakPeer->Shutdown(1000);
				}

			}
			else if (p->data[0]==ID_CLOUD_GET_RESPONSE)
			{
				RakNet::CloudQueryResult cloudQueryResult;
				cloudClient.OnGetReponse(&cloudQueryResult, p);
				unsigned int rowIndex;
				const bool wasCallToGetServers=cloudQueryResult.cloudQuery.keys[0].primaryKey=="CloudConnCount";
				RakAssert(wasCallToGetServers);
				/*
				if (wasCallToGetServers)
					printf("Downloaded server list. %i servers.\n", cloudQueryResult.rowsReturned.Size());
					*/

				unsigned short connectionsOnOurServer=65535;
				unsigned short lowestConnectionsServer=65535;
				RakNet::SystemAddress lowestConnectionAddress;

				int totalConnections=0;
				for (rowIndex=0; rowIndex < cloudQueryResult.rowsReturned.Size(); rowIndex++)
				{
					RakNet::CloudQueryRow *row = cloudQueryResult.rowsReturned[rowIndex];

					unsigned short connCount;
					RakNet::BitStream bsIn(row->data, row->length, false);
					bsIn.Read(connCount);
					printf("%i. Server found at %s with %i connections\n", rowIndex+1, row->serverSystemAddress.ToString(true), connCount);

					totalConnections+=connCount;
				}

				const int MAX_SERVERS_EVER=32;
				int available = cloudQueryResult.rowsReturned.Size() * sqlConnectionObjectCount - totalConnections;
				if (available < 0 &&
					cloudQueryResult.rowsReturned.Size() < MAX_SERVERS_EVER // no more than MAX_SERVERS_EVER servers ever, to control costs
					)
				{
					int newServersNeeded = -available / sqlConnectionObjectCount;
					if (newServersNeeded > 4)
						newServersNeeded = 4; // Do not start more than 4 servers at a time
					if (cloudQueryResult.rowsReturned.Size() + newServersNeeded > MAX_SERVERS_EVER)
					{
						newServersNeeded = MAX_SERVERS_EVER - cloudQueryResult.rowsReturned.Size();
					}
					if (newServersNeeded>0)
						cloudServerHelper->SpawnServers(newServersNeeded);
				}

				cloudClient.DeallocateWithDefaultAllocator(&cloudQueryResult);
			}

			cloudServerHelper->OnPacket(p, rakPeer, &cloudClient, &cloudServer, &fullyConnectedMesh2, &twoWayAuthentication, &connectionGraph2);

			rakPeer->DeallocatePacket(p);
			p=rakPeer->Receive();
		}

		if (timeSinceZeroUsers!=0 && appState==AP_RUNNING && autopatcherServer->GetMaxConurrentUsers()>0)
		{
			// No users currently connected
			RakNet::Time curTime = RakNet::GetTime();
			RakNet::Time diff = timeSinceZeroUsers - curTime;
			if (diff > 0)
			{
				if (fullyConnectedMesh2.IsHostSystem()==false)
				{
					// 2. Shutdown automatically if no users for 1 hour, except if we are the host
					if (diff > 1000 * 60 * 60)
						cloudServerHelper->Terminate();
				}
				else
				{
					// We are host according to RakPeer. But no users for 24 hours
					// Verify we are the host according to Rackspace DNS records. If not, shutdown
					if (diff > 1000 * 60 * 60 * 24)
					{
						// No users for 24 hours, verify if we are the host according to Rackspace DNS records. If not, shutdown
						RakNet::Time diff2;
						diff2 = curTime - cloudServerHelper->GetTimeOfLastDNSHostCheck();
						if (diff2 > 1000 * 60 * 60 * 24)
						{
							appState=AP_TERMINATE_IF_NOT_DNS_HOST;
							cloudServerHelper->SetTimeOfLastDNSHostCheck(curTime);
							cloudServerHelper->GetDomainRecords();
						}
					}
				}
			}
		}

		cloudServerHelper->Update();

		
		// 4. If we are the host, every 1 hour check load among all servers (including self). It takes like 30 minutes to build a new server.
		if (fullyConnectedMesh2.IsHostSystem() && autopatcherServer->GetMaxConurrentUsers()>0)
		{
			RakNet::Time curTime = RakNet::GetTime();
			RakNet::Time diff = curTime - timeSinceLastLoadCheck;
			if (diff > 0 && diff > 1000 * 60 * 60)
			{
				timeSinceLastLoadCheck = curTime;

				// If the load exceeds such that >=1 new fully loaded server is needed, add that many servers
				// See ID_CLOUD_GET_RESPONSE
				RakNet::CloudQuery cloudQuery;
				cloudQuery.keys.Push(RakNet::CloudKey("CloudConnCount",0),_FILE_AND_LINE_); // CloudConnCount is defined at the top of CloudServerHelper.cpp
				cloudQuery.subscribeToResults=false;
				cloudClient.Get(&cloudQuery, rakPeer->GetMyGUID());
			}
		}

		if (kbhit())
		{
			ch=getch();
			if (ch=='q')
				break;
			else if (ch=='c')
			{
				if (connectionObject[0].CreateAutopatcherTables()==false)
					printf("%s", connectionObject[0].GetLastError());
				else
					printf("Created tables.\n");

				if (connectionObject[0].AddApplication(cloudServerHelper->patcherHostSubdomainURL, username)==false)
					printf("%s", connectionObject[0].GetLastError());
				else
					printf("Added application\n");
			}
			else if (ch=='d')
			{
                if (connectionObject[0].DestroyAutopatcherTables()==false)
					printf("%s", connectionObject[0].GetLastError());
			}
			
			else if (ch=='i')
			{
				printf("Image this server?\nWill clone this server so that future servers made from this image will use its current state.\nPress 'y' to image.\n");
				if (getch()=='y')
				{
					cloudServerHelper->ImageThisServer();
				}
			}
			else if (ch=='s')
			{
				printf("How many servers to spawn? (0-4)");
				char str[32];
				Gets(str, sizeof(str));
				int num = atoi(str);
				if (num>0)
					cloudServerHelper->SpawnServers(num);
			}
			else if (ch=='m')
			{
				char maxConcurrent[64];
				printf("Enter new allowed max concurrent users: ");
				Gets(maxConcurrent, sizeof(maxConcurrent));
				if (maxConcurrent[0])
					SetMaxConcurrentUsers(atoi(maxConcurrent));
			}
			else if (ch=='t')
			{
				printf("Setting max concurrent users to 0 on all servers\n");
				SetMaxConcurrentUsers(0);

				BitStream bsOut;
				bsOut.Write((MessageID)ID_USER_PACKET_ENUM);
				bsOut.Write(cloudServerHelper->serverToServerPassword);
				DataStructures::List<RakNetGUID> remoteServersOut;
				cloudServer.GetRemoteServers(remoteServersOut);
				for (unsigned int i=0; i < remoteServersOut.Size(); i++)
				{
					rakPeer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, remoteServersOut[i], false);
					rakPeer->CloseConnection(remoteServersOut[i], true);
				}

				printf("Remote servers will shutdown when all users are done\n");
				printf("Disconnected from remote servers\n");
				printf("Updating host DNS entry to this server\n");
				cloudServerHelper->UpdateHostIPAsynch();
				
			}
			else if (ch=='l')
			{
				cloudServerHelper->GetCustomImages();
			}
			else if (ch=='u')
			{
				// printf("Enter application name: ");
				char appName[512];
				strcpy(appName, cloudServerHelper->patcherHostSubdomainURL);
// 				Gets(appName,sizeof(appName));
// 				if (appName[0]==0)
// 					strcpy(appName, "TestApp");

				printf("Enter application directory: ");
				char appDir[512];
				Gets(appDir,sizeof(appDir));
				if (appDir[0]==0)
					strcpy(appDir, "D:/temp");

				if (connectionObject[0].UpdateApplicationFiles(appName, appDir, username, 0)==false)
				{
					printf("%s", connectionObject[0].GetLastError());
				}
				else
				{
					printf("Update success.\n");
					autopatcherServer->CacheMostRecentPatch(appName);
				}
			}
		}

		RakSleep(30);
	}



	RakNet::OP_DELETE_ARRAY(connectionObject, _FILE_AND_LINE_);
	RakNet::OP_DELETE_ARRAY(connectionObjectAddresses, _FILE_AND_LINE_);
	packetizedTCP.Stop();
	RakNet::RakPeerInterface::DestroyInstance(rakPeer);
	RakNet::OP_DELETE(cloudServerHelper, _FILE_AND_LINE_);


return 0;
}
