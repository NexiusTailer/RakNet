/// \file
/// \brief The server plugin for the autopatcher.  Must be running for the client to get patches.
///
/// This file is part of RakNet Copyright 2003 Kevin Jenkins.
///
/// Usage of RakNet is subject to the appropriate license agreement.
/// Creative Commons Licensees are subject to the
/// license found at
/// http://creativecommons.org/licenses/by-nc/2.5/
/// Single application licensees are subject to the license found at
/// http://www.jenkinssoftware.com/SingleApplicationLicense.html
/// Custom license users are subject to the terms therein.
/// GPL license users are subject to the GNU General Public
/// License as published by the Free
/// Software Foundation; either version 2 of the License, or (at your
/// option) any later version.

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

AutopatcherServer::AutopatcherServer()
{
	rakPeer=0;
	fileListTransfer=0;
	priority=HIGH_PRIORITY;
	orderingChannel=0;
	repository=0;
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
	fileListTransfer=flt;
}
void AutopatcherServer::SetAutopatcherRepositoryInterface(AutopatcherRepositoryInterface *ari)
{
	RakAssert(ari);
	repository=ari;
}
void AutopatcherServer::OnAttach(RakPeerInterface *peer)
{
	rakPeer=peer;
	threadPool.StartThreads(1, 0);
}
void AutopatcherServer::OnDetach(RakPeerInterface *peer)
{
	Clear();
}
#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
void AutopatcherServer::Update(RakPeerInterface *peer)
{
	ResultTypeAndBitstream* rtab;
	while (threadPool.HasOutputFast() && threadPool.HasOutput())
	{
		rtab = threadPool.GetOutput();
		if (rtab->operation==ResultTypeAndBitstream::GET_PATCH)
		{
			if (rtab->fatalError==false)
			{
				if (rtab->patchList->fileList.Size())
					fileListTransfer->Send(rtab->patchList, rakPeer, rtab->systemAddress, rtab->setId, priority, orderingChannel, false, repository);

				rtab->bitStream1.Write((unsigned char) ID_AUTOPATCHER_FINISHED_INTERNAL);
				stringCompressor->EncodeString(rtab->currentDate.C_String(),64,&rtab->bitStream1);
			}
			else
			{
				rtab->bitStream1.Write((unsigned char) ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR);
				stringCompressor->EncodeString(repository->GetLastError(), 256, &rtab->bitStream1);
			}
			RakNet::OP_DELETE(rtab->patchList);
		}
		else if (rtab->operation==ResultTypeAndBitstream::GET_CHANGELIST_SINCE_DATE)
		{
			if (rtab->fatalError==false)
			{
				if (rtab->deletedFiles->fileList.Size())
				{
					rtab->bitStream1.Write((unsigned char) ID_AUTOPATCHER_DELETION_LIST);
					rtab->deletedFiles->Serialize(&rtab->bitStream1);
				}

				if (rtab->addedFiles->fileList.Size())
				{
					rtab->bitStream2.Write((unsigned char) ID_AUTOPATCHER_CREATION_LIST);
					rtab->addedFiles->Serialize(&rtab->bitStream2);
					stringCompressor->EncodeString(rtab->currentDate.C_String(),64,&rtab->bitStream2);
					rtab->addedFiles->Clear();
				}
				else
				{
					rtab->bitStream2.Write((unsigned char) ID_AUTOPATCHER_FINISHED);
					stringCompressor->EncodeString(rtab->currentDate.C_String(),64,&rtab->bitStream2);
				}
			}
			else
			{
				rtab->bitStream2.Write((unsigned char) ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR);
				stringCompressor->EncodeString(repository->GetLastError(), 256, &rtab->bitStream2);	
			}
			RakNet::OP_DELETE(rtab->deletedFiles);
			RakNet::OP_DELETE(rtab->addedFiles);
		}
		if (rtab->bitStream1.GetNumberOfBitsUsed()>0)
			rakPeer->Send(&(rtab->bitStream1), priority, RELIABLE_ORDERED, orderingChannel, rtab->systemAddress, false);
		if (rtab->bitStream2.GetNumberOfBitsUsed()>0)
			rakPeer->Send(&(rtab->bitStream2), priority, RELIABLE_ORDERED, orderingChannel, rtab->systemAddress, false);
		RakNet::OP_DELETE(rtab);
	}	
}
PluginReceiveResult AutopatcherServer::OnReceive(RakPeerInterface *peer, Packet *packet)
{
	switch (packet->data[0]) 
	{
	case ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE:
		OnGetChangelistSinceDate(peer, packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_AUTOPATCHER_GET_PATCH:
		OnGetPatch(peer, packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_DISCONNECTION_NOTIFICATION:
	case ID_CONNECTION_LOST:
		OnCloseConnection(peer, packet->systemAddress);
	break;
	}

	return RR_CONTINUE_PROCESSING;
}
#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
void AutopatcherServer::OnShutdown(RakPeerInterface *peer)
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
		RakNet::OP_DELETE(threadPool.GetInputAtIndex(i).clientList);
	}
	threadPool.ClearInput();
	for (i=0; i < threadPool.OutputSize(); i++)
	{
		RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->patchList);
		RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->deletedFiles);
		RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->addedFiles);
	}
	threadPool.ClearOutput();
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
void AutopatcherServer::OnCloseConnection(RakPeerInterface *peer, SystemAddress systemAddress)
{
	RemoveFromThreadPool(systemAddress);
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
			RakNet::OP_DELETE(threadPool.GetInputAtIndex(i).clientList);
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
			RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->patchList);
			RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->deletedFiles);
			RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i)->addedFiles);
			threadPool.RemoveOutputAtIndex(i);
		}
		else
			i++;
	}
	threadPool.UnlockOutput();
}
AutopatcherServer::ResultTypeAndBitstream* GetChangelistSinceDateCB(AutopatcherServer::ThreadData threadData, bool *returnOutput, void* perThreadData)
{
	
	FileList addedFiles, deletedFiles;
	char currentDate[64];
	currentDate[0]=0;
	AutopatcherServer *server = threadData.server;

	AutopatcherServer::ResultTypeAndBitstream *rtab = RakNet::OP_NEW<AutopatcherServer::ResultTypeAndBitstream>();
	rtab->systemAddress=threadData.systemAddress;
	rtab->deletedFiles=RakNet::OP_NEW<FileList>();
	rtab->addedFiles=RakNet::OP_NEW<FileList>();

	// Query the database for a changelist since this date
	RakAssert(server);
	if (server->repository->GetChangelistSinceDate(threadData.applicationName.C_String(), rtab->addedFiles, rtab->deletedFiles, threadData.lastUpdateDate.C_String(), currentDate))
	{
		rtab->fatalError=false;
	}
	else
	{
		rtab->fatalError=true;
	}

	rtab->operation=AutopatcherServer::ResultTypeAndBitstream::GET_CHANGELIST_SINCE_DATE;
	rtab->currentDate=currentDate;
	*returnOutput=true;
	return rtab;
}
void AutopatcherServer::OnGetChangelistSinceDate(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream inBitStream(packet->data, packet->length, false);
	ThreadData threadData;
	inBitStream.IgnoreBits(8);
	inBitStream.ReadCompressed(threadData.applicationName);
	inBitStream.ReadCompressed(threadData.lastUpdateDate);

	threadData.server=this;
	threadData.systemAddress=packet->systemAddress;
	threadPool.AddInput(GetChangelistSinceDateCB, threadData);
}
AutopatcherServer::ResultTypeAndBitstream* GetPatchCB(AutopatcherServer::ThreadData threadData, bool *returnOutput, void* perThreadData)
{
	AutopatcherServer *server = threadData.server;

	AutopatcherServer::ResultTypeAndBitstream *rtab = RakNet::OP_NEW<AutopatcherServer::ResultTypeAndBitstream>();
	rtab->systemAddress=threadData.systemAddress;
	rtab->patchList=RakNet::OP_NEW<FileList>();
	RakAssert(server);
	RakAssert(server->repository);
	char currentDate[64];
	currentDate[0]=0;
	if (server->repository->GetPatches(threadData.applicationName.C_String(), threadData.clientList, rtab->patchList, currentDate))
		rtab->fatalError=false;
	else
		rtab->fatalError=true;
	rtab->operation=AutopatcherServer::ResultTypeAndBitstream::GET_PATCH;
	rtab->setId=threadData.setId;
	rtab->currentDate=currentDate;

	RakNet::OP_DELETE(threadData.clientList);

	*returnOutput=true;
	return rtab;
}
void AutopatcherServer::OnGetPatch(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream inBitStream(packet->data, packet->length, false);
	
	ThreadData threadData;
	inBitStream.IgnoreBits(8);
	inBitStream.Read(threadData.setId);
	inBitStream.ReadCompressed(threadData.applicationName);
	threadData.systemAddress=packet->systemAddress;
	threadData.server=this;
	threadData.clientList=RakNet::OP_NEW<FileList>();

	if (threadData.clientList->Deserialize(&inBitStream)==false)
	{
		RakNet::OP_DELETE(threadData.clientList);
		return;
	}
	if (threadData.clientList->fileList.Size()==0)
	{
		RakAssert(0);
		RakNet::OP_DELETE(threadData.clientList);
		return;
	}

	threadPool.AddInput(GetPatchCB, threadData);
	return;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif
