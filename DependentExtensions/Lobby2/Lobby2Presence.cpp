#include "Lobby2Presence.h"
#include "BitStream.h"

using namespace RakNet;


Lobby2Presence::Lobby2Presence() {
	status=UNDEFINED;
	isVisible=true;
}
Lobby2Presence::Lobby2Presence(const Lobby2Presence& input) {
	status=input.status;
	isVisible=input.isVisible;
	titleNameOrID=input.titleNameOrID;
#if defined(_XBOX) || defined(X360)
                                                                    
#endif
	statusString=input.statusString;
}
Lobby2Presence& Lobby2Presence::operator = ( const Lobby2Presence& input )
{
	status=input.status;
	isVisible=input.isVisible;
	titleNameOrID=input.titleNameOrID;
#if defined(_XBOX) || defined(X360)
                                                                    
#endif
	statusString=input.statusString;
	return *this;
}

Lobby2Presence::~Lobby2Presence()
{

}
void Lobby2Presence::Serialize(RakNet::BitStream *bitStream, bool writeToBitstream)
{
	unsigned char gs = (unsigned char) status;
	bitStream->Serialize(writeToBitstream,gs);
	status=(Status) gs;
	bitStream->Serialize(writeToBitstream,isVisible);
	bitStream->Serialize(writeToBitstream,titleNameOrID);
#if defined(_XBOX) || defined(X360)
                                                                                                               
#endif
	bitStream->Serialize(writeToBitstream,statusString);
}
