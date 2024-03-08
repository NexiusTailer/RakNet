#ifdef SN_TARGET_PS3

#include "Lobby2Message_PS3.h"
#include "Lobby2Client_PS3.h"
#include <sys/process.h>
#include <cell/sysmodule.h>
#include <netex/net.h>
#include <netex/libnetctl.h>
#include <sysutil_common.h>
#include <sysutil_sysparam.h>
#include <cell/dbgfont.h>
#include <cell/pad.h>
#include <np.h>
#include "matching2_wrappers_Lobby2.h"
//#include "PS3Includes.h"

using namespace RakNet;

//------------------------------------------------------------------
// Globals
//------------------------------------------------------------------

//NP memory pool///////////////
uint8_t *np_pool=0;

//Flag/////////////////////////
static bool np_initialized = false;

//-------------------------------------------------------------------
// Functions Implementation
//-------------------------------------------------------------------

void SetSearchOperator(DataStructures::Table::FilterQueryType fq, SceNpMatching2Operator *op)
{
	switch (fq)
	{
	case DataStructures::Table::QF_EQUAL:
		*op=SCE_NP_MATCHING2_OPERATOR_EQ;
		break;
	case DataStructures::Table::QF_NOT_EQUAL:
		*op=SCE_NP_MATCHING2_OPERATOR_NE;
		break;
	case DataStructures::Table::QF_GREATER_THAN:
		*op=SCE_NP_MATCHING2_OPERATOR_GT;
		break;
	case DataStructures::Table::QF_GREATER_THAN_EQ:
		*op=SCE_NP_MATCHING2_OPERATOR_GE;
		break;
	case DataStructures::Table::QF_LESS_THAN:
		*op=SCE_NP_MATCHING2_OPERATOR_LT;
		break;
	case DataStructures::Table::QF_LESS_THAN_EQ:
		*op=SCE_NP_MATCHING2_OPERATOR_LE;
		break;
	case DataStructures::Table::QF_IS_EMPTY:
		RakAssert(0);
		break;
	case DataStructures::Table::QF_NOT_EMPTY:
		RakAssert(0);
		break;
	}

}
//***********************************************
// Function: NP_init()
// Description: Network initialization.
//***********************************************
int NP_init()
{
	int ret = 0;

	//First load the Network module PRX
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
	if(ret <0) {
//		PRINTF("cellSysmoduleLoadModule(CELL_SYSMODULE_NET) failed (0x%x)\n", ret);
		return ret;
	}

	//Load the libnetctl network connection information module PRX
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_NETCTL);
	if(ret <0) {
//		PRINTF("cellSysmoduleLoadModule(CELL_SYSMODULE_NETCTL) failed (0x%x)\n", ret);
		return ret;
	}

	//Load the NP PRX
//	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP );
//	if (ret < 0) {
		//		PRINTF("cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP ) failed (0x%x)\n", ret);
//		return ret;
//	}


	//Load the NP2 PRX
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP2);
	if (ret < 0) {
//		PRINTF("cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP2) failed (0x%x)\n", ret);
		return ret;
	}

	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_SYSUTIL_AVCHAT2 );
	if (ret < 0) {
		return ret;
	}

	//Initialize network
	ret = sys_net_initialize_network();
	if (ret < 0) {
//		PRINTF("sys_net_initialize_network() failed (0x%x)\n", ret);
		return ret;
	}

	//Initialize libnetctl
	ret = cellNetCtlInit();
	// -2146238206 is already initialized
	if (ret < 0  && ret!=-2146238206) {
//		PRINTF("cellNetCtlInit() failed (0x%x)\n", ret);
		return ret;
	}

	//Initialize NP2
	if (np_pool==0)
		np_pool=(uint8_t*) rakMalloc_Ex(sizeof(uint8_t) * SCE_NP_MIN_POOL_SIZE, __FILE__, __LINE__);
	ret = sceNp2Init(SCE_NP_MIN_POOL_SIZE, np_pool);
	if (ret < 0) {
		if((uint32_t)ret == SCE_NP_ERROR_ALREADY_INITIALIZED)
		{
//			PRINTF("sceNpInit() already initialized\n");
		}
		else
		{
			printf("sceNpInit() failed. ret = 0x%x\n", ret);
		}
	}


	np_initialized = true;

	return ret;
}

//***********************************************
// Function: NP_shutdown()
// Description: Network shutdown.
//***********************************************
int NP_shutdown()
{
	int ret = 0;

	if(np_initialized == true)
	{
		//Terminate NP
		ret = sceNp2Term();
//		if (ret < 0)
//			PRINTF("sceNpTerm() failed. ret = 0x%x\n", ret);

		//Terminate libnetctl
		cellNetCtlTerm();

		//Finilize network
		ret = sys_net_finalize_network();
//		if (ret < 0)
//			PRINTF("sys_net_finalize_network() failed. ret = 0x%x\n", ret);
	}

	//ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_NP);

	//Unload NP2 PRX
	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_NP2);
//	if (ret < 0)
//		PRINTF("cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_NP2) failed (0x%x)\n", ret);

	cellSysmoduleUnloadModule( CELL_SYSMODULE_SYSUTIL_AVCHAT2 );

	//Unload Network Module PRX
	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_NETCTL);
//	if(ret < 0)
//		PRINTF("cellSysmoduleUnloadModule(CELL_SYSMODULE_NETCTL) failed (0x%x)\n", ret);

	//Unload Network Module PRX
	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_NET);
//	if(ret < 0)
//		PRINTF("cellSysmoduleUnloadModule(CELL_SYSMODULE_NET) failed (0x%x)\n", ret);

	if (np_pool!=0)
	{
		rakFree_Ex(np_pool, __FILE__,__LINE__);
		np_pool=0;
	}

	np_initialized = false;
	return ret;
}




/*
Platform_Startup_PS3::Platform_Startup_PS3() {cellSysutilRegisterCallback_slot=0;}


bool Platform_Startup_PS3::ClientImpl( Lobby2Client *client)
{
	if (NP_init() < 0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	extendedResultCode = cellSysutilRegisterCallback(cellSysutilRegisterCallback_slot, Lobby2Client_PS3::sysutil_callback, client);
	if (extendedResultCode < 0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	ret = sys_net_initialize_network();
	if (ret < 0) {
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	ret = cellNetCtlInit();
	if (ret < 0) {
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	ret = sceNp2Init(SCE_NP_MIN_POOL_SIZE, ((Lobby2Client_PS3*)client)->np_pool);
	if (ret < 0) {
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	ret = sceNpMatching2Init2(0, 0, NULL);
	if (ret < 0) {
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	resultCode=L2RC_SUCCESS;

	// Call callback immediately
	return true;
}

bool Platform_Shutdown_PS3::ClientImpl( Lobby2Client *client)
{
	((Lobby2Client_PS3*)client)->ShutdownMatching2();

	cellSysutilUnregisterCallback(cellSysutilRegisterCallback_slot);

	NP_shutdown();
	
	// Call callback immediately
	resultCode=L2RC_SUCCESS;
	return true;
	}
	*/

Client_Login_PS3::Client_Login_PS3()
{
	cellSysutilRegisterCallback_slot=0;
	npCommId=0;
	npCommPassphrase=0;
	supportTitleUserStorage=true;

}

bool Client_Login_PS3::ClientImpl( Lobby2Client *client)
{
	if (npCommId==0 || npCommPassphrase==0)
	{
		resultCode=L2RC_PASSWORD_IS_EMPTY;
		return true;
	}

	extendedResultCode = NP_init();
	if (extendedResultCode < 0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	extendedResultCode = cellSysutilRegisterCallback(cellSysutilRegisterCallback_slot, Lobby2Client_PS3::sysutil_callback, client);
	if (extendedResultCode < 0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	CellNetCtlNetStartDialogParam netstart_param;

	memset(&netstart_param, 0, sizeof(netstart_param));
	netstart_param.size = sizeof(netstart_param);
	netstart_param.type = CELL_NET_CTL_NETSTART_TYPE_NP;

	extendedResultCode = cellNetCtlNetStartDialogLoadAsync(&netstart_param);
	if(extendedResultCode < 0){
		// DEBUG_PRINTF("cellNetCtlNetStartDialogLoadAsync() failed. ret = 0x%x\n", ret);
		resultCode=L2RC_GENERAL_ERROR;
		// Call callback immediately
		return true;
	}

	// Call callback deferred, for asynch processing on the client
	resultCode=L2RC_SUCCESS;
	return false;
}
Client_Logoff_PS3::Client_Logoff_PS3()
{
	
}
bool Client_Logoff_PS3::ClientImpl( Lobby2Client *client)
{
	cellSysutilRegisterCallback_slot=0;

	((Lobby2Client_PS3*)client)->ShutdownMatching2();

	if(np_initialized == true)
	{
		//Terminate NP
		extendedResultCode = sceNp2Term();
		//		if (ret < 0)
		//			PRINTF("sceNpTerm() failed. ret = 0x%x\n", ret);

		//Terminate libnetctl
		cellNetCtlTerm();

		//Finilize network
		extendedResultCode = sys_net_finalize_network();
		//		if (ret < 0)
		//			PRINTF("sys_net_finalize_network() failed. ret = 0x%x\n", ret);
	}

	np_initialized = false;
	resultCode=L2RC_SUCCESS;
	return true;
}
Client_PerTitleIntegerStorage_PS3::Client_PerTitleIntegerStorage_PS3()
{
	wasStarted=false;
}
Client_PerTitleIntegerStorage_PS3::~Client_PerTitleIntegerStorage_PS3()
{
	if (wasStarted)
	{
		if (resultCode==L2RC_PROCESSING)
			sceNpTusAbortTransaction(requestId);
		sceNpTusDestroyTransactionCtx(requestId);
	}

}
bool Client_PerTitleIntegerStorage_PS3::ClientImpl( Lobby2Client *client)
{
	if (((Lobby2Client_PS3*)client)->WasTUSInitialized()==false)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	extendedResultCode = sceNpTusCreateTransactionCtx(requestId);
	if (extendedResultCode<0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	wasStarted=true;

	Lobby2Client_PS3* clientPs3 = (Lobby2Client_PS3*)client;
	const SceNpId *targetNpId = clientPs3->GetNpId();
	SceNpTusSlotId slotId = slotIndex;
	int32_t transId = requestId;
	int32_t opeType;
	int64_t variable = (int64_t)inputValue;
	size_t resultVariableSize = sizeof(SceNpTusVariable);
	void *option=NULL;

	switch (addConditionForOperation)
	{
	case PTISC_EQUAL:
		opeType=SCE_NP_TUS_OPETYPE_EQUAL;
		break;
	case PTISC_NOT_EQUAL:
		opeType=SCE_NP_TUS_OPETYPE_NOT_EQUAL;
		break;
	case PTISC_GREATER_THAN:
		opeType=SCE_NP_TUS_OPETYPE_GREATER_THAN;
		break;
	case PTISC_GREATER_OR_EQUAL:
		opeType=SCE_NP_TUS_OPETYPE_GREATER_OR_EQUAL;
		break;
	case PTISC_LESS_THAN:
		opeType=SCE_NP_TUS_OPETYPE_LESS_THAN;
		break;
	case PTISC_LESS_OR_EQUAL:
		opeType=SCE_NP_TUS_OPETYPE_LESS_OR_EQUAL;
		break;
	}


	switch (operationToPerform)
	{
	case PTISO_WRITE:
		extendedResultCode=sceNpTusTryAndSetVariableAsync(transId, targetNpId, slotId, opeType, variable, &resultVariable, resultVariableSize, option);
		if (extendedResultCode<0)
		{
			resultCode=L2RC_GENERAL_ERROR;
			return true; // Done
		}
		break;
	case PTISO_ADD:
		extendedResultCode=sceNpTusAddAndGetVariableAsync(transId, targetNpId, slotId, variable, &resultVariable, resultVariableSize, option);
		if (extendedResultCode<0)
		{
			resultCode=L2RC_GENERAL_ERROR;
			return true; // Done
		}
		break;
	case PTISO_DELETE:
		extendedResultCode=sceNpTusDeleteMultiSlotVariableAsync(transId, targetNpId, &slotId, 1, option);
		if (extendedResultCode<0)
		{
			resultCode=L2RC_GENERAL_ERROR;
			return true; // Done
		}
		break;
	case PTISO_READ:
		extendedResultCode=sceNpTusGetMultiSlotVariableAsync(transId, targetNpId, &slotId, &resultVariable, resultVariableSize, 1, option);
		if (extendedResultCode<0)
		{
			resultCode=L2RC_GENERAL_ERROR;
			return true; // Done
		}
		break;
	}

	return false; // Deferred
}
bool Client_PerTitleIntegerStorage_PS3::WasCompleted(void)
{
	if (wasStarted==false)
		return true;
	bool working = sceNpTusPollAsync(requestId, &extendedResultCode);
	if (working==false)
	{
		resultCode=L2RC_SUCCESS;
		if (resultVariable.hasData==false)
		{
			resultCode=L2RC_Client_PerTitleIntegerStorage_ROW_EMPTY;
		}
		else
		{
			outputValue=(double)resultVariable.variable;
		}
	}
	return working==false;
}
Client_PerTitleBinaryStorage_PS3::Client_PerTitleBinaryStorage_PS3()
{
	wasStarted=false;
}
Client_PerTitleBinaryStorage_PS3::~Client_PerTitleBinaryStorage_PS3()
{
	if (wasStarted)
	{
		if (resultCode==L2RC_PROCESSING)
			sceNpTusAbortTransaction(requestId);
		sceNpTusDestroyTransactionCtx(requestId);
	}
}
bool Client_PerTitleBinaryStorage_PS3::ClientImpl( Lobby2Client *client)
{
	if (((Lobby2Client_PS3*)client)->WasTUSInitialized()==false)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	extendedResultCode = sceNpTusCreateTransactionCtx(requestId);
	if (extendedResultCode<0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	wasStarted=true;

	Lobby2Client_PS3* clientPs3 = (Lobby2Client_PS3*)client;
	const SceNpId *targetNpId = clientPs3->GetNpId();
	SceNpTusSlotId slotId = slotIndex;
	int32_t transId = requestId;
	void *option=NULL;
	SceNpTusDataInfo info;
	memset(&info, 0x00, sizeof(info));
	info.infoSize = 0;

	switch (operationToPerform)
	{
	case PTISO_WRITE:
		extendedResultCode=sceNpTusSetDataAsync(transId, targetNpId, slotId, binaryData->binaryDataLength, binaryData->binaryDataLength, binaryData->binaryData, &info, sizeof(SceNpTusDataInfo), option);
		if (extendedResultCode<0)
		{
			resultCode=L2RC_GENERAL_ERROR;
			return true; // Done
		}
		break;
	case PTISO_READ:
		extendedResultCode=sceNpTusGetDataAsync(transId, targetNpId, slotId, &dataStatus, sizeof(dataStatus), buffOut, sizeof(buffOut), option);
		if (extendedResultCode<0)
		{
			resultCode=L2RC_GENERAL_ERROR;
			return true; // Done
		}
		break;
	case PTISO_DELETE:
		extendedResultCode=sceNpTusDeleteMultiSlotData(transId, targetNpId, &slotId, 1, option);
		if (extendedResultCode<0)
		{
			resultCode=L2RC_GENERAL_ERROR;
			return true; // Done
		}
		break;
	}

	return false;
}
bool Client_PerTitleBinaryStorage_PS3::WasCompleted(void)
{
	if (wasStarted==false)
		return true;
	bool working = sceNpTusPollAsync(requestId, &extendedResultCode);
	if (working==false)
	{
		resultCode=L2RC_SUCCESS;
		if (operationToPerform==PTISO_READ)
		{
			if (dataStatus.hasData==false)
			{
				resultCode=L2RC_Client_PerTitleBinaryStorage_ROW_EMPTY;
			}
			else
			{
				binaryData->binaryData=(char*) rakMalloc_Ex(dataStatus.dataSize,__FILE__,__LINE__);
				binaryData->binaryDataLength=dataStatus.dataSize;
				memcpy(binaryData->binaryData, buffOut, dataStatus.dataSize);
			}
			binaryData->binaryDataLength=dataStatus.hasData;
		}
	}
	return working==false;
}
bool Console_GetServerStatus_PS3::ClientImpl( Lobby2Client *client)
{
	SceNpMatching2GetServerInfoRequest reqParam;
	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.serverId = serverId;	//Default use first server in the list.
	SceNpMatching2RequestOptParam optParam;
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = Lobby2Client_PS3::getServerInfoCb;	//Set callback
	optParam.cbFuncArg = client;

	//Make the request//////////////////////////////////////////
	extendedResultCode = sceNpMatching2GetServerInfo(((Lobby2Client_PS3*)client)->m_ctxId, &reqParam, &optParam, &requestId);
	if (extendedResultCode < 0) {
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	return false; // Deferred
}
Console_GetWorldListFromServer_PS3::Console_GetWorldListFromServer_PS3()
{
	response.world=0;
}
Console_GetWorldListFromServer_PS3::~Console_GetWorldListFromServer_PS3()
{
	RakNet::OP_DELETE_ARRAY<SceNpMatching2World>(response.world, __FILE__, __LINE__);
}
bool Console_GetWorldListFromServer_PS3::ClientImpl( Lobby2Client *client)
{
	//If everything is ok get the world list/////////////////
	SceNpMatching2GetWorldInfoListRequest reqParam;
	reqParam.serverId = serverId;
	SceNpMatching2RequestOptParam optParam;
	optParam.cbFunc = Lobby2Client_PS3::getWorldInfoListCb;
	optParam.cbFuncArg = client;
	optParam.timeout = 0;	//Default timeout

	//Make Matching2 request to get world list////////////////////
	extendedResultCode = sceNpMatching2GetWorldInfoList(((Lobby2Client_PS3*)client)->m_ctxId, &reqParam, &optParam, &requestId);
	if (extendedResultCode < 0) {
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false; // Deferred
}
Console_GetLobbyListFromWorld_PS3::Console_GetLobbyListFromWorld_PS3()
{
}
Console_GetLobbyListFromWorld_PS3::~Console_GetLobbyListFromWorld_PS3()
{
}
bool Console_GetLobbyListFromWorld_PS3::ClientImpl( Lobby2Client *client)
{
	SceNpMatching2GetLobbyInfoListRequest reqParam;
	memset(&reqParam, 0, sizeof(reqParam));

	reqParam.worldId = m_worldId;
	reqParam.rangeFilter.startIndex = SCE_NP_MATCHING2_RANGE_FILTER_START_INDEX_MIN;
	reqParam.rangeFilter.max = SCE_NP_MATCHING2_RANGE_FILTER_MAX;
	reqParam.attrId = NULL;
	reqParam.attrIdNum = 0;

	SceNpMatching2RequestOptParam optParam;
	memset(&optParam, 0, sizeof(optParam));

	optParam.cbFunc = Lobby2Client_PS3::getLobbyInfoListCb;
	optParam.cbFuncArg = client;
	optParam.timeout = 0;

	extendedResultCode = sceNpMatching2GetLobbyInfoList(((Lobby2Client_PS3*)client)->m_ctxId, &reqParam, &optParam, &requestId);
	if(extendedResultCode < 0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false;
}

bool Console_JoinLobby_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode = sample_join_lobby(((Lobby2Client_PS3*)client)->m_ctxId,
		&requestId,
		m_lobbyId,
		Lobby2Client_PS3::joinLobbyCb,
		client);


	resultCode=L2RC_SUCCESS;
	return false;
}

bool Console_LeaveLobby_PS3::ClientImpl( Lobby2Client *client)
{
	int ret = sample_leave_lobby(((Lobby2Client_PS3*)client)->m_ctxId, &requestId, 
		m_lobbyId, Lobby2Client_PS3::leaveLobbyCb, client);
	if(ret != CELL_OK)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false;
}

bool Console_SendLobbyChatMessage_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode = sample_send_lobby_chat_message(((Lobby2Client_PS3*)client)->m_ctxId,
		&requestId,
		m_lobbyId,
		message.C_String(),
		message.GetLength(),
		Lobby2Client_PS3::sendLobbyChatMsgCb,
		client);

	if(extendedResultCode < 0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false;
}
bool Console_SearchRooms_PS3::ClientImpl( Lobby2Client *client)
{
	SceNpMatching2IntSearchFilter intFilter[32];
	unsigned int intFilterNum=0;
	SceNpMatching2BinSearchFilter binFilter[32];
	unsigned int binFilterNum=0;
	memset(intFilter,0,sizeof(intFilter));
	memset(binFilter,0,sizeof(binFilter));

	unsigned int i;
	for (i=0; i < filterQueries.Size(); i++)
	{
		if (filterQueries[i]->cellValue->EstimateColumnType()==DataStructures::Table::NUMERIC)
		{
			SetSearchOperator(filterQueries[i]->operation, &intFilter[intFilterNum].searchOperator);
			intFilter[intFilterNum].attr.num=filterQueries[i]->cellValue->i;
			intFilter[intFilterNum].attr.id=SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_1_ID+intFilterNum;
			intFilterNum++;
		}
		else
		{
			SetSearchOperator(filterQueries[i]->operation, &binFilter[binFilterNum].searchOperator);
			binFilter[binFilterNum].attr.ptr=filterQueries[i]->cellValue->c;
			binFilter[binFilterNum].attr.size=filterQueries[i]->cellValue->i;
			binFilter[binFilterNum].attr.id=SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_1_ID+binFilterNum;
			binFilterNum++;
		}
	}

	//Make the request
	extendedResultCode = sample_search_room(((Lobby2Client_PS3*)client)->m_ctxId,
		&requestId,
		m_lobbyId,
		m_worldId,
		m_search_room_startIndex,
		rangeFilterMax,
		returnRandomResults,
		Lobby2Client_PS3::searchRoomCb,
		(SceNpMatching2IntSearchFilter *) &intFilter,
		intFilterNum,
		(SceNpMatching2BinSearchFilter *) &binFilter,
		binFilterNum,
		client);

	if(extendedResultCode != CELL_OK)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false;
}
bool Console_GetRoomDetails_PS3::ClientImpl( Lobby2Client *client)
{
	//Make the request
	extendedResultCode = sample_get_room_data_external_list(((Lobby2Client_PS3*)client)->m_ctxId,
		&requestId,
		&roomId,
		1,
		Lobby2Client_PS3::getRoomDataExternalListCb,
		client);
	if(extendedResultCode < 0)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false;
}
bool Console_GetLobbyMemberData_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode = sample_get_lobby_member_data_internal(((Lobby2Client_PS3*)client)->m_ctxId,
		&requestId,
		m_lobbyId,
		lobbyMemberId,
		Lobby2Client_PS3::getLobbyMemberDataInternalCb,
		client);

	if(extendedResultCode != CELL_OK)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false;
}

bool Console_CreateRoom_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode = sample_createjoin_room(((Lobby2Client_PS3*)client)->m_ctxId,
		&requestId,
		m_worldId,
		m_lobbyId,
		room_total_slots,
		Lobby2Client_PS3::createJoinRoomCb,
		client,
		hidden,
		numGroups,
		password.C_String(),
		passwordSlotMaskBits);

	if(extendedResultCode != CELL_OK)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false; // Asynch processing
}
bool Console_SetRoomSearchProperties_PS3::ClientImpl( Lobby2Client *client)
{
	SceNpMatching2SetRoomDataExternalRequest reqParam;
	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2IntAttr intFilter[32];
	unsigned int intFilterNum=0;
	SceNpMatching2BinAttr binFilter[32];
	unsigned int binFilterNum=0;

	//E Request parameters
	memset(&reqParam, 0, sizeof(reqParam));
	memset(&optParam, 0, sizeof(optParam));
	reqParam.roomId = roomId; //E Specify ID of current room

	reqParam.roomSearchableIntAttrExternal = intFilter;
	reqParam.roomSearchableBinAttrExternal = binFilter;

		
	unsigned int i;
	for (i=0; i < searchProperties.Size(); i++)
	{
		if (searchProperties[i]->EstimateColumnType()==DataStructures::Table::NUMERIC)
		{
			intFilter[intFilterNum].num=searchProperties[i]->i;
			intFilter[intFilterNum].id=SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_1_ID+intFilterNum;
			intFilterNum++;
		}
		else
		{
			binFilter[binFilterNum].ptr=searchProperties[i]->c;
			binFilter[binFilterNum].size=searchProperties[i]->i;
			binFilter[binFilterNum].id=SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_1_ID+binFilterNum;
			binFilterNum++;
		}
	}

	reqParam.roomSearchableBinAttrExternal=(SceNpMatching2BinAttr *) &binFilter;
	reqParam.roomSearchableBinAttrExternalNum=binFilterNum;
	reqParam.roomSearchableIntAttrExternal=(SceNpMatching2IntAttr *) &intFilter;
	reqParam.roomSearchableIntAttrExternalNum=intFilterNum;
	
	extendedResultCode = sceNpMatching2SetRoomDataExternal(
		((Lobby2Client_PS3*)client)->m_ctxId, &reqParam, &optParam, &requestId);
	if (extendedResultCode < 0) {
		//E Error handling
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false; // Asynch processing
}
bool Console_UpdateRoomParameters_PS3::ClientImpl( Lobby2Client *client)
{
	SceNpMatching2SetRoomDataInternalRequest reqParam;
	SceNpMatching2RequestOptParam optParam;

	//E Request parameters
	memset(&reqParam, 0, sizeof(reqParam));
	memset(&optParam, 0, sizeof(optParam));
	reqParam.roomId = roomId; //E Specify ID of current room
	reqParam.flagFilter = SCE_NP_MATCHING2_ROOM_FLAG_ATTR_HIDDEN | SCE_NP_MATCHING2_ROOM_FLAG_ATTR_CLOSED;
	if (isHidden)
	{
		reqParam.flagAttr = SCE_NP_MATCHING2_ROOM_FLAG_ATTR_HIDDEN;
	}
	if (isClosed)
	{
		reqParam.flagAttr |= SCE_NP_MATCHING2_ROOM_FLAG_ATTR_CLOSED;
	}
	reqParam.roomBinAttrInternal = NULL;
	reqParam.roomBinAttrInternalNum = 0;
	reqParam.passwordConfig = NULL;
	reqParam.passwordConfigNum = 0;
	reqParam.passwordSlotMask = NULL;
	reqParam.ownerPrivilegeRank = NULL;
	reqParam.ownerPrivilegeRankNum = 0;

	SceNpMatching2RoomPasswordSlotMask slotMask;
	memset(&slotMask, 0, sizeof(slotMask));
	for (int i=0; i < 32; i++)
	{
		if (passwordSlotMaskBits & (1<<i))
			SCE_NP_MATCHING2_ADD_SLOTNUM_TO_ROOM_PASSWORD_SLOT_MASK(slotMask, i);
	}
	reqParam.passwordSlotMask = &slotMask;


	extendedResultCode = sceNpMatching2SetRoomDataInternal(
		((Lobby2Client_PS3*)client)->m_ctxId, &reqParam, &optParam, &requestId);
	if (extendedResultCode < 0) {
		//E Error handling
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false; // Asynch processing
}
bool Console_JoinRoom_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode = sample_join_room(((Lobby2Client_PS3*)client)->m_ctxId,
		&requestId,
		roomId,
		Lobby2Client_PS3::joinRoomCb,
		client,
		joinGroupId);

	if(extendedResultCode != CELL_OK)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false; // Asynch processing
}

bool Console_LeaveRoom_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode = sample_leave_room(((Lobby2Client_PS3*)client)->m_ctxId, &requestId, 
		roomId, Lobby2Client_PS3::leaveRoomCb, client);
	
	if(extendedResultCode != CELL_OK)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false; // Asynch processing
}

bool Console_SendLobbyInvitationToRoom_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode = sample_send_lobby_invitation(((Lobby2Client_PS3*)client)->m_ctxId, &requestId,
		m_target_lobbyId, roomId, targetMemberId,
		NULL, client);
	
	if(extendedResultCode != CELL_OK)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false; // Asynch processing
}
bool Console_SendGUIInvitationToRoom_PS3::ClientImpl( Lobby2Client *client)
{
	SceNpBasicMessageDetails msg;
	memset(&msg,0,sizeof(msg));
	msg.mainType=SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE;
	msg.subType=SCE_NP_BASIC_MESSAGE_INVITE_SUBTYPE_ACTION_ACCEPT_DENY;
	msg.npids=recipient;
	msg.count=1;
	RakNet::RakString sub = defaultSubject;
	RakNet::RakString bod = defaultBody;
	sub.Truncate(SCE_NP_BASIC_SUBJECT_CHARACTER_MAX);
	bod.Truncate(SCE_NP_BASIC_BODY_CHARACTER_MAX);
	msg.subject=sub.C_String();
	msg.body=bod.C_String();
	
	sceNpBasicSendMessageGui(&msg, SYS_MEMORY_CONTAINER_ID_INVALID);
	return true; // Done
}
bool Console_SendDataMessageToUser_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode=sceNpBasicSendMessage(
		&recipient,
		bitStream.GetData(),
		bitStream.GetNumberOfBytesUsed());
	assert(bitStream.GetNumberOfBytesUsed()<SCE_NP_BASIC_MAX_MESSAGE_SIZE);
	if (extendedResultCode==0)
		resultCode=L2RC_SUCCESS;
	else if (extendedResultCode==SCE_NP_BASIC_ERROR_OUT_OF_MEMORY)
		resultCode=L2RC_OUT_OF_MEMORY;
	else if (extendedResultCode==SCE_NP_BASIC_ERROR_BUSY || extendedResultCode==SCE_NP_BASIC_ERROR_EXCEEDS_MAX)
		resultCode=L2RC_BUSY_EXCEEDED_PROCESSING_LIMIT;
	else
		resultCode=L2RC_GENERAL_ERROR;
	return true; // Done
}

bool Console_SendRoomChatMessage_PS3::ClientImpl( Lobby2Client *client)
{
	extendedResultCode = sample_send_room_chat_message(((Lobby2Client_PS3*)client)->m_ctxId,
		&requestId,
		roomId,
		message.C_String(),
		message.GetLength(),
		Lobby2Client_PS3::sendRoomChatMsgCb,
		client);

	if(extendedResultCode != CELL_OK)
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true; // Done
	}

	resultCode=L2RC_SUCCESS;
	return false; // Asynch processing
}
bool Console_SetPresence_PS3::ClientImpl( Lobby2Client *client)
{
	sceNpBasicSetPresence(
		presenceInfo.GetData(),
		presenceInfo.GetNumberOfBytesUsed()
		);

	resultCode=L2RC_SUCCESS;
	return true; // Done
}
Friends_GetFriends_PS3::Friends_GetFriends_PS3() {}
Friends_GetFriends_PS3::~Friends_GetFriends_PS3()
{
	for (unsigned int i=0; i < friendIds.Size(); i++)
		RakNet::OP_DELETE(friendIds[i],__FILE__,__LINE__);
}
Lobby2ResultCode Friends_GetFriends_PS3::PopulateFriends(void)
{
	uint32_t friend_count;
	int ret = sceNpBasicGetFriendListEntryCount(&friend_count);
	if (ret==SCE_NP_BASIC_ERROR_BUSY)
	{
		return L2RC_PROCESSING;
	}
	if(ret < 0)
	{
		return L2RC_GENERAL_ERROR;
	}

	SceNpUserInfo user_info;
	SceNpBasicPresenceDetails pres_details;
	SceNpId npId;
	for(unsigned int i=0; i<friend_count; ++i)
	{
		ret = sceNpBasicGetFriendPresenceByIndex(i, &user_info, &pres_details, 0);
		if(ret < 0)
		{
			return L2RC_GENERAL_ERROR;
		}

		if(pres_details.state != SCE_NP_BASIC_PRESENCE_STATE_OFFLINE)
		{
			ret = sceNpBasicGetFriendListEntry(i, &npId);
			if(ret < 0)
			{
				break;
			}

			SceNpId *idTemp = RakNet::OP_NEW<SceNpId>(__FILE__,__LINE__);
			memcpy(idTemp,&npId,sizeof(npId));
			friendIds.Push(idTemp, __FILE__, __LINE__);
			myFriends.Push(idTemp->handle.data,__FILE__,__LINE__);
		}
	}

	return L2RC_SUCCESS;
}
bool Friends_GetFriends_PS3::ClientImpl( Lobby2Client *client)
{
	return WasCompleted();
}

bool Friends_GetFriends_PS3::WasCompleted(void)
{
	resultCode = PopulateFriends();
	if (resultCode!=L2RC_PROCESSING)
		return true; // Done
	return false;
}

#endif // #ifdef SN_TARGET_PS3