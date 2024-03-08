#include "UPNPPortForwarder.h"
#include "UPNPCallBackInterface.h"
#include "EventReceiver.h"




int main (int argc,char * argv[]){

	UPNPPortForwarder testInterface;

	EventReceiver * eventObject= new EventReceiver(3);

	//Default is same port is open on router that is used internally
	testInterface.OpenPortOnInterface(65534,eventObject);
	testInterface.OpenPortOnInterface(65535,eventObject);
	testInterface.OpenPortOnInterface(65533,eventObject);



	while(eventObject->IsRunningAnyThread())
	{}

	delete eventObject;

	return 0;


}
