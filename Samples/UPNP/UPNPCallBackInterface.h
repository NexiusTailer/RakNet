#pragma once
#include "RakString.h"
#include "RakNetTypes.h" 
#include "SimpleMutex.h"
#include "RakSleep.h"
#include "DS_List.h"

/*Virtual, but not pure virtual functions.

This allows the user to implement the interface without forcing the user to
override all the functions he does not want to use.


Some functions are not virtual so that thread safety can be enforced.

*/

/// \class UPNPCallBackInterface
/// \brief CallBack interface for use with the UPNPThreadedWrapper class
/// \details
/// \ingroup UPNP_GROUP

class SimpleMutex;

class UPNPCallBackInterface
{
public:
	UPNPCallBackInterface(void);
	virtual ~UPNPCallBackInterface(void);

	/// This is called to receive status updates, don't override if you don't wish to use 
	/// \param[in] stringToPrint The string that is being printed about what is going on.
	virtual void UPNPPrint(RakNet::RakString stringToPrint){(void) stringToPrint;}

	/// This is called when no UPNP router found on any interfaces, override to perform an action in that case
	/// \param[in] threadNumber The object specific thread number received when the function was called
	/// \param[in] internalPort The internal port given when the function was called
	/// \param[in] portToOpenOnRouter The port to open on the router,given when the function was called
	virtual void NoRouterFoundOnAnyInterface(int threadNumber,int internalPort,int portToOpenOnRouter){(void)threadNumber;(void)internalPort;(void)portToOpenOnRouter;}

	/// This is called when a router is found based on a call to UPNPPortForwarder::QueryUPNPSupport
	/// \param[in] threadNumber The object specific thread number received when the function was called
	virtual void RouterFound(int threadNumber) {}

	/// This is called when UPNP router was found, but port could not be opened on any interfaces
	/// \param[in] threadNumber The object specific thread number received when the function was called
	/// \param[in] internalPort The internal port given when the function was called
	/// \param[in] portToOpenOnRouter The port to open on the router,given when the function was called
	virtual void NoPortOpenedOnAnyInterface(int threadNumber,int internalPort,int portToOpenOnRouter){(void)threadNumber;(void)internalPort;(void)portToOpenOnRouter;}


	/// This is called when the thread has finished, whether or not it was a failure or success
	/// \param[in] threadNumber The object specific thread number received when the function was called
	/// \param[in] internalPort The internal port given when the function was called
	/// \param[in] portToOpenOnRouter The port to open on the router,given when the function was called
	/// \param[in] success Did the function succeed?
	virtual void ThreadFinished(int threadNumber,int internalPort,int portToOpenOnRouter,bool success){(void)threadNumber;(void)internalPort;(void)portToOpenOnRouter;(void)success;}

	/// This is used internally for thread safety, triggers the mutex lock.
	void Lock(){if (*internalReleaseTracker){return;}theLock->Lock();}

	/// This is used internally for thread safety, releases the mutex lock
	void UnLock(){if (*internalReleaseTracker){return;} theLock->Unlock();}

	/// This is used internally for thread safety, tracks if the callback was released for use by each thread
	/// \param[in] inReleaseTracker The tracker to add
	void SetReleaseTracker(bool &inReleaseTracker){releaseTrackers->Push(&inReleaseTracker,__FILE__,__LINE__);}

	/// This is used internally for thread safety, before thread returns removes tracker
	/// \param[in] inReleaseTracker The tracker to remove
	void RemoveReleaseTracker(bool &inReleaseTracker){releaseTrackers->RemoveAtIndexFast(releaseTrackers->GetIndexOf(&inReleaseTracker));}

private:
	SimpleMutex *theLock;// This is used internally for thread safety
	DataStructures::List <bool *> *releaseTrackers;
	bool * internalReleaseTracker;
};
