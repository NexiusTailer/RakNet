#include "Lobby2Server.h"
#include "RakAssert.h"
#include "MessageIdentifiers.h"

// #define __INTEGRATE_LOBBY2_WITH_ROOMS_PLUGIN

#ifdef __INTEGRATE_LOBBY2_WITH_ROOMS_PLUGIN
#include "RoomsPlugin.h"
#endif

using namespace RakNet;

int Lobby2Server::UserCompBySysAddr( const SystemAddress &key, Lobby2Server::User * const &data )
{
	if (key < data->systemAddress)
		return -1;
	if (key==data->systemAddress)
		return 0;
	return 1;
}

Lobby2Server::Lobby2Server()
{
	DataStructures::OrderedList<SystemAddress, SystemAddress>::IMPLEMENT_DEFAULT_COMPARISON();
	roomsPlugin=0;
	roomsPluginAddress=UNASSIGNED_SYSTEM_ADDRESS;
}
Lobby2Server::~Lobby2Server()
{
	Clear();
}
void Lobby2Server::SendMessage(Lobby2Message *msg, SystemAddress recipient)
{
	RakNet::BitStream bs;
	bs.Write((MessageID)ID_LOBBY2_SEND_MESSAGE);
	bs.Write((MessageID)msg->GetID());
	msg->Serialize(true,true,&bs);
	rakPeer->Send(&bs,packetPriority, RELIABLE_ORDERED, orderingChannel, recipient, false);
}
void Lobby2Server::Update(RakPeerInterface *peer)
{
	while (threadActionQueue.Size())
	{
		threadActionQueueMutex.Lock();
		if (threadActionQueue.Size())
		{
			ThreadAction ta = threadActionQueue.Pop();
			threadActionQueueMutex.Unlock();
			if (ta.action==L2MID_Client_Logoff)
			{
				OnLogoff(&ta.command, false);
			}
			else if (ta.action==L2MID_Client_Login)
			{
				OnLogin(&ta.command, false);
			}
			else if (ta.action=L2MID_Client_ChangeHandle)
			{
				OnChangeHandle(&ta.command, false);
			}
		}
		else
		{
			threadActionQueueMutex.Unlock();
			break;
		}
	}

	if (threadPool.HasOutputFast() && threadPool.HasOutput())
	{
		Lobby2ServerCommand c = threadPool.GetOutput();
		if (c.returnToSender)
		{
			RakNet::BitStream bs;
			bs.Write((MessageID)ID_LOBBY2_SEND_MESSAGE);
			bs.Write((MessageID)c.lobby2Message->GetID());
			c.lobby2Message->Serialize(true,true,&bs);
			if (c.callerSystemAddress==UNASSIGNED_SYSTEM_ADDRESS)
			{
				unsigned int i;
				if (c.callerUserId!=0)
				{
					for (i=0; i < users.Size(); i++)
					{
						if (users[i]->callerUserId==c.callerUserId)
						{
							c.callerSystemAddress=users[i]->systemAddress;
							break;
						}
					}
				}
				if (c.callerSystemAddress==UNASSIGNED_SYSTEM_ADDRESS && c.callingUserName.IsEmpty()==false)
				{
					for (i=0; i < users.Size(); i++)
					{
						if (users[i]->userName==c.callingUserName)
						{
							c.callerSystemAddress=users[i]->systemAddress;
							break;
						}
					}
				}
			}
			rakPeer->Send(&bs,packetPriority, RELIABLE_ORDERED, orderingChannel, c.callerSystemAddress, false);
		}
		if (c.deallocMsgWhenDone)
			delete c.lobby2Message;
	}
}
PluginReceiveResult Lobby2Server::OnReceive(RakPeerInterface *peer, Packet *packet)
{
	RakAssert(packet);
	RakAssert(peer);

	switch (packet->data[0]) 
	{
	case ID_CONNECTION_LOST:
	case ID_DISCONNECTION_NOTIFICATION:
		OnCloseConnection(peer, packet->systemAddress);
		break;
	case ID_LOBBY2_SEND_MESSAGE:
		OnMessage(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}

	return RR_CONTINUE_PROCESSING;
}
void Lobby2Server::OnShutdown(RakPeerInterface *peer)
{
	Clear();
}
void Lobby2Server::OnCloseConnection(RakPeerInterface *peer, SystemAddress systemAddress)
{
	unsigned int index = GetUserIndexBySystemAddress(systemAddress);

	// If systemAddress is a user, then notify his friends about him logging off
	if (index!=-1)
	{
		RemoveUser(index);
	}
}
void Lobby2Server::OnMessage(Packet *packet)
{
	RakNet::BitStream bs(packet->data,packet->length,false);
	bs.IgnoreBytes(1); // ID_LOBBY2_SEND_MESSAGE
	MessageID msgId;
	bs.Read(msgId);
	Lobby2MessageID lobby2MessageID = (Lobby2MessageID) msgId;
	unsigned int index;
	Lobby2Message *lobby2Message = msgFactory->Alloc(lobby2MessageID);
	if (lobby2Message)
	{
		lobby2Message->Serialize(false,false,&bs);
		Lobby2ServerCommand command;
		command.lobby2Message=lobby2Message;
		command.deallocMsgWhenDone=true;
		command.returnToSender=true;
		index=GetUserIndexBySystemAddress(packet->systemAddress);
		if (index!=-1)
		{
			command.callingUserName=users[index]->userName;
			command.callerUserId=users[index]->callerUserId;
		}
		else
		{
			if (lobby2Message->RequiresLogin())
			{
				RakNet::BitStream bs;
				bs.Write((MessageID)ID_LOBBY2_SEND_MESSAGE);
				bs.Write((MessageID)lobby2Message->GetID());
				lobby2Message->resultCode=L2RC_NOT_LOGGED_IN;
				lobby2Message->Serialize(true,true,&bs);
				rakPeer->Send(&bs,packetPriority, RELIABLE_ORDERED, orderingChannel, packet->systemAddress, false);
				return;
			}
			command.callerUserId=0;
		}
		command.callerSystemAddress=packet->systemAddress;
		command.server=this;
		ExecuteCommand(&command);
	}
	else
	{
		RakNet::BitStream out;
		out.Write((MessageID)ID_LOBBY2_SERVER_ERROR);
		out.Write((unsigned char) L2SE_UNKNOWN_MESSAGE_ID);
		out.Write((unsigned int) msgId);
		rakPeer->Send(&bs,packetPriority, RELIABLE_ORDERED, orderingChannel, packet->systemAddress, false);
	}
}
void Lobby2Server::Clear(void)
{
	ClearAdminAddresses();
	ClearRankingAddresses();
	ClearUsers();
	ClearConnections();

	threadPool.StopThreads();
	RakAssert(threadPool.NumThreadsWorking()==0);

	unsigned i;
	Lobby2ServerCommand c;
	for (i=0; i < threadPool.InputSize(); i++)
	{
		c = threadPool.GetInputAtIndex(i);
		if (c.deallocMsgWhenDone && c.lobby2Message)
			delete c.lobby2Message;
	}
	threadPool.ClearInput();
	for (i=0; i < threadPool.OutputSize(); i++)
	{
		c = threadPool.GetOutputAtIndex(i);
		if (c.deallocMsgWhenDone && c.lobby2Message)
			delete c.lobby2Message;
	}
	threadPool.ClearOutput();

	threadActionQueueMutex.Lock();
	threadActionQueue.Clear();
	threadActionQueueMutex.Unlock();
}
void Lobby2Server::AddAdminAddress(SystemAddress addr)
{
	adminAddresses.Insert(addr,addr,false);
}
bool Lobby2Server::IsAdminAddress(SystemAddress addr)
{
	return addr==UNASSIGNED_SYSTEM_ADDRESS || adminAddresses.HasData(addr);
}
void Lobby2Server::RemoveAdminAddress(SystemAddress addr)
{
	adminAddresses.RemoveIfExists(addr);
}
void Lobby2Server::ClearAdminAddresses(void)
{
	adminAddresses.Clear();
}
void Lobby2Server::AddRankingAddress(SystemAddress addr)
{
	rankingAddresses.Insert(addr,addr,false);
}
bool Lobby2Server::IsRankingAddress(SystemAddress addr)
{
	return addr==UNASSIGNED_SYSTEM_ADDRESS || rankingAddresses.HasData(addr);
}
void Lobby2Server::RemoveRankingAddress(SystemAddress addr)
{
	rankingAddresses.RemoveIfExists(addr);
}
void Lobby2Server::ClearRankingAddresses(void)
{
	rankingAddresses.Clear();
}
void Lobby2Server::ExecuteCommand(Lobby2ServerCommand *command)
{
	RakNet::BitStream out;
	if (command->lobby2Message->PrevalidateInput()==false)
	{
		SendMessage(command->lobby2Message, command->callerSystemAddress);
		if (command->deallocMsgWhenDone)
			msgFactory->Dealloc(command->lobby2Message);
		return;
	}

	if (command->lobby2Message->RequiresAdmin() && IsAdminAddress(command->callerSystemAddress)==false)
	{
		command->lobby2Message->resultCode=L2RC_REQUIRES_ADMIN;
		SendMessage(command->lobby2Message, command->callerSystemAddress);
		rakPeer->Send(&out,packetPriority, RELIABLE_ORDERED, orderingChannel, command->callerSystemAddress, false);
		if (command->deallocMsgWhenDone)
			msgFactory->Dealloc(command->lobby2Message);
		return;
	}

	if (command->lobby2Message->RequiresRankingPermission() && IsRankingAddress(command->callerSystemAddress)==false)
	{
		command->lobby2Message->resultCode=L2RC_REQUIRES_ADMIN;
		SendMessage(command->lobby2Message, command->callerSystemAddress);
		rakPeer->Send(&out,packetPriority, RELIABLE_ORDERED, orderingChannel, command->callerSystemAddress, false);
		if (command->deallocMsgWhenDone)
			msgFactory->Dealloc(command->lobby2Message);
		return;
	}

	if (command->lobby2Message->ServerMemoryImpl(this, command->callerSystemAddress)==true)
	{
		SendMessage(command->lobby2Message, command->callerSystemAddress);
		if (command->deallocMsgWhenDone)
			msgFactory->Dealloc(command->lobby2Message);
		return;
	}

	command->server=this;
	AddInputCommand(*command);
}
void Lobby2Server::SetRoomsPlugin(RoomsPlugin *rp)
{
	roomsPlugin=rp;
	roomsPluginAddress=UNASSIGNED_SYSTEM_ADDRESS;
}
void Lobby2Server::SetRoomsPluginAddress(SystemAddress address)
{
	roomsPluginAddress=address;
	roomsPlugin=0;
}
void Lobby2Server::ClearUsers(void)
{
	unsigned int i;
	for (i=0; i < users.Size(); i++)
		delete users[i];
	users.Clear();
}
void Lobby2Server::OnLogin(Lobby2ServerCommand *command, bool calledFromThread)
{
	if (calledFromThread)
	{
		ThreadAction ta;
		ta.action=L2MID_Client_Login;
		ta.command=*command;
		threadActionQueueMutex.Lock();
		threadActionQueue.Push(ta);
		threadActionQueueMutex.Unlock();
		return;
	}

	bool objectExists;
	unsigned int insertionIndex = users.GetIndexFromKey(command->callerSystemAddress, &objectExists);
	User *user = new User;
	user->userName=command->callingUserName;
	user->systemAddress=command->callerSystemAddress;
	user->callerUserId=command->callerUserId;
	users.InsertAtIndex(user, insertionIndex);

#if defined(__INTEGRATE_LOBBY2_WITH_ROOMS_PLUGIN)
	// Tell the rooms plugin about the login event
	if (roomsPlugin)
	{
		roomsPlugin->LoginRoomsParticipant(user->userName, user->systemAddress, UNASSIGNED_SYSTEM_ADDRESS);
	}
	else if (roomsPluginAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		RakNet::BitStream bs;
		RoomsPlugin::SerializeLogin(user->userName,user->systemAddress, &bs);
		rakPeer->Send(&bs,packetPriority, RELIABLE_ORDERED, orderingChannel, roomsPluginAddress, false);
	}
#endif
}
void Lobby2Server::OnLogoff(Lobby2ServerCommand *command, bool calledFromThread)
{
	if (calledFromThread)
	{
		ThreadAction ta;
		ta.action=L2MID_Client_Logoff;
		ta.command=*command;
		threadActionQueueMutex.Lock();
		threadActionQueue.Push(ta);
		threadActionQueueMutex.Unlock();
		return;
	}
	RemoveUser(command->callerSystemAddress);
}
void Lobby2Server::OnChangeHandle(Lobby2ServerCommand *command, bool calledFromThread)
{
	if (calledFromThread)
	{
		ThreadAction ta;
		ta.action=L2MID_Client_ChangeHandle;
		ta.command=*command;
		threadActionQueueMutex.Lock();
		threadActionQueue.Push(ta);
		threadActionQueueMutex.Unlock();
		return;
	}

	unsigned int i;
	RakNet::RakString oldHandle;
	for (i=0; i < users.Size(); i++)
	{
		if (users[i]->callerUserId==command->callerUserId)
		{
			oldHandle=users[i]->userName;
			users[i]->userName=command->callingUserName;
			break;
		}
	}

	if (oldHandle.IsEmpty())
		return;

#if defined(__INTEGRATE_LOBBY2_WITH_ROOMS_PLUGIN)
	// Tell the rooms plugin about the handle change
	if (roomsPlugin)
	{
		roomsPlugin->ChangeHandle(oldHandle, command->callingUserName);
	}
	else if (roomsPluginAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		RakNet::BitStream bs;
		RoomsPlugin::SerializeChangeHandle(oldHandle,command->callingUserName,&bs);
		rakPeer->Send(&bs,packetPriority, RELIABLE_ORDERED, orderingChannel, roomsPluginAddress, false);
	}
#endif
}
void Lobby2Server::RemoveUser(SystemAddress address)
{
	bool objectExists;
	unsigned int index = users.GetIndexFromKey(address, &objectExists);
	if (objectExists)
		RemoveUser(index);
}
void Lobby2Server::RemoveUser(unsigned int index)
{
	User *user = users[index];

	Lobby2ServerCommand command;
	Notification_Friends_StatusChange *notification = (Notification_Friends_StatusChange *) GetMessageFactory()->Alloc(L2MID_Notification_Friends_StatusChange);
	notification->otherHandle=user->userName;
	notification->op=Notification_Friends_StatusChange::FRIEND_LOGGED_OFF;
	notification->resultCode=L2RC_SUCCESS;
	command.callerSystemAddress=UNASSIGNED_SYSTEM_ADDRESS;
	command.server=this;
	command.deallocMsgWhenDone=true;
	command.lobby2Message=notification;
	command.callerUserId=users[index]->callerUserId;
	command.callingUserName=users[index]->userName;
	ExecuteCommand(&command);

	unsigned i;
	i=0;
	threadPool.LockInput();
	while (i < threadPool.InputSize())
	{
		Lobby2ServerCommand command;
		command = threadPool.GetInputAtIndex(i);
		if (command.lobby2Message->CancelOnDisconnect() && command.callerSystemAddress==user->systemAddress)
		{
			if (command.deallocMsgWhenDone)
				delete command.lobby2Message;;
			threadPool.RemoveInputAtIndex(i);
		}
		else
			i++;
	}
	threadPool.UnlockInput();

#if defined(__INTEGRATE_LOBBY2_WITH_ROOMS_PLUGIN)
	// Tell the rooms plugin about the logoff event
	if (roomsPlugin)
	{
		roomsPlugin->LogoffRoomsParticipant(user->userName, user->systemAddress);
	}
	else if (roomsPluginAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		RakNet::BitStream bs;
		RoomsPlugin::SerializeLogoff(user->userName,&bs);
		rakPeer->Send(&bs,packetPriority, RELIABLE_ORDERED, orderingChannel, roomsPluginAddress, false);
	}
#endif
}
unsigned int Lobby2Server::GetUserIndexBySystemAddress(SystemAddress systemAddress)
{
	bool objectExists;
	unsigned int idx;
	idx = users.GetIndexFromKey(systemAddress, &objectExists);
	if (objectExists)
		return idx;
	return -1;
}
void Lobby2Server::StopThreads(void)
{
	threadPool.StopThreads();
}
void Lobby2Server::SetConfigurationProperties(ConfigurationProperties c)
{
	configurationProperties=c;
}
const Lobby2Server::ConfigurationProperties *Lobby2Server::GetConfigurationProperties(void) const
{
	return &configurationProperties;
}