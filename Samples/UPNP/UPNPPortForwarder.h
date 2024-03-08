#ifndef __UPNP_PORT_FORWADER
#define __UPNP_PORT_FORWADER

#include "UPNPNATInternal.h"

#include <stdarg.h>

#include "SocketLayer.h"
#include "RakPeerInterface.h"
#include "RakNetworkFactory.h"
#include "RakThread.h"
#include "DS_List.h"
#include "Rand.h"
#include "UPNPCallBackInterface.h"

/// \class UPNPPortForwarder
/// \brief Threaded interface to UPNP class for programmers to use
/// \details
/// \ingroup UPNP_GROUP
class UPNPPortForwarder 
{
public:
	UPNPPortForwarder();
	~UPNPPortForwarder();

	enum ProtocolType {UDP,TCP};

	/// Query if the router supports UPNP at all, or if there is no router
	/// Will call back UPNPCallBackInterface::NoRouterFoundOnAnyInterface(), or UPNPCallBackInterface::RouterFound()
	/// \param[in] callBack Pass in a class that has UPNPCallBackInterface as a superclass for the event notifications
	/// \return The number for the thread, if you choose to keep track of it. Returned in the callback as well.
	unsigned int QueryUPNPSupport(UPNPCallBackInterface *callBack);

	/// Creates a new thread and queries selected interface if selected, or all interfaces if not selected for a UPNP router and forwards the ports
	/// \param[in] internalPort The port number used on the interfaces that this machine is running on.
	/// \param[in] callBack Pass in a class that has UPNPCallBackInterface as a superclass for the event notifications
	/// \param[in] interfaceIp (Optional) The ip of the interface you wish to query for UPNP routers and open, by default,or if "ALL" is passed it will try all the interfaces
	/// \param[in] portToOpenOnRouter (Optional) The external port on the router that the outside sees, copies the internalPort by default or if set to -1
	/// \param[in] mappingName (Optional) This is the name that may show up under the router management, defaults to UPNPRAKNET and three random chars
	/// \param[in] protocol (Optional) What type of packets do you want to forward UDP or TCP, defaults to UDP
	/// \return The number for the thread, if you choose to keep track of it. Returned in the callback as well.
	unsigned int OpenPortOnInterface(int internalPort,UPNPCallBackInterface *callBack,RakNet::RakString interfaceIp="ALL",int portToOpenOnRouter=-1, RakNet::RakString mappingName="", ProtocolType protocol=UDP);

private:

	struct UPNPParamDesc //A descriptor if you want to search through the params and find out where it is located
	{
		RakNet::RakString name;
		RakNet::RakString type;
		RakNet::RakString subType;
		int index;

	};
	struct StandardUPNPParams//Flexible to pass to threads and make parameter maintanance easier, the way it is made supports search by 
	{//argument name even though a search function has not been implemented
		DataStructures::List <int> intList;
		DataStructures::List <RakNet::RakString > stringList;
		DataStructures::List <void*> pointerList;
		DataStructures::List <UPNPParamDesc> descList;
		int currentIntIndex;
		int currentStringIndex;
		int currentPointerIndex;
	};

	unsigned int threadNumber;//Increment on each thread call to give the thread a new id

	/// Creates a new parameter descriptor
	/// \param[in] index The index in the array it will be put into like intList,etc
	/// \param[in] name Parameter name
	/// \param[in] type The type of parameter like RakString,int,etc
	/// \param[in] subType (Optional) Typically used for pointers it indicates a custom subtype for your own use
	/// \return The newly created parameter descriptor
	struct UPNPParamDesc CreateParamDesc(int index,RakNet::RakString name,RakNet::RakString type="RakString", RakNet::RakString subType="None");

	/// Creates A new argument holding struct, initialized
	/// \return A new argument holding struct, initialized
	struct StandardUPNPParams * CreateNewParamList();

	/// Sets parameters for passing common to all current functions
	/// \param[in] StandardUPNPParams The param struct pointer you will be passing
	/// \param[in] threadNumber The current thread id
	/// \param[in] internalPort The port number used on the interfaces that this machine is running on.
	/// \param[in] callBack If you wish to recieve event notifications you can pass in a class that has UPNPCallBackInterface as a superclass
	/// \param[in] portToOpenOnRouter  The external port on the router that the outside sees, copies the internalPort by default or if set to -1
	/// \param[in] mappingName  This is the name that may show up under the router management, defaults to UPNPRAKNET and three random chars
	/// \param[in] protocol  What type of packets do you want to forward UDP or TCP, defaults to UDP
	void SetCommonParams(struct StandardUPNPParams* params,int threadNumber,int internalPort,UPNPCallBackInterface *callBack, int portToOpenOnRouter,RakNet::RakString mappingName, ProtocolType protocol);

	/// Adds an argument to be passed
	/// \param[in] inParam the actual paramater you wish to add, unless a pointer pass the address
	/// \param[in] StandardUPNPParams The param struct pointer you will be passing
	/// \param[in] name Parameter name
	/// \param[in] type The type of parameter like RakString,int,etc, should be the same as the template definition without the namespace quantifiers defaults to RakString
	/// \param[in] subType (Optional) Typically used for pointers it indicates a custom subtype for your own use


	void AddParam (void * inParam,struct StandardUPNPParams* params,RakNet::RakString name,RakNet::RakString type="RakString", RakNet::RakString subType="None");

	/// The thread to open the port on the interface/interfaces
	/// \param[in] arglist Should be a pointer to a properly made StandardUPNPParams
	static RAK_THREAD_DECLARATION(RunOpenPortOnAllInterfacesOrInterfaceThread);
};

#endif