#include "SQLite3ServerPlugin.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"

using namespace RakNet;

bool operator<( const DataStructures::MLKeyRef<RakNet::RakString> &inputKey, const SQLite3ServerPlugin::NamedDBHandle &cls ) {return inputKey.Get() < cls.dbIdentifier;}
bool operator>( const DataStructures::MLKeyRef<RakNet::RakString> &inputKey, const SQLite3ServerPlugin::NamedDBHandle &cls ) {return inputKey.Get() > cls.dbIdentifier;}
bool operator==( const DataStructures::MLKeyRef<RakNet::RakString> &inputKey, const SQLite3ServerPlugin::NamedDBHandle &cls ) {return inputKey.Get() == cls.dbIdentifier;}

int PerRowCallback(void *userArgument, int argc, char **argv, char **azColName)
{
	SQLite3Table *outputTable = (SQLite3Table*)userArgument;
	DataStructures::DefaultIndexType idx;
	if (outputTable->columnNames.GetSize()==0)
	{
		for (idx=0; idx < (DataStructures::DefaultIndexType) argc; idx++)
			outputTable->columnNames.Push(azColName[idx]);
	}
	SQLite3Row *row = RakNet::OP_NEW<SQLite3Row>(__FILE__,__LINE__);
	outputTable->rows.Push(row,__FILE__,__LINE__);
	for (idx=0; idx < (DataStructures::DefaultIndexType) argc; idx++)
		row->entries.Push(argv[idx]);
	return 0;
}
SQLite3ServerPlugin::SQLite3ServerPlugin()
{
}
SQLite3ServerPlugin::~SQLite3ServerPlugin()
{
	StopThreads();
}
bool SQLite3ServerPlugin::AddDBHandle(RakNet::RakString dbIdentifier, sqlite3 *dbHandle)
{
	if (dbIdentifier.IsEmpty())
		return false;
	DataStructures::DefaultIndexType idx = dbHandles.GetInsertionIndex(dbIdentifier);
	if (idx==(DataStructures::DefaultIndexType)-1)
		return false;
	NamedDBHandle ndbh;
	ndbh.dbHandle=dbHandle;
	ndbh.dbIdentifier=dbIdentifier;
	dbHandles.InsertAtIndex(ndbh,idx,__FILE__,__LINE__);
	
#ifdef SQLite3_STATEMENT_EXECUTE_THREADED
	if (threadPool.WasStarted()==false)
		threadPool.StartThreads(1,0);
#endif

	return true;
}
void SQLite3ServerPlugin::RemoveDBHandle(RakNet::RakString dbIdentifier)
{
	DataStructures::DefaultIndexType idx = dbHandles.GetIndexOf(dbIdentifier);
	if (idx!=(DataStructures::DefaultIndexType)-1)
		dbHandles.RemoveAtIndex(idx,__FILE__,__LINE__);
#ifdef SQLite3_STATEMENT_EXECUTE_THREADED
	if (dbHandles.GetSize()==0)
		StopThreads();
#endif // SQLite3_STATEMENT_EXECUTE_THREADED
}
void SQLite3ServerPlugin::RemoveDBHandle(sqlite3 *dbHandle)
{
	DataStructures::DefaultIndexType idx;
	for (idx=0; idx < dbHandles.GetSize(); idx++)
	{
		if (dbHandles[idx].dbHandle==dbHandle)
		{
			dbHandles.RemoveAtIndex(idx,__FILE__,__LINE__);
#ifdef SQLite3_STATEMENT_EXECUTE_THREADED
			if (dbHandles.GetSize()==0)
				StopThreads();
#endif // SQLite3_STATEMENT_EXECUTE_THREADED
			return;
		}
	}
}
#ifdef SQLite3_STATEMENT_EXECUTE_THREADED
void SQLite3ServerPlugin::Update(void)
{
	ExecThreadOutput output;
	while (threadPool.HasOutputFast() && threadPool.HasOutput())
	{
		output = threadPool.GetOutput();
		RakNet::BitStream bsOut((unsigned char*) output.data, output.length,false);
		SendUnified(&bsOut, MEDIUM_PRIORITY,RELIABLE_ORDERED,0,output.sender,false);
		rakFree_Ex(output.data,__FILE__,__LINE__);
	}
}
SQLite3ServerPlugin::ExecThreadOutput ExecStatementThread(SQLite3ServerPlugin::ExecThreadInput threadInput, bool *returnOutput, void* perThreadData)
{
	unsigned int queryId;
	RakNet::RakString dbIdentifier;
	RakNet::RakString inputStatement;
	RakNet::BitStream bsIn((unsigned char*) threadInput.data, threadInput.length, false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	bsIn.Read(queryId);
	bsIn.Read(dbIdentifier);
	bsIn.Read(inputStatement);
	// bool isRequest;
	// bsIn.Read(isRequest);
	bsIn.IgnoreBits(1);

	char *errorMsg;
	RakNet::RakString errorMsgStr;
	SQLite3Table outputTable;					
	sqlite3_exec(threadInput.dbHandle, inputStatement.C_String(), PerRowCallback, &outputTable, &errorMsg);
	if (errorMsg)
	{
		errorMsgStr=errorMsg;
		sqlite3_free(errorMsg);
	}

	RakNet::BitStream bsOut;
	bsOut.Write((MessageID)ID_SQLite3_EXEC);
	bsOut.Write(queryId);
	bsOut.Write(dbIdentifier);
	bsOut.Write(inputStatement);
	bsOut.Write(false);
	bsOut.Write(errorMsgStr);
	outputTable.Serialize(&bsOut);

	// Free input data
	rakFree_Ex(threadInput.data,__FILE__,__LINE__);

	// Copy to output data
	SQLite3ServerPlugin::ExecThreadOutput threadOutput;
	threadOutput.data=(char*) rakMalloc_Ex(bsOut.GetNumberOfBytesUsed(),__FILE__,__LINE__);
	memcpy(threadOutput.data,bsOut.GetData(),bsOut.GetNumberOfBytesUsed());
	threadOutput.length=bsOut.GetNumberOfBytesUsed();
	threadOutput.sender=threadInput.sender;	
	// SendUnified(&bsOut, MEDIUM_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
	return threadOutput;
}
#endif // SQLite3_STATEMENT_EXECUTE_THREADED

PluginReceiveResult SQLite3ServerPlugin::OnReceive(Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_SQLite3_EXEC:
		{
			unsigned int queryId;
			RakNet::RakString dbIdentifier;
			RakNet::RakString inputStatement;
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(MessageID));
			bsIn.Read(queryId);
			bsIn.Read(dbIdentifier);
			bsIn.Read(inputStatement);
			bool isRequest;
			bsIn.Read(isRequest);
			if (isRequest)
			{
				// Server code

				DataStructures::DefaultIndexType idx = dbHandles.GetIndexOf(dbIdentifier);
				if (idx==-1)
				{
					RakNet::BitStream bsOut;
					bsOut.Write((MessageID)ID_SQLite3_UNKNOWN_DB);
					bsOut.Write(queryId);
					bsOut.Write(dbIdentifier);
					bsOut.Write(inputStatement);
					SendUnified(&bsOut, MEDIUM_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
				}
				else
				{
#ifdef SQLite3_STATEMENT_EXECUTE_THREADED
					// Push to the thread
					ExecThreadInput input;
					input.data=(char*) rakMalloc_Ex(packet->length, __FILE__,__LINE__);
					memcpy(input.data,packet->data,packet->length);
					input.dbHandle=dbHandles[idx].dbHandle;
					input.length=packet->length;
					input.sender=packet->systemAddress;
					threadPool.AddInput(ExecStatementThread, input);
#else
					char *errorMsg;
					RakNet::RakString errorMsgStr;
					SQLite3Table outputTable;					
					sqlite3_exec(dbHandles[idx].dbHandle, inputStatement.C_String(), PerRowCallback, &outputTable, &errorMsg);
					if (errorMsg)
					{
						errorMsgStr=errorMsg;
						sqlite3_free(errorMsg);
					}
					RakNet::BitStream bsOut;
					bsOut.Write((MessageID)ID_SQLite3_EXEC);
					bsOut.Write(queryId);
					bsOut.Write(dbIdentifier);
					bsOut.Write(inputStatement);
					bsOut.Write(false);
					bsOut.Write(errorMsgStr);
					outputTable.Serialize(&bsOut);
					SendUnified(&bsOut, MEDIUM_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
#endif
				}
			}
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
		break;
	}

	return RR_CONTINUE_PROCESSING;
}

void SQLite3ServerPlugin::OnAttach(void)
{
}
void SQLite3ServerPlugin::OnDetach(void)
{
	StopThreads();
}
void SQLite3ServerPlugin::StopThreads(void)
{
#ifdef SQLite3_STATEMENT_EXECUTE_THREADED
	threadPool.StopThreads();
	unsigned int i;
	for (i=0; i < threadPool.InputSize(); i++)
	{
		RakNet::OP_DELETE(threadPool.GetInputAtIndex(i).data, __FILE__, __LINE__);
	}
	threadPool.ClearInput();
	for (i=0; i < threadPool.OutputSize(); i++)
	{
		RakNet::OP_DELETE(threadPool.GetOutputAtIndex(i).data, __FILE__, __LINE__);
	}
	threadPool.ClearOutput();
#endif
}