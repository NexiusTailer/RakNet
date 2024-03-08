/// \file
/// \brief The server plugin for the autopatcher.  Must be running for the client to get patches.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "AutopatcherServer.h"
#include "DirectoryDeltaTransfer.h"
#include "FileList.h"
#include "StringCompressor.h"
#include "RakPeerInterface.h"
#include "FileListTransfer.h"
#include "FileListTransferCBInterface.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include "AutopatcherRepositoryInterface.h"
#include "RakAssert.h"

#ifdef _MSC_VER
#pragma warning( push )
#endif

using namespace RakNet;


void AutopatcherServerLoadNotifier_Printf::OnQueueUpdate(SystemAddress remoteSystem, AutopatcherServerLoadNotifier::RequestType requestType, AutopatcherServerLoadNotifier::QueueOperation queueOperation, AutopatcherServerLoadNotifier::AutopatcherState *autopatcherState)
{
	char *operationString;
	char *requestTypeString;
	char systemAddressString[32];
	remoteSystem.ToString(true, systemAddressString);
	if (requestType==ASUMC_GET_CHANGELIST)
		requestTypeString="GetChangelist";
	else
		requestTypeString="GetPatch";
	if (queueOperation==QO_WAS_ADDED)
		operationString="added";
	else if (queueOperation==QO_POPPED_ONTO_TO_PROCESSING_THREAD)
		operationString="processing";
	else if (queueOperation==QO_WAS_ABORTED)
		operationString="aborted";

	printf("%s %s %s. %i queued. %i working.\n", systemAddressString, requestTypeString, operationString, autopatcherState->requestsQueued, autopatcherState->requestsWorking);
}
void AutopatcherServerLoadNotifier_Printf::OnGetChangelistCompleted(
									  SystemAddress remoteSystem,
									  AutopatcherServerLoadNotifier::GetChangelistResult getChangelistResult,
									  AutopatcherServerLoadNotifier::AutopatcherState *autopatcherState)
{
	char systemAddressString[32];
	remoteSystem.ToString(true, systemAddressString);

	char *changelistString;
	if (getChangelistResult==GCR_DELETE_FILES)
		changelistString="Delete files";
	else if (getChangelistResult==GCR_ADD_FILES)
		changelistString="Add files";
	else if (getChangelistResult==GCR_ADD_AND_DELETE_FILES)
		changelistString="Add and delete files";
	else if (getChangelistResult==GCR_NOTHING_TO_DO)
		changelistString="No files in changelist";
	else if (getChangelistResult==GCR_REPOSITORY_ERROR)
		changelistString="Repository error";

	printf("%s GetChangelist complete. %s. %i queued. %i working.\n", systemAddressString, changelistString, autopatcherState->requestsQueued, autopatcherState->requestsWorking);
}
void AutopatcherServerLoadNotifier_Printf::OnGetPatchCompleted(SystemAddress remoteSystem, AutopatcherServerLoadNotifier::PatchResult patchResult, AutopatcherServerLoadNotifier::AutopatcherState *autopatcherState)
{
	char systemAddressString[32];
	remoteSystem.ToString(true, systemAddressString);

	char *patchResultString;
	if (patchResult==PR_NO_FILES_NEEDED_PATCHING)
		patchResultString="No files needed patching"; 
	else if (patchResult==PR_REPOSITORY_ERROR)
		patchResultString="Repository error"; 
	else if (patchResult==PR_PATCHES_WERE_SENT)
		patchResultString="Files pushed for patching";
	else if (patchResult==PR_ABORTED_FROM_INPUT_THREAD)
		patchResultString="Aborted from input thread";
	else if (patchResult==PR_ABORTED_FROM_DOWNLOAD_THREAD)
		patchResultString="Aborted from download thread";

	printf("%s GetPatch complete. %s. %i queued. %i working.\n", systemAddressString, patchResultString, autopatcherState->requestsQueued, autopatcherState->requestsWorking);
}

AutopatcherServer::AutopatcherServer()
{
	fileListTransfer=0;
	priority=HIGH_PRIORITY;
	orderingChannel=0;
//	repository=0;
	patchingUserCount=0;
	maxConcurrentUsers=0;
	loadNotifier=0;
}
AutopatcherServer::~AutopatcherServer()
{
	Clear();
}
void AutopatcherServer::SetUploadSendParameters(PacketPriority _priority, char _orderingChannel)
{
	priority=_priority;
	orderingChannel=_orderingChannel;
}
void AutopatcherServer::SetFileListTransferPlugin(FileListTransfer *flt)
{
	if (fileListTransfer)
		fileListTransfer->RemoveCallback(this);
	fileListTransfer=flt;
	if (fileListTransfer)
		fileListTransfer->AddCallback(this);
}
void AutopatcherServer::StartThreads(int numThreads, int numSQLConnections, AutopatcherRepositoryInterface **sqlConnectionPtrArray)
{
	connectionPoolMutex.Lock();
	for (int i=0; i < numSQLConnections; i++)
	{
		// Test the pointers passed, in case the user incorrectly casted an array of a different type
		sqlConnectionPtrArray[i]->GetLastError();
		connectionPool.Push(sqlConnectionPtrArray[i],_FILE_AND_LINE_);
	}
	connectionPoolMutex.Unlock();
	threadPool.SetThreadDataInterface(this,0);
	threadPool.StartThreads(numThreads, 0);
}
void AutopatcherServer::OnAttach(void)
{
}
void AutopatcherServer::OnDetach(void)
{
	Clear();
}
#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
void AutopatcherServer::Update(void)
{
	while (PatchingUserLimitReached()==false && userRequestWaitingQueue.Size()>0)
	{
		Packet *packet = PopOffWaitingQueue();
		switch (packet->data[0]) 
		{
		case ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE:
			OnGetChangelistSinceDate(packet);
			break;
		case ID_AUTOPATCHER_GET_PATCH:
			OnGetPatch(packet);
			break;
		}
		DeallocPacketUnified(packet);
	}
}
PluginReceiveResult AutopatcherServer::OnReceive(Packet *packet)
{
	switch (packet->data[0]) 
	{
	case ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE:
		if (PatchingUserLimitReached())
		{
			AddToWaitingQueue(packet);
			return RR_STOP_PROCESSING;
		}
		else
		{
			OnGetChangelistSinceDate(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
	case ID_AUTOPATCHER_GET_PATCH:
		if (PatchingUserLimitReached())
		{
			AddToWaitingQueue(packet);
			return RR_STOP_PROCESSING;
		}
		else
		{
			OnGetPatch(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
	}

	return RR_CONTINUE_PROCESSING;
}
#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
void AutopatcherServer::OnShutdown(void)
{
	Clear();
}
void AutopatcherServer::Clear(void)
{
	// Clear the waiting input and output from the thread pool.
	unsigned i;
	threadPool.StopThreads();
	for (i=0; i < threadPool.InputSize(); i++)
	{
		DecrementPatchingUserCount();
		CallPatchCompleteCallback(threadPool.GetInputAtIndex(i).systemAddress, AutopatcherServerLoadNotifier::PR_ABORTED_FROM_INPUT_THREAD);
		RakNet::OP_DELETE(threadPool.GetInputAtIndex(i).clientList, _FILE_AND_LINE_);
	}
	threadPool.ClearInput();
	for (i=0; i < threadPool.OutputSize(); i++)
	{
		RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->patchList, _FILE_AND_LINE_);
		RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->deletedFiles, _FILE_AND_LINE_);
		RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->addedFiles, _FILE_AND_LINE_);
	}
	threadPool.ClearOutput();

	while (userRequestWaitingQueue.Size())
		DeallocPacketUnified(AbortOffWaitingQueue());
}
#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
void AutopatcherServer::OnStartup(RakPeerInterface *peer)
{
}
#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
void AutopatcherServer::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	RemoveFromThreadPool(systemAddress);

	while (userRequestWaitingQueue.Size())
		DeallocPacketUnified(AbortOffWaitingQueue());
}
void AutopatcherServer::RemoveFromThreadPool(SystemAddress systemAddress)
{
	unsigned i;
	i=0;
	threadPool.LockInput();
	while (i < threadPool.InputSize())
	{
		if (threadPool.GetInputAtIndex(i).systemAddress==systemAddress)
		{
			DecrementPatchingUserCount();
			CallPatchCompleteCallback(threadPool.GetInputAtIndex(i).systemAddress, AutopatcherServerLoadNotifier::PR_ABORTED_FROM_INPUT_THREAD);
			RakNet::OP_DELETE(threadPool.GetInputAtIndex(i).clientList, _FILE_AND_LINE_);
			threadPool.RemoveInputAtIndex(i);
		}
		else
			i++;
	}
	threadPool.UnlockInput();

	i=0;
	threadPool.LockOutput();
	while (i < threadPool.OutputSize())
	{
		if (threadPool.GetOutputAtIndex(i)->systemAddress==systemAddress)
		{
			RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->patchList, _FILE_AND_LINE_);
			RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->deletedFiles, _FILE_AND_LINE_);
			RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->addedFiles, _FILE_AND_LINE_);
			threadPool.RemoveOutputAtIndex(i);
		}
		else
			i++;
	}
	threadPool.UnlockOutput();
}
namespace RakNet
{
AutopatcherServer::ResultTypeAndBitstream* GetChangelistSinceDateCB(AutopatcherServer::ThreadData threadData, bool *returnOutput, void* perThreadData)
{
	AutopatcherRepositoryInterface *repository = (AutopatcherRepositoryInterface*)perThreadData;
	
	FileList addedFiles, deletedFiles;
	char currentDate[64];
	currentDate[0]=0;
	AutopatcherServer *server = threadData.server;

	//AutopatcherServer::ResultTypeAndBitstream *rtab = RakNet::OP_NEW<AutopatcherServer::ResultTypeAndBitstream>( _FILE_AND_LINE_ );
	AutopatcherServer::ResultTypeAndBitstream rtab;
	rtab.systemAddress=threadData.systemAddress;
// 	rtab.deletedFiles=RakNet::OP_NEW<FileList>( _FILE_AND_LINE_ );
// 	rtab.addedFiles=RakNet::OP_NEW<FileList>( _FILE_AND_LINE_ );
	rtab.deletedFiles=&deletedFiles;
	rtab.addedFiles=&addedFiles;

	// Query the database for a changelist since this date
	RakAssert(server);
	//if (server->repository->GetChangelistSinceDate(threadData.applicationName.C_String(), rtab.addedFiles, rtab.deletedFiles, threadData.lastUpdateDate.C_String(), currentDate))
	if (repository->GetChangelistSinceDate(threadData.applicationName.C_String(), rtab.addedFiles, rtab.deletedFiles, threadData.lastUpdateDate.C_String(), currentDate))
	{
		rtab.fatalError=false;
	}
	else
	{
		rtab.fatalError=true;
	}

	rtab.operation=AutopatcherServer::ResultTypeAndBitstream::GET_CHANGELIST_SINCE_DATE;
	rtab.currentDate=currentDate;
	// *returnOutput=true;
	// return rtab;

	if (rtab.fatalError==false)
	{
		if (rtab.deletedFiles->fileList.Size())
		{
			rtab.bitStream1.Write((unsigned char) ID_AUTOPATCHER_DELETION_LIST);
			rtab.deletedFiles->Serialize(&rtab.bitStream1);
		}

		if (rtab.addedFiles->fileList.Size())
		{
			rtab.bitStream2.Write((unsigned char) ID_AUTOPATCHER_CREATION_LIST);
			rtab.addedFiles->Serialize(&rtab.bitStream2);
			StringCompressor::Instance()->EncodeString(rtab.currentDate.C_String(),64,&rtab.bitStream2);
			rtab.addedFiles->Clear();
		}
		else
		{
			rtab.bitStream2.Write((unsigned char) ID_AUTOPATCHER_FINISHED);
			StringCompressor::Instance()->EncodeString(rtab.currentDate.C_String(),64,&rtab.bitStream2);
		}
	}
	else
	{
		rtab.bitStream2.Write((unsigned char) ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR);
		StringCompressor::Instance()->EncodeString(repository->GetLastError(), 256, &rtab.bitStream2);	
	}
// 	RakNet::OP_DELETE(rtab.deletedFiles, _FILE_AND_LINE_);
// 	RakNet::OP_DELETE(rtab.addedFiles, _FILE_AND_LINE_);

	*returnOutput=false;

	if (rtab.bitStream1.GetNumberOfBitsUsed()>0)
		server->SendUnified(&(rtab.bitStream1), server->priority, RELIABLE_ORDERED, server->orderingChannel, rtab.systemAddress, false);
	if (rtab.bitStream2.GetNumberOfBitsUsed()>0)
		server->SendUnified(&(rtab.bitStream2), server->priority, RELIABLE_ORDERED, server->orderingChannel, rtab.systemAddress, false);

	server->DecrementPatchingUserCount();

	if (server->loadNotifier)
	{
		AutopatcherServerLoadNotifier::AutopatcherState autopatcherState;
		autopatcherState.requestsQueued=server->userRequestWaitingQueue.Size();
		autopatcherState.requestsWorking=server->patchingUserCount;

		AutopatcherServerLoadNotifier::GetChangelistResult getChangelistResult;
		if (rtab.fatalError==true)
			getChangelistResult=AutopatcherServerLoadNotifier::GCR_REPOSITORY_ERROR;
		else if (rtab.deletedFiles->fileList.Size()==0 && rtab.addedFiles->fileList.Size()==0)
			getChangelistResult=AutopatcherServerLoadNotifier::GCR_NOTHING_TO_DO;
		else if (rtab.deletedFiles->fileList.Size()==0)
			getChangelistResult=AutopatcherServerLoadNotifier::GCR_ADD_FILES;
		else if (rtab.addedFiles->fileList.Size()==0)
			getChangelistResult=AutopatcherServerLoadNotifier::GCR_DELETE_FILES;
		else
			getChangelistResult=AutopatcherServerLoadNotifier::GCR_ADD_AND_DELETE_FILES;

		server->loadNotifier->OnGetChangelistCompleted(rtab.systemAddress, getChangelistResult, &autopatcherState);
	}

	return 0;
}
}
void AutopatcherServer::OnGetChangelistSinceDate(Packet *packet)
{
	RakNet::BitStream inBitStream(packet->data, packet->length, false);
	ThreadData threadData;
	inBitStream.IgnoreBits(8);
	inBitStream.ReadCompressed(threadData.applicationName);
	inBitStream.ReadCompressed(threadData.lastUpdateDate);

	IncrementPatchingUserCount();
	CallPacketCallback(packet, AutopatcherServerLoadNotifier::QO_POPPED_ONTO_TO_PROCESSING_THREAD);

	threadData.server=this;
	threadData.systemAddress=packet->systemAddress;
	threadPool.AddInput(GetChangelistSinceDateCB, threadData);
}
namespace RakNet {
AutopatcherServer::ResultTypeAndBitstream* GetPatchCB(AutopatcherServer::ThreadData threadData, bool *returnOutput, void* perThreadData)
{
	AutopatcherServer *server = threadData.server;
	AutopatcherRepositoryInterface *repository = (AutopatcherRepositoryInterface*)perThreadData;

	// AutopatcherServer::ResultTypeAndBitstream *rtab = RakNet::OP_NEW<AutopatcherServer::ResultTypeAndBitstream>( _FILE_AND_LINE_ );
	AutopatcherServer::ResultTypeAndBitstream rtab;
	rtab.systemAddress=threadData.systemAddress;
	FileList fileList;
	// rtab.patchList=RakNet::OP_NEW<FileList>( _FILE_AND_LINE_ );
	rtab.patchList=&fileList;
	RakAssert(server);
//	RakAssert(server->repository);
	char currentDate[64];
	currentDate[0]=0;
//	if (server->repository->GetPatches(threadData.applicationName.C_String(), threadData.clientList, rtab.patchList, currentDate))
	if (repository->GetPatches(threadData.applicationName.C_String(), threadData.clientList, rtab.patchList, currentDate))
		rtab.fatalError=false;
	else
		rtab.fatalError=true;
	rtab.operation=AutopatcherServer::ResultTypeAndBitstream::GET_PATCH;
	rtab.setId=threadData.setId;
	rtab.currentDate=currentDate;

	RakNet::OP_DELETE(threadData.clientList, _FILE_AND_LINE_);

	if (rtab.fatalError==false)
	{
		if (rtab.patchList->fileList.Size())
		{
			//server->fileListTransfer->Send(rtab.patchList, 0, rtab.systemAddress, rtab.setId, server->priority, server->orderingChannel, false, server->repository);
			server->fileListTransfer->Send(rtab.patchList, 0, rtab.systemAddress, rtab.setId, server->priority, server->orderingChannel, repository, repository->GetIncrementalReadChunkSize());
		}
		else
		{
			server->DecrementPatchingUserCount(); // No files needed to send
			server->CallPatchCompleteCallback(rtab.systemAddress, AutopatcherServerLoadNotifier::PR_NO_FILES_NEEDED_PATCHING);
		}

		rtab.bitStream1.Write((unsigned char) ID_AUTOPATCHER_FINISHED_INTERNAL);
		StringCompressor::Instance()->EncodeString(rtab.currentDate.C_String(),64,&rtab.bitStream1);
	}
	else
	{
		rtab.bitStream1.Write((unsigned char) ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR);
	//	StringCompressor::Instance()->EncodeString(server->repository->GetLastError(), 256, &rtab.bitStream1);
		StringCompressor::Instance()->EncodeString(repository->GetLastError(), 256, &rtab.bitStream1);

		server->DecrementPatchingUserCount(); // Repository error
		server->CallPatchCompleteCallback(rtab.systemAddress, AutopatcherServerLoadNotifier::PR_REPOSITORY_ERROR);
	}

	*returnOutput=false;

	if (rtab.bitStream1.GetNumberOfBitsUsed()>0)
		server->SendUnified(&(rtab.bitStream1), server->priority, RELIABLE_ORDERED, server->orderingChannel, rtab.systemAddress, false);
	if (rtab.bitStream2.GetNumberOfBitsUsed()>0)
		server->SendUnified(&(rtab.bitStream2), server->priority, RELIABLE_ORDERED, server->orderingChannel, rtab.systemAddress, false);

	// 12/1/2010 This doesn't scale well. Changing to allocating a connection object per request
	/*
	// Wait for repository to finish
	// This is so that the same sql connection is not used between two different plugins, which causes thrashing and bad performance
	// Plus if fileListTransfer uses multiple threads, this will keep this thread and the fileListTransfer thread from using the same connection at the same time
	// PostgreSQL possibly MySQL are not threadsafe for multiple threads on the same connection
	int pendingFiles = server->fileListTransfer->GetPendingFilesToAddress(rtab.systemAddress);
	while (pendingFiles>0)
	{
		RakSleep(pendingFiles*10);
		pendingFiles = server->fileListTransfer->GetPendingFilesToAddress(rtab.systemAddress);
	}
	*/
	
	// *returnOutput=true;
	// return rtab;
	return 0;
}
}
void AutopatcherServer::OnGetPatch(Packet *packet)
{
	RakNet::BitStream inBitStream(packet->data, packet->length, false);
	
	ThreadData threadData;
	inBitStream.IgnoreBits(8);
	inBitStream.Read(threadData.setId);
	inBitStream.ReadCompressed(threadData.applicationName);
	threadData.systemAddress=packet->systemAddress;
	threadData.server=this;
	threadData.clientList=RakNet::OP_NEW<FileList>( _FILE_AND_LINE_ );

	if (threadData.clientList->Deserialize(&inBitStream)==false)
	{
		RakNet::OP_DELETE(threadData.clientList, _FILE_AND_LINE_);
		return;
	}
	if (threadData.clientList->fileList.Size()==0)
	{
		RakAssert(0);
		RakNet::OP_DELETE(threadData.clientList, _FILE_AND_LINE_);
		return;
	}

	IncrementPatchingUserCount();
	CallPacketCallback(packet, AutopatcherServerLoadNotifier::QO_POPPED_ONTO_TO_PROCESSING_THREAD);

	threadPool.AddInput(GetPatchCB, threadData);
	return;
}
void* AutopatcherServer::PerThreadFactory(void *context)
{
	(void)context;

	AutopatcherRepositoryInterface* p;
	connectionPoolMutex.Lock();
	p=connectionPool.Pop();
	connectionPoolMutex.Unlock();
	return p;
}
void AutopatcherServer::PerThreadDestructor(void* factoryResult, void *context)
{
	(void)context;
	(void)factoryResult;
}
void AutopatcherServer::OnFilePushesComplete( SystemAddress systemAddress )
{
	DecrementPatchingUserCount();
	CallPatchCompleteCallback(systemAddress, AutopatcherServerLoadNotifier::PR_PATCHES_WERE_SENT);
}
void AutopatcherServer::OnSendAborted( SystemAddress systemAddress )
{
	DecrementPatchingUserCount();
	CallPatchCompleteCallback(systemAddress, AutopatcherServerLoadNotifier::PR_ABORTED_FROM_DOWNLOAD_THREAD);
}
void AutopatcherServer::IncrementPatchingUserCount(void)
{
	++patchingUserCount;
}
void AutopatcherServer::DecrementPatchingUserCount(void)
{
	--patchingUserCount;
}
bool AutopatcherServer::PatchingUserLimitReached(void) const
{
	if (maxConcurrentUsers==0)
		return false;

	return patchingUserCount>=maxConcurrentUsers;
}
void AutopatcherServer::SetMaxConurrentUsers(unsigned int _maxConcurrentUsers)
{
	maxConcurrentUsers=_maxConcurrentUsers;
}
void AutopatcherServer::CallPacketCallback(Packet *packet, AutopatcherServerLoadNotifier::QueueOperation queueOperation)
{
	if (loadNotifier)
	{
		AutopatcherServerLoadNotifier::AutopatcherState autopatcherState;
		autopatcherState.requestsQueued=userRequestWaitingQueue.Size();
		autopatcherState.requestsWorking=patchingUserCount;

		AutopatcherServerLoadNotifier::RequestType requestType;
		if (packet->data[0]==ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE)
			requestType=AutopatcherServerLoadNotifier::ASUMC_GET_CHANGELIST;
		else
			requestType=AutopatcherServerLoadNotifier::ASUMC_GET_PATCH;

		loadNotifier->OnQueueUpdate(packet->systemAddress, requestType, queueOperation, &autopatcherState);
	}
}
void AutopatcherServer::CallPatchCompleteCallback(SystemAddress systemAddress, AutopatcherServerLoadNotifier::PatchResult patchResult)
{
	if (loadNotifier)
	{
		AutopatcherServerLoadNotifier::AutopatcherState autopatcherState;
		autopatcherState.requestsQueued=userRequestWaitingQueue.Size();
		autopatcherState.requestsWorking=patchingUserCount;

		loadNotifier->OnGetPatchCompleted(systemAddress, patchResult, &autopatcherState);
	}
}
void AutopatcherServer::AddToWaitingQueue(Packet *packet)
{
	userRequestWaitingQueue.Push(packet, _FILE_AND_LINE_);
	CallPacketCallback(packet, AutopatcherServerLoadNotifier::QO_WAS_ADDED);
}
Packet *AutopatcherServer::AbortOffWaitingQueue(void)
{
	Packet *packet = userRequestWaitingQueue.Pop();
	CallPacketCallback(packet,AutopatcherServerLoadNotifier::QO_WAS_ABORTED);
	return packet;
}
Packet *AutopatcherServer::PopOffWaitingQueue(void)
{
	return userRequestWaitingQueue.Pop();;
}
void AutopatcherServer::SetLoadManagementCallback(AutopatcherServerLoadNotifier *asumc)
{
	loadNotifier=asumc;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif
