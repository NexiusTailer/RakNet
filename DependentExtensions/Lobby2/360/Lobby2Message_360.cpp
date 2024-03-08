#include "Lobby2Message_360.h"
#include "Lobby2Client_360.h"
#include "AtgSignIn.h"

using namespace RakNet;

Client_Login_360::Client_Login_360()
{

}

bool Client_Login_360::ClientImpl( Lobby2Client *client)
{
	if (((Lobby2Client_360*) client)->ClientStartup())
	{
	//	ATG::SignIn::Initialize( 1, 4, TRUE, 4 );

		resultCode=L2RC_SUCCESS;
		return true; // Done, no callback
	}
	else
	{
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}
}
Client_Logoff_360::Client_Logoff_360()
{

}
bool Client_Logoff_360::ClientImpl( Lobby2Client *client)
{

	// Done, no callback
	resultCode=L2RC_SUCCESS;
	return true;
}

Console_CreateRoom_360::Console_CreateRoom_360()
{

}
bool Console_CreateRoom_360::ClientImpl( Lobby2Client *client)
{
//	XUserSetProperty( localConsoleUserIndexCreatingRoom, PROPERTY_VICTORY_POINTS, sizeof( DWORD ), &m_nVictoryPoints );
//	XUserSetContext( localConsoleUserIndexCreatingRoom, X_CONTEXT_GAME_MODE, m_nGameMode );
//	XUserSetContext( localConsoleUserIndexCreatingRoom, CONTEXT_MAP, m_nMap );
//	XUserSetContext( localConsoleUserIndexCreatingRoom, X_CONTEXT_GAME_TYPE, m_nGameType );

	ZeroMemory(&m_SessionInfo, sizeof(m_SessionInfo));

	DWORD ret = XSessionCreate(
		sessionFlagsToXSessionCreate | XSESSION_CREATE_HOST,
		localConsoleUserIndexCreatingRoom,
		publicSlots,
		privateSlots,
		&hostSessionNonce,
		&m_SessionInfo,
		&m_Overlapped,
		&sessionIdentificationHandle );

	if( ret != ERROR_IO_PENDING )
	{
		// Done, no callback
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	resultCode=L2RC_PROCESSING;
	return false;
}

Console_JoinRoom_360::Console_JoinRoom_360()
{

}
bool Console_JoinRoom_360::ClientImpl( Lobby2Client *client)
{
	DWORD ret = XSessionCreate(
		sessionFlagsToXSessionCreate,
		localConsoleUserIndexJoiningRoom,
		publicSlots,
		privateSlots,
		hostSessionNonce,
		m_SessionInfo,
		&m_Overlapped,
		sessionIdentificationHandle );

	if( ret != ERROR_IO_PENDING )
	{
		// Done, no callback
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	resultCode=L2RC_PROCESSING;
	return false;
}

Console_SearchRoomsInLobby_360::Console_SearchRoomsInLobby_360()
{
	m_pSearchResults=0;
}
Console_SearchRoomsInLobby_360::~Console_SearchRoomsInLobby_360()
{
	// Is NOT freed automatically. User should free when done with it using FreeSearchResults
//	rakFree(m_pSearchResults);
}
bool Console_SearchRoomsInLobby_360::ClientImpl( Lobby2Client *client)
{
	// Figure out the size of the max search results
	DWORD cbResults = 0;
	DWORD ret;

	// Pass in 0 for cbResults to have the buffer size 
	//  calculated for us by XTL
	ret = XSessionSearch(
		dwProcedureIndex, // Procedure index
		sessionOwner,					  // User index
		maxResultsToReturn,                // Maximum results
		0,                                // Number of properties   (ignored)
		0,                                // Number of contexts     (ignored)
		NULL,                             // Properties             (ignored)
		NULL,                             // Contexts               (ignored)
		&cbResults,                       // Size of result buffer
		NULL,                             // Pointer to results     (ignored)
		NULL                              // This call is always synchronous
		);

	if( ( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbResults == 0 ) )
	{
		// Done, no callback
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	m_pSearchResults = ( XSESSION_SEARCHRESULT_HEADER* ) rakMalloc (cbResults);

	if( !m_pSearchResults )
	{
		// Done, no callback
		resultCode=L2RC_OUT_OF_MEMORY;
		return true;
	}

	// Fire off the query
	ret = XSessionSearch(
		dwProcedureIndex, // Procedure index
		sessionOwner,					 // User index
		maxResultsToReturn,               // Maximum results
		searchProperties.Size(),                      // Number of properties
		searchContexts.Size(),                        // Number of contexts
		&searchProperties[0],                      // Properties
		&searchContexts[0],                        // Contexts
		&cbResults,                       // Size of result buffer
		m_pSearchResults,                 // Pointer to results
		&m_Overlapped                     // Overlapped data structure
		);

	if( ret != ERROR_IO_PENDING && ret != ERROR_SUCCESS )
	{
		// Done, no callback
		resultCode=L2RC_GENERAL_ERROR;
		return true;
	}

	resultCode=L2RC_PROCESSING;
	return false;
}
void Console_SearchRoomsInLobby_360::FreeSearchResults(PXSESSION_SEARCHRESULT_HEADER header)
{
	rakFree(header);
}