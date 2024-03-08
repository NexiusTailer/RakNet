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

#include "AutopatcherClientGFx3Impl.h"


static const char *AUTOPATCHER_LAST_UPDATE_FILE="autopatcherLastUpdate.txt";
static const char *AUTOPATCHER_RESTART_FILE="autopatcherRestart.txt";

class TestCB : public FileListTransferCBInterface
{
public:
	virtual bool OnFile(OnFileStruct *onFileStruct)
	{
		char buff[1024];

		if (onFileStruct->context.op==PC_HASH_WITH_PATCH)
			strcpy(buff,"Patched: ");
		else if (onFileStruct->context.op==PC_WRITE_FILE)
			strcpy(buff,"Written: ");
		else if (onFileStruct->context.op==PC_ERROR_FILE_WRITE_FAILURE)
			strcpy(buff,"Write Failure: ");
		else if (onFileStruct->context.op==PC_ERROR_PATCH_TARGET_MISSING)
			strcpy(buff,"Patch target missing: ");
		else if (onFileStruct->context.op==PC_ERROR_PATCH_APPLICATION_FAILURE)
			strcpy(buff,"Patch process failure: ");
		else if (onFileStruct->context.op==PC_ERROR_PATCH_RESULT_CHECKSUM_FAILURE)
			strcpy(buff,"Patch checksum failure: ");
		else if (onFileStruct->context.op==PC_NOTICE_WILL_COPY_ON_RESTART)
			strcpy(buff,"Copy pending restart: ");
		else if (onFileStruct->context.op==PC_NOTICE_FILE_DOWNLOADED)
			strcpy(buff,"Downloaded: ");
		else if (onFileStruct->context.op==PC_NOTICE_FILE_DOWNLOADED_PATCH)
			strcpy(buff,"Downloaded Patch: ");
		else
			RakAssert(0);


		sprintf(buff+strlen(buff), "%i. (100%%) %i/%i %s %ib / %ib\n", onFileStruct->setID, onFileStruct->fileIndex+1, onFileStruct->numberOfFilesInThisSet,
			onFileStruct->fileName, onFileStruct->byteLengthOfThisFile,
			 onFileStruct->byteLengthOfThisSet);

		FxResponseArgs<1> args;
		args.Add(GFxValue(buff));
		FxDelegate::Invoke(autopatcherClient->movie, "addToPatchNotesText", args);

		FxResponseArgs<5> args2;
		args2.Add(GFxValue(buff));
		args2.Add(GFxValue(1.0));
		args2.Add(GFxValue(1.0));
		args2.Add(GFxValue((double)onFileStruct->bytesDownloadedForThisSet));
		args2.Add(GFxValue((double)onFileStruct->byteLengthOfThisSet));
		FxDelegate::Invoke(autopatcherClient->movie, "updateProgressBars", args2);

		// Return false for the file data to be deallocated automatically
		return false;
	}

	virtual void OnFileProgress(OnFileStruct *onFileStruct,unsigned int partCount,unsigned int partTotal,unsigned int dataChunkLength, char *firstDataChunk)
	{
		char buff[1024];
		sprintf(buff, "%s %ib / %ib\n", onFileStruct->fileName,
			onFileStruct->bytesDownloadedForThisFile, onFileStruct->byteLengthOfThisFile);

		FxResponseArgs<5> args2;
		float thisFileProgress,totalProgress;
		thisFileProgress=(float)partCount/(float)partTotal;
		totalProgress=(float)(onFileStruct->fileIndex+1)/(float)onFileStruct->numberOfFilesInThisSet;
		args2.Add(GFxValue(buff));
		args2.Add(GFxValue((double)onFileStruct->bytesDownloadedForThisFile));
		args2.Add(GFxValue((double)onFileStruct->byteLengthOfThisFile));
		args2.Add(GFxValue((double)onFileStruct->bytesDownloadedForThisSet));
		args2.Add(GFxValue((double)onFileStruct->byteLengthOfThisSet));
		FxDelegate::Invoke(autopatcherClient->movie, "updateProgressBars", args2);
	}

	AutopatcherClientGFx3Impl *autopatcherClient;

} transferCallback;

AutopatcherClientGFx3Impl::AutopatcherClientGFx3Impl()
{
	autopatcherClient=0;
	fileListTransfer=0;
	packetizedTCP=0;
}
AutopatcherClientGFx3Impl::~AutopatcherClientGFx3Impl()
{
	Shutdown();
}
void AutopatcherClientGFx3Impl::Init(const char *_pathToThisExe, GPtr<FxDelegate> pDelegate, GPtr<GFxMovieView> pMovie)
{
	pDelegate->RegisterHandler(this);
	delegate=pDelegate;
	movie=pMovie;
	strcpy(pathToThisExe,_pathToThisExe);

	autopatcherClient=RakNet::OP_NEW<AutopatcherClient>(__FILE__,__LINE__);
	fileListTransfer=RakNet::OP_NEW<FileListTransfer>(__FILE__,__LINE__);
	packetizedTCP=RakNet::OP_NEW<PacketizedTCP>(__FILE__,__LINE__);
	autopatcherClient->SetFileListTransferPlugin(fileListTransfer);
	
	packetizedTCP->AttachPlugin(autopatcherClient);
	packetizedTCP->AttachPlugin(fileListTransfer);

}
void AutopatcherClientGFx3Impl::Update(void)
{
	Packet *p;

	SystemAddress notificationAddress;
	notificationAddress=packetizedTCP->HasCompletedConnectionAttempt();
	if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		UpdateConnectResult(true);
		serverAddress=notificationAddress;
	}
	notificationAddress=packetizedTCP->HasFailedConnectionAttempt();
	if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		UpdateConnectResult(false);
	}
	notificationAddress=packetizedTCP->HasNewIncomingConnection();
	notificationAddress=packetizedTCP->HasLostConnection();
	if (notificationAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		UpdateConnectResult(false);
	}

	p=packetizedTCP->Receive();
	while (p)
	{
		if (p->data[0]==ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR)
		{
			char buff[256];
			RakNet::BitStream temp(p->data, p->length, false);
			temp.IgnoreBits(8);
			stringCompressor->DecodeString(buff, 256, &temp);

			// Error.
			FxDelegate::Invoke(movie, "gotoCompletionMenu", FxResponseArgs<0>());

			FxResponseArgs<1> args2;
			args2.Add(GFxValue(buff));
			FxDelegate::Invoke(movie, "setCompletionMessage", args2);
		}
		else if (p->data[0]==ID_AUTOPATCHER_FINISHED)
		{
			FxDelegate::Invoke(movie, "gotoCompletionMenu", FxResponseArgs<0>());

			SaveLastUpdateDate();
		}
		else if (p->data[0]==ID_AUTOPATCHER_RESTART_APPLICATION)
		{
			FxDelegate::Invoke(movie, "gotoCompletionMenu", FxResponseArgs<0>());

			FxResponseArgs<1> args2;
			RakNet::RakString completionMsg("Launch \"AutopatcherClientRestarter.exe %s\"\nQuit this application immediately after to unlock files.\n", AUTOPATCHER_RESTART_FILE);
			args2.Add(GFxValue(completionMsg.C_String()));
			FxDelegate::Invoke(movie, "setCompletionMessage", args2);

			SaveLastUpdateDate();
		}

		packetizedTCP->DeallocatePacket(p);
		p=packetizedTCP->Receive();
	}
}
void AutopatcherClientGFx3Impl::Shutdown(void)
{
	if (delegate.GetPtr()!=0)
	{
		delegate->UnregisterHandler(this);
		delegate.Clear();
	}
	movie.Clear();
	if (packetizedTCP)
		packetizedTCP->Stop();
	RakNet::OP_DELETE(autopatcherClient,__FILE__,__LINE__);
	RakNet::OP_DELETE(fileListTransfer,__FILE__,__LINE__);
	RakNet::OP_DELETE(packetizedTCP,__FILE__,__LINE__);
	autopatcherClient=0;
	fileListTransfer=0;
	packetizedTCP=0;
}
const char* AutopatcherClientGFx3Impl::Connect(const char *ip, unsigned short port)
{
	if (packetizedTCP->Start(0,1)==true)
	{
		packetizedTCP->Connect(ip,port,false);
		return "Connecting";
	}
	else
		return "Start call failed.";
}
void AutopatcherClientGFx3Impl::PressedPatch(const FxDelegateArgs& pparams)
{
	AutopatcherClientGFx3Impl* prt = (AutopatcherClientGFx3Impl*)pparams.GetHandler();
	//appNameText.text, appDirectoryText.text, fullRescanBtn.selected
	const char *appName = pparams[0].GetString();
	const char *appDir = pparams[1].GetString();
	bool fullRescan = pparams[2].GetBool();
	strcpy(prt->appDirectory, appDir);

	char restartFile[512];
	strcpy(restartFile, appDir);
	strcat(restartFile, "/");
	strcat(restartFile, AUTOPATCHER_RESTART_FILE);
	char lastUpdateDate[512];
	if (fullRescan==false)
		prt->LoadLastUpdateDate(lastUpdateDate,appDir);
	else
		lastUpdateDate[0]=0;
	
	transferCallback.autopatcherClient=prt;
	if (prt->autopatcherClient->PatchApplication(appName, appDir, lastUpdateDate, prt->serverAddress, &transferCallback, restartFile, prt->pathToThisExe))
	{
		FxDelegate::Invoke(prt->movie, "gotoPatchMenu", FxResponseArgs<0>());
	}
	else
	{
		prt->packetizedTCP->Stop();
		//prt->UpdateConnectResult("Failed to start patching");
		FxDelegate::Invoke(prt->movie, "gotoPatchStartMenu", FxResponseArgs<0>());
	}
}
void AutopatcherClientGFx3Impl::OpenSite(const FxDelegateArgs& pparams)
{
	AutopatcherClientGFx3Impl* prt = (AutopatcherClientGFx3Impl*)pparams.GetHandler();
	const char *siteType = pparams[0].GetString();
	if (stricmp(siteType, "help")==0)
	{
		ShellExecute(NULL, "open", "http://www.jenkinssoftware.com/raknet/manual/autopatcher.html",	NULL, NULL, SW_SHOWNORMAL);
	}
	else if (stricmp(siteType, "raknet")==0)
	{
		ShellExecute(NULL, "open", "http://www.jenkinssoftware.com/",	NULL, NULL, SW_SHOWNORMAL);
	}
	else if (stricmp(siteType, "scaleform")==0)
	{
		ShellExecute(NULL, "open", "https://www.scaleform.com/",	NULL, NULL, SW_SHOWNORMAL);
	}
}
void AutopatcherClientGFx3Impl::PressedConnect(const FxDelegateArgs& pparams)
{
	AutopatcherClientGFx3Impl* prt = (AutopatcherClientGFx3Impl*)pparams.GetHandler();
	const char *result = prt->Connect(pparams[0].GetString(), atoi(pparams[1].GetString()));
}
void AutopatcherClientGFx3Impl::PressedOKBtn(const FxDelegateArgs& pparams)
{
	AutopatcherClientGFx3Impl* prt = (AutopatcherClientGFx3Impl*)pparams.GetHandler();
	prt->autopatcherClient->Clear();
	prt->packetizedTCP->Stop();

	prt->GotoMainMenu();
}
void AutopatcherClientGFx3Impl::UpdateConnectResult( bool isConnected )
{
	FxResponseArgs<1> args;
	args.Add(GFxValue(isConnected));
	FxDelegate::Invoke(movie, "ConnectResult", args);
}

void AutopatcherClientGFx3Impl::Accept(CallbackProcessor* cbreg)
{
	cbreg->Process( "PressedOKBtn", &AutopatcherClientGFx3Impl::PressedOKBtn );
	cbreg->Process( "PressedConnect", &AutopatcherClientGFx3Impl::PressedConnect );
	cbreg->Process( "PressedPatch", &AutopatcherClientGFx3Impl::PressedPatch );
	cbreg->Process( "openSite", &AutopatcherClientGFx3Impl::OpenSite );
}

void AutopatcherClientGFx3Impl::SaveLastUpdateDate(void)
{
	char inPath[512];
	char *serverDate=autopatcherClient->GetServerDate();
	if (serverDate==0 || serverDate[0]==0)
		return;
	strcpy(inPath, appDirectory);
	strcat(inPath, "/");
	strcat(inPath, AUTOPATCHER_LAST_UPDATE_FILE);
	FILE *fp = fopen(inPath,"wb");
	if (fp!=0)
	{
		fwrite(serverDate,1,512,fp);
		fclose(fp);
	}
}

void AutopatcherClientGFx3Impl::LoadLastUpdateDate(char *out, const char *appDir)
{
	strcpy(appDirectory,appDir);
	strcpy(out, appDirectory);
	strcat(out, "/");
	strcat(out, AUTOPATCHER_LAST_UPDATE_FILE);
	FILE *fp = fopen(out,"rb");
	if (fp!=0)
	{
		fread(out,1,512,fp);
		fclose(fp);
	}
	else
		out[0]=0;
}
void AutopatcherClientGFx3Impl::GotoMainMenu(void)
{
	FxDelegate::Invoke(movie, "gotoMainMenu", FxResponseArgs<0>());
	autopatcherClient->Clear();
	packetizedTCP->Stop();
}
