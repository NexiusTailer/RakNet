#ifndef __LOBBY_2_SERVER_H
#define __LOBBY_2_SERVER_H

#include "Export.h"
#include "RakNetTypes.h"
#include "Lobby2Plugin.h"
#include "DS_OrderedList.h"
#include "ThreadPool.h"

//class PostgreSQLInterface;

namespace RakNet
{
	
struct Lobby2Message;
class RoomsPlugin;

/// Commands are either messages from remote systems, or can be run by the local system
/// \internal
struct Lobby2ServerCommand
{
	Lobby2Message *lobby2Message;
	bool deallocMsgWhenDone;
	bool returnToSender;
	unsigned int callerUserId;
	RakNet::RakString callingUserName;
	SystemAddress callerSystemAddress;
	SystemAddress requiredConnectionAddress;
	RakNetGUID callerGuid;
	Lobby2Server *server;
};

/// The base class for the lobby server, without database specific functionality
/// This is a plugin which will take incoming messages via Lobby2Client_PC::SendMessage(), process them, and send back the same messages with output and a result code
/// Unlike the first implementation of the lobby server, this is a thin plugin that mostly just sends messages to threads and sends back the results.
class RAK_DLL_EXPORT Lobby2Server : public RakNet::Lobby2Plugin, public ThreadDataInterface
{
public:	
	Lobby2Server();
	virtual ~Lobby2Server();
	
	/// Connect to the database \a numWorkerThreads times using the connection string
	/// \param[in] conninfo See the postgre docs
	/// \return True on success, false on failure.
	virtual bool ConnectToDB(const char *conninfo, int numWorkerThreads)=0;
	/// \internal
	virtual void AddInputFromThread(Lobby2Message *msg, unsigned int targetUserId, RakNet::RakString targetUserHandle)=0;
	/// \internal
	virtual void AddOutputFromThread(Lobby2Message *msg, unsigned int targetUserId, RakNet::RakString targetUserHandle)=0;

	/// Lobby2Message encapsulates a user command, containing both input and output data
	/// This will serialize and transmit that command
	void SendMessage(Lobby2Message *msg, SystemAddress recipient);

	/// Add a command, which contains a message and other data such as who send the message.
	/// The command will be processed according to its implemented virtual functions. Most likely it will be processed in a thread to run database commands
	void ExecuteCommand(Lobby2ServerCommand *command);

	/// If Lobby2Message::RequiresAdmin() returns true, the message can only be processed from a remote system if the sender's system address is first added()
	/// This is useful if you want to administrate the server remotely
	void AddAdminAddress(SystemAddress addr);
	/// If AddAdminAddress() was previously called with \a addr then this returns true.
	bool IsAdminAddress(SystemAddress addr);
	/// Removes a system address previously added with AddAdminAddress()
	void RemoveAdminAddress(SystemAddress addr);
	/// Removes all system addresses previously added with AddAdminAddress()
	void ClearAdminAddresses(void);

	// This is a server which can submit ranking and match results
	void AddRankingAddress(SystemAddress addr);
	bool IsRankingAddress(SystemAddress addr);
	void RemoveRankingAddress(SystemAddress addr);
	void ClearRankingAddresses(void);
	
	/// The rooms plugin does not automatically handle users logging in and logging off, or renaming users.
	/// You can have Lobby2Server manage this for you by calling SetRoomsPlugin() with a pointer to the rooms plugin, if it is on the local system.
	/// This will call RoomsPlugin::LoginRoomsParticipant() and RoomsPlugin::LogoffRoomsParticipant() as the messages L2MID_Client_Login and L2MID_Client_Logoff arrive
	/// This will use the pointer to RoomsPlugin directly. Setting this will disable SetRoomsPluginAddress()
	/// \note This is an empty function. You must #define __INTEGRATE_LOBBY2_WITH_ROOMS_PLUGIN and link with RoomsPlugin.h to use it()
	void SetRoomsPlugin(RoomsPlugin *rp);

	/// This is similar to SetRoomsPlugin(), except the plugin is on another system.
	/// This will set the system address of that system to send the login and logoff commands to.
	/// For this function to work, you must first call RoomsPlugin::AddLoginServerAddress() so that RoomsPlugin will accept the incoming login and logoff messages.
	/// \note This is an empty function. You must #define __INTEGRATE_LOBBY2_WITH_ROOMS_PLUGIN and link with RoomsPlugin.h to use it()
	void SetRoomsPluginAddress(SystemAddress address);

	/// Server configuration properties, to customize how the server runs specific commands
	struct ConfigurationProperties
	{
		ConfigurationProperties() {requiresEmailAddressValidationToLogin=false; requiresTitleToLogin=false; accountRegistrationRequiredAgeYears=0;}

		/// If true, Client_Login will fail with Client_Login_EMAIL_ADDRESS_NOT_VALIDATED unless the email address registered with Client_RegisterAccount is verified with the command System_SetEmailAddressValidated
		bool requiresEmailAddressValidationToLogin;

		/// If true Client_Login::titleName and Client_Login::titleSecretKey must be previously registered with System_CreateTitle or Client_Login will fail with L2RC_Client_Login_BAD_TITLE_OR_TITLE_SECRET_KEY
		bool requiresTitleToLogin;

		/// If true, Client_RegisterAccount::cdKey must be previously registered with CDKey_Add or Client_RegisterAccount will fail with L2RC_Client_RegisterAccount_REQUIRES_CD_KEY or a related error message
		bool accountRegistrationRequiresCDKey;

		/// The minimum age needed to register accounts. If Client_RegisterAccount::createAccountParameters::ageInDays is less than this, then the account registration will fail with L2RC_Client_RegisterAccount_REQUIRED_AGE_NOT_MET
		/// Per-title age requirements can be handled client-side with System_CreateTitle::requiredAge and System_GetTitleRequiredAge
		unsigned int accountRegistrationRequiredAgeYears;
	};

	/// Set the desired configuration properties. This is read during runtime from threads.
	void SetConfigurationProperties(ConfigurationProperties c);
	/// Get the previously set configuration properties.
	const ConfigurationProperties *GetConfigurationProperties(void) const;

	/// \internal Lets the plugin know that a user has logged on, so this user can be tracked and the message forwarded to RoomsPlugin
	void OnLogin(Lobby2ServerCommand *command, bool calledFromThread);
	/// \internal Lets the plugin know that a user has logged off, so this user can be tracked and the message forwarded to RoomsPlugin
	void OnLogoff(Lobby2ServerCommand *command, bool calledFromThread);
	/// \internal Lets the plugin know that a user has been renamed, so this user can be tracked and the message forwarded to RoomsPlugin
	void OnChangeHandle(Lobby2ServerCommand *command, bool calledFromThread);

	/// \internal
	struct User
	{
		SystemAddress systemAddress;
		RakNetGUID guid;
		unsigned int callerUserId;
		RakNet::RakString userName;
	};

	/// \internal
	static int UserCompBySysAddr( const SystemAddress &key, Lobby2Server::User * const &data );

	/// \internal
	struct ThreadAction
	{
		Lobby2MessageID action;
		Lobby2ServerCommand command;
	};

	const DataStructures::OrderedList<SystemAddress, User*, Lobby2Server::UserCompBySysAddr>& GetUsers(void) const {return users;}

protected:

	void Update(void);
	PluginReceiveResult OnReceive(Packet *packet);
	void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );
	void OnShutdown(void);
	void OnMessage(Packet *packet);
	void Clear(void);
	void ClearUsers(void);
	unsigned int GetUserIndexBySystemAddress(SystemAddress systemAddress);
	void StopThreads(void);

	/// \internal
	void RemoveUser(SystemAddress address);
	/// \internal
	void RemoveUser(unsigned int index);

	virtual void* PerThreadFactory(void *context)=0;
	virtual void PerThreadDestructor(void* factoryResult, void *context)=0;
	virtual void AddInputCommand(Lobby2ServerCommand command)=0;
	virtual void ClearConnections(void)=0;

	DataStructures::OrderedList<SystemAddress, SystemAddress> adminAddresses;
	DataStructures::OrderedList<SystemAddress, SystemAddress> rankingAddresses;
	DataStructures::OrderedList<SystemAddress, User*, Lobby2Server::UserCompBySysAddr> users;
	RoomsPlugin *roomsPlugin;
	SystemAddress roomsPluginAddress;
	ThreadPool<Lobby2ServerCommand,Lobby2ServerCommand> threadPool;
	SimpleMutex connectionPoolMutex;
	ConfigurationProperties configurationProperties;
	DataStructures::Queue<ThreadAction> threadActionQueue;
	SimpleMutex threadActionQueueMutex;

	//DataStructures::List<PostgreSQLInterface *> connectionPool;
};
	
}

#endif
