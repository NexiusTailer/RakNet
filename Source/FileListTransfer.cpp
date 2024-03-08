#include "FileListTransfer.h"
#include "DS_HuffmanEncodingTree.h"
#include "FileListTransferCBInterface.h"
#include "StringCompressor.h"
#include "FileList.h"
#include "DS_Queue.h"
#include "MessageIdentifiers.h"
#include "RakNetTypes.h"
#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "IncrementalReadInterface.h"
#include "RakAssert.h"
#include "RakAlloca.h"

#ifdef _MSC_VER
#pragma warning( push )
#endif

struct FLR_MemoryBlock
{
	unsigned int allocatedLength;
	char *block;
};

struct FileListReceiver
{
	FileListReceiver();
	~FileListReceiver();
	FileListTransferCBInterface *downloadHandler;
	SystemAddress allowedSender;
	unsigned short setID;
	unsigned setCount;
	unsigned setTotalCompressedTransmissionLength;
	unsigned setTotalFinalLength;
	unsigned setTotalDownloadedLength;
	bool gotSetHeader;
	bool deleteDownloadHandler;
	bool isCompressed;
	int  filesReceived;
	DataStructures::Map<unsigned int, FLR_MemoryBlock> pushedFiles;

	// Notifications
	unsigned int partLength;

};

FileListReceiver::FileListReceiver() {filesReceived=0; setTotalDownloadedLength=0; partLength=1; DataStructures::Map<unsigned int, FLR_MemoryBlock>::IMPLEMENT_DEFAULT_COMPARISON();}
FileListReceiver::~FileListReceiver() {
	unsigned int i=0;
	for (i=0; i < pushedFiles.Size(); i++)
		rakFree_Ex(pushedFiles[i].block, __FILE__, __LINE__ );
}

FileListTransfer::FileListTransfer()
{
	setId=0;
	callback=0;
	DataStructures::Map<unsigned short, FileListReceiver*>::IMPLEMENT_DEFAULT_COMPARISON();
}
FileListTransfer::~FileListTransfer()
{
	Clear();
}
unsigned short FileListTransfer::SetupReceive(FileListTransferCBInterface *handler, bool deleteHandler, SystemAddress allowedSender)
{
	if (rakPeerInterface && rakPeerInterface->IsConnected(allowedSender)==false)
		return (unsigned short)-1;
	FileListReceiver *receiver;

	if (fileListReceivers.Has(setId))
	{
		receiver=fileListReceivers.Get(setId);
		receiver->downloadHandler->OnDereference();
		if (receiver->deleteDownloadHandler)
			RakNet::OP_DELETE(receiver->downloadHandler, __FILE__, __LINE__);
		RakNet::OP_DELETE(receiver, __FILE__, __LINE__);
		fileListReceivers.Delete(setId);
	}

	unsigned short oldId;
	receiver = RakNet::OP_NEW<FileListReceiver>( __FILE__, __LINE__ );
	RakAssert(handler);
	receiver->downloadHandler=handler;
	receiver->allowedSender=allowedSender;
	receiver->gotSetHeader=false;
	receiver->deleteDownloadHandler=deleteHandler;
	fileListReceivers.Set(setId, receiver);
	oldId=setId;
	if (++setId==(unsigned short)-1)
		setId=0;
	return oldId;
}

void FileListTransfer::Send(FileList *fileList, RakPeerInterface *rakPeer, SystemAddress recipient, unsigned short setID, PacketPriority priority, char orderingChannel, bool compressData, IncrementalReadInterface *_incrementalReadInterface, unsigned int _chunkSize)
{
	(void) compressData;

	if (callback)
		fileList->SetCallback(callback);

	unsigned int i, totalLength;
	RakNet::BitStream outBitstream;
	bool sendReference;
	const char *dataBlocks[2];
	int lengths[2];
	totalLength=0;
	for (i=0; i < fileList->fileList.Size(); i++)
	{
		const FileListNode &fileListNode = fileList->fileList[i];
		totalLength+=fileListNode.fileLengthBytes;
	}

	// Write the chunk header, which contains the frequency table, the total number of files, and the total number of bytes
	bool anythingToWrite;
	outBitstream.Write((MessageID)ID_FILE_LIST_TRANSFER_HEADER);
	outBitstream.Write(setID);
	anythingToWrite=fileList->fileList.Size()>0;
	outBitstream.Write(anythingToWrite);
	if (anythingToWrite)
	{
		outBitstream.WriteCompressed(fileList->fileList.Size());
		outBitstream.WriteCompressed(totalLength);

		if (rakPeer)
			rakPeer->Send(&outBitstream, priority, RELIABLE_ORDERED, orderingChannel, recipient, false);
		else
			SendUnified(&outBitstream, priority, RELIABLE_ORDERED, orderingChannel, recipient, false);

		DataStructures::Queue<FileToPush*> filesToPush;
	
		for (i=0; i < fileList->fileList.Size(); i++)
		{
			sendReference = fileList->fileList[i].isAReference && _incrementalReadInterface!=0;
			if (sendReference)
			{
				FileToPush *fileToPush = RakNet::OP_NEW<FileToPush>(__FILE__,__LINE__);
				fileToPush->fileListNode.context=fileList->fileList[i].context;
				fileToPush->setIndex=i;
				fileToPush->fileListNode.filename=fileList->fileList[i].filename;
				fileToPush->fileListNode.fullPathToFile=fileList->fileList[i].fullPathToFile;
				fileToPush->fileListNode.fileLengthBytes=fileList->fileList[i].fileLengthBytes;
				fileToPush->fileListNode.dataLengthBytes=fileList->fileList[i].dataLengthBytes;
				//	fileToPush->systemAddress=recipient;
				fileToPush->setID=setID;
				fileToPush->packetPriority=priority;
				fileToPush->orderingChannel=orderingChannel;
				fileToPush->currentOffset=0;
				fileToPush->incrementalReadInterface=_incrementalReadInterface;
				fileToPush->chunkSize=_chunkSize;
				filesToPush.Push(fileToPush,__FILE__,__LINE__);
			}
			else
			{
				outBitstream.Reset();
				outBitstream.Write((MessageID)ID_FILE_LIST_TRANSFER_FILE);

				outBitstream.Write(fileList->fileList[i].context);
				outBitstream.Write(setID);
				stringCompressor->EncodeString(fileList->fileList[i].filename, 512, &outBitstream);

				outBitstream.WriteCompressed(i);
				outBitstream.WriteCompressed(fileList->fileList[i].dataLengthBytes); // Original length in bytes

				outBitstream.AlignWriteToByteBoundary();

				dataBlocks[0]=(char*) outBitstream.GetData();
				lengths[0]=outBitstream.GetNumberOfBytesUsed();
				dataBlocks[1]=fileList->fileList[i].data;
				lengths[1]=fileList->fileList[i].dataLengthBytes;
				SendListUnified(dataBlocks,lengths,2,priority, RELIABLE_ORDERED, orderingChannel, recipient, false);
			}
		}

		if (filesToPush.IsEmpty()==false)
		{
			FileToPushRecipient *ftpr=0;
			filesToPushAllSameAddressMutex.Lock();
			for (unsigned int i=0; i < filesToPushAllSameAddress.Size(); i++)
			{
				if (filesToPushAllSameAddress[i]->systemAddress==recipient)
				{
					ftpr=filesToPushAllSameAddress[i];
					break;
				}
			}
			if (ftpr==0)
			{
				ftpr = RakNet::OP_NEW<FileToPushRecipient>(__FILE__,__LINE__);
				ftpr->systemAddress=recipient;
				filesToPushAllSameAddress.Push(ftpr, __FILE__,__LINE__);
			}
			while (filesToPush.IsEmpty()==false)
			{
				ftpr->filesToPush.Push(filesToPush.Pop(), __FILE__,__LINE__);
			}
			filesToPushAllSameAddressMutex.Unlock();
			SendIRIToAddress(recipient);
			return;
		}
	}
	else
	{
		if (rakPeer)
			rakPeer->Send(&outBitstream, priority, RELIABLE_ORDERED, orderingChannel, recipient, false);
		else
			SendUnified(&outBitstream, priority, RELIABLE_ORDERED, orderingChannel, recipient, false);
	}
}

bool FileListTransfer::DecodeSetHeader(Packet *packet)
{
	bool anythingToWrite=false;
	unsigned short setID;
	RakNet::BitStream inBitStream(packet->data, packet->length, false);
	inBitStream.IgnoreBits(8);
	inBitStream.Read(setID);
	FileListReceiver *fileListReceiver;
	if (fileListReceivers.Has(setID)==false)
	{
#ifdef _DEBUG
		RakAssert(0);
#endif
		return false;
	}
	fileListReceiver=fileListReceivers.Get(setID);
	if (fileListReceiver->allowedSender!=packet->systemAddress)
	{
#ifdef _DEBUG
		RakAssert(0);
#endif
		return false;
	}

#ifdef _DEBUG
	RakAssert(fileListReceiver->gotSetHeader==false);
#endif

	inBitStream.Read(anythingToWrite);

	if (anythingToWrite)
	{
		inBitStream.ReadCompressed(fileListReceiver->setCount);
		if (inBitStream.ReadCompressed(fileListReceiver->setTotalFinalLength))
		{
			fileListReceiver->setTotalCompressedTransmissionLength=fileListReceiver->setTotalFinalLength;
			fileListReceiver->gotSetHeader=true;
			return true;
		}

	}
	else
	{
		if (fileListReceiver->downloadHandler->OnDownloadComplete()==false)
		{
			fileListReceiver->downloadHandler->OnDereference();
			fileListReceivers.Delete(setID);
			if (fileListReceiver->deleteDownloadHandler)
				RakNet::OP_DELETE(fileListReceiver->downloadHandler, __FILE__, __LINE__);
			RakNet::OP_DELETE(fileListReceiver, __FILE__, __LINE__);
		}

		return true;
	}

	return false;
}

bool FileListTransfer::DecodeFile(Packet *packet, bool fullFile)
{
	FileListTransferCBInterface::OnFileStruct onFileStruct;
	RakNet::BitStream inBitStream(packet->data, packet->length, false);
	inBitStream.IgnoreBits(8);

	unsigned int partCount=0;
	unsigned int partTotal=0;
	unsigned int partLength=0;
	onFileStruct.fileData=0;
	if (fullFile==false)
	{
		// Disable endian swapping on reading this, as it's generated locally in ReliabilityLayer.cpp
		inBitStream.ReadBits( (unsigned char* ) &partCount, BYTES_TO_BITS(sizeof(partCount)), true );
		inBitStream.ReadBits( (unsigned char* ) &partTotal, BYTES_TO_BITS(sizeof(partTotal)), true );
		inBitStream.ReadBits( (unsigned char* ) &partLength, BYTES_TO_BITS(sizeof(partLength)), true );
		inBitStream.IgnoreBits(8);
		// The header is appended to every chunk, which we continue to read after this statement block
	}
	inBitStream.Read(onFileStruct.context);
	inBitStream.Read(onFileStruct.setID);
	FileListReceiver *fileListReceiver;
	if (fileListReceivers.Has(onFileStruct.setID)==false)
	{
		return false;
	}
	fileListReceiver=fileListReceivers.Get(onFileStruct.setID);
	if (fileListReceiver->allowedSender!=packet->systemAddress)
	{
#ifdef _DEBUG
		RakAssert(0);
#endif
		return false;
	}

#ifdef _DEBUG
	RakAssert(fileListReceiver->gotSetHeader==true);
#endif

	if (stringCompressor->DecodeString(onFileStruct.fileName, 512, &inBitStream)==false)
	{
#ifdef _DEBUG
		RakAssert(0);
#endif
		return false;
	}

	inBitStream.ReadCompressed(onFileStruct.fileIndex);
	inBitStream.ReadCompressed(onFileStruct.byteLengthOfThisFile);
	onFileStruct.bytesDownloadedForThisFile=onFileStruct.byteLengthOfThisFile;


	if (fullFile)
	{
		// Support SendLists
		inBitStream.AlignReadToByteBoundary();

		onFileStruct.fileData = (char*) rakMalloc_Ex( (size_t) onFileStruct.byteLengthOfThisFile, __FILE__, __LINE__ );

		inBitStream.Read((char*)onFileStruct.fileData, onFileStruct.byteLengthOfThisFile);

		fileListReceiver->setTotalDownloadedLength+=onFileStruct.byteLengthOfThisFile;
	}
	

	onFileStruct.numberOfFilesInThisSet=fileListReceiver->setCount;
//	onFileStruct.setTotalCompressedTransmissionLength=fileListReceiver->setTotalCompressedTransmissionLength;
	onFileStruct.byteLengthOfThisSet=fileListReceiver->setTotalFinalLength;

	// User callback for this file.
	if (fullFile)
	{
		onFileStruct.bytesDownloadedForThisSet=fileListReceiver->setTotalDownloadedLength;
		if (fileListReceiver->downloadHandler->OnFile(&onFileStruct))
			rakFree_Ex(onFileStruct.fileData, __FILE__, __LINE__ );

		fileListReceiver->filesReceived++;

		// If this set is done, free the memory for it.
		if ((int) fileListReceiver->setCount==fileListReceiver->filesReceived)
		{
			if (fileListReceiver->downloadHandler->OnDownloadComplete()==false)
			{
				fileListReceiver->downloadHandler->OnDereference();
				if (fileListReceiver->deleteDownloadHandler)
					RakNet::OP_DELETE(fileListReceiver->downloadHandler, __FILE__, __LINE__);
				fileListReceivers.Delete(onFileStruct.setID);
				RakNet::OP_DELETE(fileListReceiver, __FILE__, __LINE__);
			}
		}

	}
	else
	{
		inBitStream.AlignReadToByteBoundary();

		bool usedAlloca=false;
		char *firstDataChunk;
		unsigned int unreadBits = inBitStream.GetNumberOfUnreadBits();
		unsigned int unreadBytes = BITS_TO_BYTES(unreadBits);
		if (unreadBytes>0)
		{
#if !defined(_XBOX) && !defined(_X360)
			if (unreadBits < MAX_ALLOCA_STACK_ALLOCATION)
			{
				firstDataChunk = ( char* ) alloca( unreadBytes );
				usedAlloca=true;
			}
			else
#endif
				firstDataChunk = (char*) rakMalloc_Ex( unreadBytes, __FILE__, __LINE__ );

			// Read partLength bytes, reported to OnFileProgress

			inBitStream.Read((char*)firstDataChunk, unreadBytes );

		}
		else
			firstDataChunk=0;

		onFileStruct.bytesDownloadedForThisSet=fileListReceiver->setTotalDownloadedLength+unreadBytes;
		onFileStruct.bytesDownloadedForThisFile=onFileStruct.byteLengthOfThisFile;

		fileListReceiver->downloadHandler->OnFileProgress(&onFileStruct, partCount, partTotal, unreadBytes, firstDataChunk);

		if (usedAlloca==false)
			RakNet::OP_DELETE_ARRAY(firstDataChunk, __FILE__, __LINE__);
	}

	return true;
}
PluginReceiveResult FileListTransfer::OnReceive(Packet *packet)
{
	switch (packet->data[0]) 
	{
	case ID_FILE_LIST_TRANSFER_HEADER:
		DecodeSetHeader(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_FILE_LIST_TRANSFER_FILE:
		DecodeFile(packet, true);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_FILE_LIST_REFERENCE_PUSH:
		OnReferencePush(packet, true);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_FILE_LIST_REFERENCE_PUSH_ACK:
		OnReferencePushAck(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_DOWNLOAD_PROGRESS:
		if (packet->length>sizeof(MessageID)+sizeof(unsigned int)*3)
		{
			if (packet->data[sizeof(MessageID)+sizeof(unsigned int)*3]==ID_FILE_LIST_TRANSFER_FILE)
			{
				DecodeFile(packet, false);
				return RR_STOP_PROCESSING_AND_DEALLOCATE;
			}
			if (packet->data[sizeof(MessageID)+sizeof(unsigned int)*3]==ID_FILE_LIST_REFERENCE_PUSH)
			{
				OnReferencePush(packet, false);
				return RR_STOP_PROCESSING_AND_DEALLOCATE;
			}
		}
		break;
	}

	return RR_CONTINUE_PROCESSING;
}
void FileListTransfer::OnShutdown(void)
{
	Clear();	
}
void FileListTransfer::Clear(void)
{
	unsigned i;
	for (i=0; i < fileListReceivers.Size(); i++)
	{
		fileListReceivers[i]->downloadHandler->OnDereference();
		if (fileListReceivers[i]->deleteDownloadHandler)
			RakNet::OP_DELETE(fileListReceivers[i]->downloadHandler, __FILE__, __LINE__);
		RakNet::OP_DELETE(fileListReceivers[i], __FILE__, __LINE__);
	}
	fileListReceivers.Clear();

	filesToPushAllSameAddressMutex.Lock();
	for (unsigned int i=0; i < filesToPushAllSameAddress.Size(); i++)
	{
		FileToPushRecipient *ftpr = filesToPushAllSameAddress[i];
		for (unsigned int j=0; j < ftpr->filesToPush.Size(); j++)
			RakNet::OP_DELETE(ftpr->filesToPush[j],__FILE__,__LINE__);
		RakNet::OP_DELETE(ftpr,__FILE__,__LINE__);
	}
	filesToPushAllSameAddress.Clear(false,__FILE__,__LINE__);
	filesToPushAllSameAddressMutex.Unlock();

	//filesToPush.Clear(false, __FILE__, __LINE__);
}
void FileListTransfer::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) rakNetGUID;

	RemoveReceiver(systemAddress);
}
void FileListTransfer::CancelReceive(unsigned short setId)
{
	if (fileListReceivers.Has(setId)==false)
	{
#ifdef _DEBUG
		RakAssert(0);
#endif
		return;
	}
	FileListReceiver *fileListReceiver=fileListReceivers.Get(setId);
	fileListReceiver->downloadHandler->OnDereference();
	if (fileListReceiver->deleteDownloadHandler)
		RakNet::OP_DELETE(fileListReceiver->downloadHandler, __FILE__, __LINE__);
	RakNet::OP_DELETE(fileListReceiver, __FILE__, __LINE__);
	fileListReceivers.Delete(setId);
}
void FileListTransfer::RemoveReceiver(SystemAddress systemAddress)
{
	unsigned i;
	i=0;
	while (i < fileListReceivers.Size())
	{
		if (fileListReceivers[i]->allowedSender==systemAddress)
		{
			fileListReceivers[i]->downloadHandler->OnDereference();
			if (fileListReceivers[i]->deleteDownloadHandler)
				RakNet::OP_DELETE(fileListReceivers[i]->downloadHandler, __FILE__, __LINE__);
			RakNet::OP_DELETE(fileListReceivers[i], __FILE__, __LINE__);
			fileListReceivers.RemoveAtIndex(i);
		}
		else
			i++;
	}

// 	i=0;
// 	while (i < filesToPush.Size())
// 	{
// 		if (filesToPush[i].systemAddress==systemAddress)
// 		{
// 			filesToPush.RemoveAtIndex(i);
// 		}
// 		else
// 			i++;
// 	}


	filesToPushAllSameAddressMutex.Lock();
	for (unsigned int i=0; i < filesToPushAllSameAddress.Size(); i++)
	{
		if (filesToPushAllSameAddress[i]->systemAddress==systemAddress)
		{
			FileToPushRecipient *ftpr = filesToPushAllSameAddress[i];
			for (unsigned int j=0; j < ftpr->filesToPush.Size(); j++)
			{
				RakNet::OP_DELETE(ftpr->filesToPush[j], __FILE__,__LINE__);
			}
			RakNet::OP_DELETE(ftpr, __FILE__,__LINE__);
			filesToPushAllSameAddress.RemoveAtIndexFast(i);
			break;
		}
	}
	filesToPushAllSameAddressMutex.Unlock();

}
bool FileListTransfer::IsHandlerActive(unsigned short setId)
{
	return fileListReceivers.Has(setId);
}
void FileListTransfer::SetCallback(FileListProgress *cb)
{
	callback=cb;
}
FileListProgress *FileListTransfer::GetCallback(void) const
{
	return callback;
}
void FileListTransfer::Update(void)
{
	unsigned i;
	i=0;
	while (i < fileListReceivers.Size())
	{
		if (fileListReceivers[i]->downloadHandler->Update()==false)
		{
			fileListReceivers[i]->downloadHandler->OnDereference();
			if (fileListReceivers[i]->deleteDownloadHandler)
				RakNet::OP_DELETE(fileListReceivers[i]->downloadHandler, __FILE__, __LINE__);
			RakNet::OP_DELETE(fileListReceivers[i], __FILE__, __LINE__);
			fileListReceivers.RemoveAtIndex(i);
		}
		else
			i++;
	}

}
void FileListTransfer::OnReferencePush(Packet *packet, bool fullFile)
{
	RakNet::BitStream refPushAck;
	if (fullFile)
	{
		// This is not a progress notification, it is actually the entire packet
		refPushAck.Write((MessageID)ID_FILE_LIST_REFERENCE_PUSH_ACK);
		SendUnified(&refPushAck,HIGH_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
	}

	FileListTransferCBInterface::OnFileStruct onFileStruct;
	RakNet::BitStream inBitStream(packet->data, packet->length, false);
	inBitStream.IgnoreBits(8);

	unsigned int partCount=0;
	unsigned int partTotal=1;
	unsigned int partLength=0;
	onFileStruct.fileData=0;
	if (fullFile==false)
	{
		// Disable endian swapping on reading this, as it's generated locally in ReliabilityLayer.cpp
		inBitStream.ReadBits( (unsigned char* ) &partCount, BYTES_TO_BITS(sizeof(partCount)), true );
		inBitStream.ReadBits( (unsigned char* ) &partTotal, BYTES_TO_BITS(sizeof(partTotal)), true );
		inBitStream.ReadBits( (unsigned char* ) &partLength, BYTES_TO_BITS(sizeof(partLength)), true );
		inBitStream.IgnoreBits(8);
		// The header is appended to every chunk, which we continue to read after this statement block
	}

	inBitStream.Read(onFileStruct.context);
	inBitStream.Read(onFileStruct.setID);
	FileListReceiver *fileListReceiver;
	if (fileListReceivers.Has(onFileStruct.setID)==false)
	{
		return;
	}
	fileListReceiver=fileListReceivers.Get(onFileStruct.setID);
	if (fileListReceiver->allowedSender!=packet->systemAddress)
	{
#ifdef _DEBUG
		RakAssert(0);
#endif
		return;
	}

#ifdef _DEBUG
	RakAssert(fileListReceiver->gotSetHeader==true);
#endif

	if (stringCompressor->DecodeString(onFileStruct.fileName, 512, &inBitStream)==false)
	{
#ifdef _DEBUG
		RakAssert(0);
#endif
		return;
	}

	inBitStream.ReadCompressed(onFileStruct.fileIndex);
	inBitStream.ReadCompressed(onFileStruct.byteLengthOfThisFile);
	unsigned int offset;
	unsigned int chunkLength;
	inBitStream.ReadCompressed(offset);
	inBitStream.ReadCompressed(chunkLength);
//	if (chunkLength==0)
//		return;
	bool lastChunk;
	inBitStream.Read(lastChunk);
	bool finished = lastChunk && fullFile;

	if (fullFile==false)
		fileListReceiver->partLength=partLength;

	FLR_MemoryBlock mb;
	if (fileListReceiver->pushedFiles.Has(onFileStruct.fileIndex)==false)
	{
		if (chunkLength > 1000000000 || onFileStruct.byteLengthOfThisFile > 1000000000)
		{
			RakAssert("FileListTransfer::OnReferencePush: file too large" && 0);
			return;
		}

		mb.allocatedLength=onFileStruct.byteLengthOfThisFile;
		mb.block = (char*) rakMalloc_Ex(onFileStruct.byteLengthOfThisFile, __FILE__, __LINE__);
		if (mb.block==0)
		{
			notifyOutOfMemory(__FILE__, __LINE__);
			return;
		}

		fileListReceiver->pushedFiles.SetNew(onFileStruct.fileIndex, mb);
	}
	else
	{
		mb=fileListReceiver->pushedFiles.Get(onFileStruct.fileIndex);
	}
	

	if (offset+chunkLength > mb.allocatedLength)
	{
		// Overrun
		RakAssert("FileListTransfer::OnReferencePush: Write would overrun allocated block" && 0);
		return;
	}

	// Read header uncompressed so the data is byte aligned, for speed
//	onFileStruct.compressedTransmissionLength=(unsigned int) onFileStruct.byteLengthOfThisFile;

	unsigned int unreadBits = inBitStream.GetNumberOfUnreadBits();
	unsigned int unreadBytes = BITS_TO_BYTES(unreadBits);
	unsigned int amountToRead;
	if (fullFile)
		amountToRead=chunkLength;
	else
		amountToRead=unreadBytes;

	inBitStream.AlignReadToByteBoundary();
//	if (fullFile ||
//		(
//		rakPeerInterface->GetSplitMessageProgressInterval() != 0 &&
//		(int)partCount==rakPeerInterface->GetSplitMessageProgressInterval())
//		)
	if (packet->data[0]!=ID_DOWNLOAD_PROGRESS)
	{
		inBitStream.Read(mb.block+offset, amountToRead);

		onFileStruct.bytesDownloadedForThisFile=offset+amountToRead;
	}
	else
	{
		fileListReceiver->setTotalDownloadedLength+=partLength;
		onFileStruct.bytesDownloadedForThisFile=partCount*partLength;
		
	}
	onFileStruct.bytesDownloadedForThisSet=fileListReceiver->setTotalDownloadedLength;

	onFileStruct.numberOfFilesInThisSet=fileListReceiver->setCount;
//	onFileStruct.setTotalCompressedTransmissionLength=fileListReceiver->setTotalCompressedTransmissionLength;
	onFileStruct.byteLengthOfThisSet=fileListReceiver->setTotalFinalLength;
	onFileStruct.fileData=mb.block;

	if (finished)
	{
		if (fileListReceiver->downloadHandler->OnFile(&onFileStruct))
			rakFree_Ex(onFileStruct.fileData, __FILE__, __LINE__ );
		fileListReceiver->pushedFiles.Delete(onFileStruct.fileIndex);

		fileListReceiver->filesReceived++;

		// If this set is done, free the memory for it.
		if ((int) fileListReceiver->setCount==fileListReceiver->filesReceived)
		{
			if (fileListReceiver->downloadHandler->OnDownloadComplete()==false)
			{
				fileListReceiver->downloadHandler->OnDereference();
				fileListReceivers.Delete(onFileStruct.setID);
				if (fileListReceiver->deleteDownloadHandler)
					RakNet::OP_DELETE(fileListReceiver->downloadHandler, __FILE__, __LINE__);
				RakNet::OP_DELETE(fileListReceiver, __FILE__, __LINE__);
			}
		}
	}
	else
	{
		unsigned int totalNotifications;
		unsigned int currentNotificationIndex;
		unsigned int unreadBytes;

		// if (rakPeerInterface==0 || rakPeerInterface->GetSplitMessageProgressInterval()==0)
		if (packet->data[0]!=ID_DOWNLOAD_PROGRESS)
		{
			totalNotifications = onFileStruct.byteLengthOfThisFile / chunkLength + 1;
			currentNotificationIndex = offset / chunkLength;
			unreadBytes = mb.allocatedLength - ((offset+1)*chunkLength);
			if (rakPeerInterface)
			{
				fileListReceiver->downloadHandler->OnReferencePush(&onFileStruct, currentNotificationIndex, totalNotifications, unreadBytes, mb.block);
			}
			else
			{
				// For TCP - this is all we get for small values of incremental push
				// Works with large values too
				fileListReceiver->downloadHandler->OnFileProgress(&onFileStruct, currentNotificationIndex, totalNotifications, unreadBytes, mb.block);
			}
		}
		else
		{
			totalNotifications = onFileStruct.byteLengthOfThisFile / fileListReceiver->partLength + 1;
			if (fullFile==false)
				currentNotificationIndex = (offset+partCount*fileListReceiver->partLength) / fileListReceiver->partLength ;
			else
				currentNotificationIndex = (offset+chunkLength) / fileListReceiver->partLength ;
			unreadBytes = onFileStruct.byteLengthOfThisFile - ((currentNotificationIndex+1) * fileListReceiver->partLength);

			// For TCP - incremental push. Works with large values for incremental push
			// This is ID_DOWNLOAD_PROGRESS returned from PacketizedTCP
			if (rakPeerInterface)
				fileListReceiver->downloadHandler->OnFileProgress(&onFileStruct, currentNotificationIndex, totalNotifications, unreadBytes, mb.block);
		}
	}

	return;
}
void FileListTransfer::SendIRIToAddress(SystemAddress systemAddress)
{
	// Was previously using GetStatistics to get outgoing buffer size, but TCP with UnifiedSend doesn't have this
	unsigned int bytesRead;	
	const char *dataBlocks[2];
	int lengths[2];
	unsigned int smallFileTotalSize=0;
	RakNet::BitStream outBitstream;

	filesToPushAllSameAddressMutex.Lock();
	for (unsigned int ftpIndex=0; ftpIndex < filesToPushAllSameAddress.Size(); ftpIndex++)
	{
		FileToPushRecipient *ftpr = filesToPushAllSameAddress[ftpIndex];
		if (ftpr->systemAddress==systemAddress)
		{
			FileToPush *ftp = ftpr->filesToPush.Peek();

			// Read and send chunk. If done, delete at this index
			void *buff = rakMalloc_Ex(ftp->chunkSize, __FILE__, __LINE__);
			if (buff==0)
			{
				filesToPushAllSameAddressMutex.Unlock();
				notifyOutOfMemory(__FILE__, __LINE__);
				return;
			}

			bytesRead=ftp->incrementalReadInterface->GetFilePart(ftp->fileListNode.fullPathToFile, ftp->currentOffset, ftp->chunkSize, buff, ftp->fileListNode.context);
			bool done = bytesRead!=ftp->chunkSize;
			while (done && ftp->currentOffset==0 && ftpr->filesToPush.Size()>=2 && smallFileTotalSize<ftp->chunkSize)
			{
				// Send all small files at once, rather than wait for ID_FILE_LIST_REFERENCE_PUSH. But at least one ID_FILE_LIST_REFERENCE_PUSH must be sent
				outBitstream.Reset();
				outBitstream.Write((MessageID)ID_FILE_LIST_TRANSFER_FILE);
				outBitstream.Write(ftp->fileListNode.context);
				outBitstream.Write(ftp->setID);
				stringCompressor->EncodeString(ftp->fileListNode.filename, 512, &outBitstream);
				outBitstream.WriteCompressed(ftp->setIndex);
				outBitstream.WriteCompressed(ftp->fileListNode.dataLengthBytes); // Original length in bytes
				outBitstream.AlignWriteToByteBoundary();
				dataBlocks[0]=(char*) outBitstream.GetData();
				lengths[0]=outBitstream.GetNumberOfBytesUsed();
				dataBlocks[1]=(const char*) buff;
				lengths[1]=bytesRead;
				SendListUnified(dataBlocks,lengths,2,ftp->packetPriority, RELIABLE_ORDERED, ftp->orderingChannel, systemAddress, false);

				RakNet::OP_DELETE(ftp,__FILE__,__LINE__);
				ftpr->filesToPush.Pop();
				smallFileTotalSize+=bytesRead;
				done = bytesRead!=ftp->chunkSize;
				ftp = ftpr->filesToPush.Peek();
				bytesRead=ftp->incrementalReadInterface->GetFilePart(ftp->fileListNode.fullPathToFile, ftp->currentOffset, ftp->chunkSize, buff, ftp->fileListNode.context);
			}


			outBitstream.Reset();
			outBitstream.Write((MessageID)ID_FILE_LIST_REFERENCE_PUSH);
			outBitstream.Write(ftp->fileListNode.context);
			outBitstream.Write(ftp->setID);
			stringCompressor->EncodeString(ftp->fileListNode.filename, 512, &outBitstream);
			outBitstream.WriteCompressed(ftp->setIndex);
			outBitstream.WriteCompressed(ftp->fileListNode.dataLengthBytes); // Original length in bytes
			outBitstream.WriteCompressed(ftp->currentOffset);
			ftp->currentOffset+=bytesRead;
			outBitstream.WriteCompressed(bytesRead);
			outBitstream.Write(done);

			if (callback)
			{
				callback->OnFilePush(ftp->fileListNode.filename, ftp->fileListNode.fileLengthBytes, ftp->currentOffset-bytesRead, bytesRead, done, systemAddress);
			}

			dataBlocks[0]=(char*) outBitstream.GetData();
			lengths[0]=outBitstream.GetNumberOfBytesUsed();
			dataBlocks[1]=(char*) buff;
			lengths[1]=bytesRead;
			//rakPeerInterface->SendList(dataBlocks,lengths,2,ftp->packetPriority, RELIABLE_ORDERED, ftp->orderingChannel, ftp->systemAddress, false);
			SendListUnified(dataBlocks,lengths,2,ftp->packetPriority, RELIABLE_ORDERED, ftp->orderingChannel, systemAddress, false);
			if (done)
			{
				// Done
				RakNet::OP_DELETE(ftp,__FILE__,__LINE__);
				ftpr->filesToPush.Pop();
				if (ftpr->filesToPush.Size()==0)
				{
					RakNet::OP_DELETE(ftpr,__FILE__,__LINE__);
					filesToPushAllSameAddress.RemoveAtIndexFast(ftpIndex);
				}
			}
			rakFree_Ex(buff, __FILE__, __LINE__ );
			break;
		}
	}
	filesToPushAllSameAddressMutex.Unlock();
}
void FileListTransfer::OnReferencePushAck(Packet *packet)
{
	SendIRIToAddress(packet->systemAddress);
}
unsigned int FileListTransfer::GetPendingFilesToAddress(SystemAddress recipient)
{
	filesToPushAllSameAddressMutex.Lock();
	for (unsigned int i=0; i < filesToPushAllSameAddress.Size(); i++)
	{
		if (filesToPushAllSameAddress[i]->systemAddress==recipient)
		{
			unsigned int size = filesToPushAllSameAddress[i]->filesToPush.Size();
			filesToPushAllSameAddressMutex.Unlock();
			return size;
		}

	}
	filesToPushAllSameAddressMutex.Unlock();
	return 0;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif
