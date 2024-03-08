#include "UPNPPortForwarder.h"

UPNPPortForwarder::UPNPPortForwarder(void)
{
	threadNumber=0;
}

UPNPPortForwarder::~UPNPPortForwarder(void)
{
}

struct UPNPPortForwarder::UPNPParamDesc UPNPPortForwarder::CreateParamDesc(int index,RakNet::RakString name,RakNet::RakString type, RakNet::RakString subType)
{
	struct UPNPParamDesc currentDesc;

	currentDesc.type=type;
	currentDesc.subType=subType;
	currentDesc.name=name;
	currentDesc.index=index;

	return currentDesc;
}

struct UPNPPortForwarder::StandardUPNPParams * UPNPPortForwarder::CreateNewParamList()
{
	struct StandardUPNPParams *params= new struct StandardUPNPParams;
	params->currentIntIndex=0;
	params->currentStringIndex=0;
	params->currentPointerIndex=0;

	return params;

}
unsigned int UPNPPortForwarder::QueryUPNPSupport(UPNPCallBackInterface *callBack)
{
	// Tyler TODO
	// Callback NoRouterFoundOnAnyInterface or RouterFound
	callBack->RouterFound(0);
	return 0;
}
unsigned int UPNPPortForwarder::OpenPortOnInterface(int internalPort,UPNPCallBackInterface *callBack,RakNet::RakString interfaceIp,int portToOpenOnRouter, RakNet::RakString mappingName, ProtocolType protocol)
{

	struct StandardUPNPParams *params=CreateNewParamList();
	threadNumber++;
	SetCommonParams( params,threadNumber,internalPort,callBack,  portToOpenOnRouter, mappingName,  protocol);


	if (interfaceIp!="ALL")
	{
		AddParam (&interfaceIp,params,"interfaceIp");	
	}

	RakNet::RakThread::Create(RunOpenPortOnAllInterfacesOrInterfaceThread,params);






	return threadNumber;

}


void UPNPPortForwarder::SetCommonParams(struct StandardUPNPParams* params,int threadNumber,int internalPort,UPNPCallBackInterface *callBack, int portToOpenOnRouter,RakNet::RakString mappingName, ProtocolType protocol)
{

	RakNet::RakString protocolString;
	seedMT((unsigned int)RakNet::GetTime()+threadNumber);

	if (internalPort<=0)//Zero is valid, but indicates a random port, I doubt it is what is wanted in this situation
	{
		internalPort=1;

	}

	if (internalPort>65535)
	{
		internalPort=65535;
	}


	if (portToOpenOnRouter<=0||portToOpenOnRouter>65535)//Zero is valid, but indicates a random port, I doubt it is what is wanted in this situation
	{
		portToOpenOnRouter=internalPort;
	}

	if (mappingName=="")
	{
		mappingName="UPNPRAKNET";

		for (int i=0;i<3;i++)
			mappingName+=(char)(randomMT()%26+65);

	}




	if (protocol==TCP)
	{
		protocolString="TCP";
	}
	else
	{
		protocolString="UDP";

	}




	//Push pointer
	AddParam (callBack,params,"callBack","pointer","UPNPCallBackInterface");

	//Push ints
	AddParam (&internalPort,params,"internalPort","int");
	AddParam (&portToOpenOnRouter,params,"portToOpenOnRouter","int");	
	AddParam (&threadNumber,params,"threadNumber","int");	




	//Push strings
	AddParam (&mappingName,params,"mappingName");
	AddParam (&protocolString,params,"protocol");	

}






RAK_THREAD_DECLARATION(UPNPPortForwarder::RunOpenPortOnAllInterfacesOrInterfaceThread)
{

	struct StandardUPNPParams *params=(struct StandardUPNPParams *)arguments;

	UPNPCallBackInterface *callBack=(UPNPCallBackInterface *)params->pointerList[0];


	int internalPort=params->intList[0];
	int portToOpenOnRouter=params->intList[1];
	int threadNum=params->intList[2];
	RakNet::RakString mappingName=params->stringList[0];
	RakNet::RakString protocol=params->stringList[1];
	bool specificInterface=false;
	RakNet::RakString interfaceIp="";
	if (params->stringList.Size()==3)
	{
		if (params->stringList[2].GetLength()<=16)
		{
			specificInterface=true;
			interfaceIp=params->stringList[2];
		}
	}

	bool foundOne=false;

	bool success=false;
	UPNPNATInternal* nat;
	char ipList[99][16];
	unsigned int binaryAddresses[99];
	bool wasReleased=false;

	if (callBack!=NULL)
	{
		callBack->Lock();
		callBack->SetReleaseTracker(wasReleased);
		callBack->UnLock();
	}

	if (!specificInterface)
	{
		if (callBack!=NULL&&!wasReleased)
		{
			callBack->Lock();
			if (!wasReleased)
			{
				callBack->UPNPPrint("Opening ports on all interfaces, if at least one success then pass.");
				callBack->UnLock();
			}
		}
	}
	else
	{
		if (callBack!=NULL&&!wasReleased)
		{
			callBack->Lock();
			if (!wasReleased)
			{
				callBack->UPNPPrint("Opening ports on interface.");
				callBack->UnLock();
			}
		}
	}


	if (!specificInterface)
	{
		SocketLayer::Instance()->GetMyIP(ipList,binaryAddresses);
	}
	else
	{
		strcpy(ipList[0],interfaceIp.C_String());
		ipList[1][0]=0;
	}


	for (int i=0;ipList[i][0]!=0;i++)
	{

		if (strcmp(ipList[i],"127.0.0.1")==0)//Skip localhost
		{

			if (callBack!=NULL&&!wasReleased)
			{
				callBack->Lock();
				if (!wasReleased)
				{
					callBack->UPNPPrint("Skipping localhost");
					callBack->UnLock();
				}
			}
			continue;
		}
		if (i>=99)
			break;
		if (callBack!=NULL&&!wasReleased)
		{
			callBack->Lock();
			if (!wasReleased)
			{
				callBack->UPNPPrint(RakNet::RakString ("Ip: %s",ipList[i]));
				callBack->UnLock();
			}
		}
		nat=new UPNPNATInternal(ipList[i]);



		if(!nat->Discovery()){
			if (callBack!=NULL&&!wasReleased)
			{
				callBack->Lock();
				if (!wasReleased)
				{
					callBack->UPNPPrint(RakNet::RakString ("Discovery for interface %s error is %s",ipList[i],nat->GetLastError()));
					callBack->UnLock();
				}
			}
			delete nat;
			continue;
		}
		foundOne=true;
		if(!nat->AddPortMapping(mappingName.C_String(),ipList[i],portToOpenOnRouter,internalPort,(char *)protocol.C_String())){
			if (callBack!=NULL&&!wasReleased)
			{
				callBack->Lock();
				if (!wasReleased)
				{
					callBack->UPNPPrint(RakNet::RakString ("Add port mapping  error is %s for %s",nat->GetLastError(),ipList[i]));
					callBack->UnLock();
				}
			}
			delete nat;
			continue;
		}

		if (callBack!=NULL&&!wasReleased)
		{
			callBack->Lock();
			if (!wasReleased)
			{
				callBack->UPNPPrint(RakNet::RakString ("Add port mapping success for %s",ipList[i]));
				callBack->UnLock();
			}
			success=true;
		}
		delete nat;
	}


	if (success)
	{
		if (callBack!=NULL&&!wasReleased)
		{
			callBack->Lock();
			if (!wasReleased)
			{
				callBack->UPNPPrint("Success");
				callBack->UnLock();
			}
		}

	}
	else
	{
		if (callBack!=NULL&&!wasReleased)
		{
			callBack->Lock();
			if (!wasReleased)
			{
				callBack->UPNPPrint("Fail");
				callBack->UnLock();
			}
		}

		if (callBack!=NULL&&!wasReleased)
		{
			if (foundOne)
			{
				callBack->Lock();
				if (!wasReleased)
				{
					callBack->NoPortOpenedOnAnyInterface(threadNum,internalPort,portToOpenOnRouter);
					callBack->UnLock();
				}
			}
			else
			{
				callBack->Lock();
				if (!wasReleased)
				{
					callBack->NoRouterFoundOnAnyInterface(threadNum,internalPort,portToOpenOnRouter);
					callBack->UnLock();
				}
			}
		}
	}
	if (callBack!=NULL&&!wasReleased)
	{
		callBack->Lock();
		if (!wasReleased)
		{
			callBack->ThreadFinished(threadNum,internalPort,portToOpenOnRouter,success);
			callBack->UnLock();
		}
	}


	if (callBack!=NULL&&!wasReleased)
	{
		callBack->Lock();
		if (!wasReleased)
		{
			callBack->RemoveReleaseTracker(wasReleased);
			callBack->UnLock();
		}
	}
	delete params;

	return 0;
}



void UPNPPortForwarder::AddParam (void* inParam,struct StandardUPNPParams* params,RakNet::RakString name,RakNet::RakString type, RakNet::RakString subType)
{

	if (type=="RakString")
	{
		params->stringList.Push(*((RakNet::RakString*)inParam),__FILE__,__LINE__);
		params->descList.Push(CreateParamDesc(params->currentStringIndex,name,type,subType),__FILE__,__LINE__);
		params->currentStringIndex++;
	}
	if (type=="pointer")
	{
		params->pointerList.Push(inParam,__FILE__,__LINE__);
		params->descList.Push(CreateParamDesc(params->currentPointerIndex,name,type,subType),__FILE__,__LINE__);
		params->currentPointerIndex++;
	}
	if (type=="int")
	{
		params->intList.Push(*((int *)inParam),__FILE__,__LINE__);
		params->descList.Push(CreateParamDesc(params->currentIntIndex,name,type,subType),__FILE__,__LINE__);
		params->currentIntIndex++;

	}
}




