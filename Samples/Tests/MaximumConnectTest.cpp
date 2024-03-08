#include "MaximumConnectTest.h"





/*
What is being done here is having 8 peers all connect to eachother over the max defined connection.

It runs the connect, wait 20 seconds then see the current connections.


Success conditions:
All extra connections Refused.

Failure conditions:
There are more connected than allowed.
The connect function fails, the test is not even done.


*/
int MaximumConnectTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{

    const int peerNum= 8;
    const int maxConnections=4;//Max allowed connections for test
    RakPeerInterface *peerList[peerNum];//A list of 8 peers
    int connectionAmount[peerNum];//Counter for me to keep track of connection requests and accepts


    char str[512];

    Packet *packet;





    //Initializations of the arrays
    for (int i=0;i<peerNum;i++)
    {
        peerList[i]=RakNetworkFactory::GetRakPeerInterface();
        connectionAmount[i]=0;




        peerList[i]->Startup(maxConnections, 30, &SocketDescriptor(60000+i,0), 1);
        peerList[i]->SetMaximumIncomingConnections(maxConnections);

    }




    //Connect all the peers together

    strcpy(str, "127.0.0.1");

    for (int i=0;i<peerNum;i++)
    {

        for (int j=i+1;j<peerNum;j++)//Start at i+1 so don't connect two of the same together.
        {

            if (!peerList[i]->Connect(str, 60000+j, 0,0))
            {

                if (isVerbose)
                    printf("Problem while calling connect. Enter to continue. \n");

                if (!noPauses && isVerbose)
                    gets(str);
                return 1;//This fails the test, don't bother going on.

            }

        }	

    }



    RakNetTime entryTime=RakNet::GetTime();//Loop entry time



    while(RakNet::GetTime()-entryTime<20000)//Run for 20 Secoonds
    {





        for (int i=0;i<peerNum;i++)//Receive for all peers
        {
            if (isVerbose)
                printf("For peer %i\n",i);

            packet=peerList[i]->Receive();


            while(packet)
            {
                switch (packet->data[0])
                {
                case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                    if (isVerbose)
                        printf("Another client has disconnected.\n");

                    break;
                case ID_REMOTE_CONNECTION_LOST:
                    if (isVerbose)
                        printf("Another client has lost the connection.\n");

                    break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION:
                    if (isVerbose)              
                        printf("Another client has connected.\n");
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    if (isVerbose)              
                        printf("Our connection request has been accepted.\n");


                    break;
                case ID_CONNECTION_ATTEMPT_FAILED:
                    if (isVerbose)
                        printf("A connection has failed.\n");

                    break;

                case ID_NEW_INCOMING_CONNECTION:
                    if (isVerbose)              
                        printf("A connection is incoming.\n");

                    break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    if (isVerbose)              
                        printf("The server is full.\n");


                    break;

                case ID_ALREADY_CONNECTED:
                    if (isVerbose)              
                        printf("Already connected\n");//Shouldn't happen

                    break;


                case ID_DISCONNECTION_NOTIFICATION:
                    if (isVerbose)
                        printf("We have been disconnected.\n");
                    break;
                case ID_CONNECTION_LOST:
                    if (isVerbose)
                        printf("Connection lost.\n");

                    break;
                default:

                    break;
                }

                peerList[i]->DeallocatePacket(packet);

                // Stay in the loop as long as there are more packets.
                packet = peerList[i]->Receive();
            }
        }
        RakSleep(0);//If needed for testing
    }

    DataStructures::List< SystemAddress  > systemList;
    DataStructures::List< RakNetGUID > guidList;


    for (int i=0;i<peerNum;i++)
    {

        peerList[i]->GetSystemList(systemList,guidList);
        int connNum=guidList.Size();//Get the number of connections for the current peer
        if (connNum>maxConnections)//Did we connect to more?
        {


            if (isVerbose)
                printf("More connections were allowed to peer %i, %i total.Fail\n",i,connNum);

            for (int i=0;i<peerNum;i++)//Clean
            {
                RakNetworkFactory::DestroyRakPeerInterface(peerList[i]);
            }


            return 2;


        }


    }


    for (int i=0;i<peerNum;i++)//Clean
    {
        RakNetworkFactory::DestroyRakPeerInterface(peerList[i]);
    }

    if (isVerbose)
        printf("Pass\n");
    return 0;

}



RakNet::RakString MaximumConnectTest::GetTestName()
{

    return "MaximumConnectTest";

}

RakNet::RakString MaximumConnectTest::ErrorCodeToString(int errorCode)
{

    switch (errorCode)
    {

    case 0:
        return "No error";
        break;

    case 1:
        return "The connect function failed.";
        break;

    case 2:
        return "An extra connection was allowed.";
        break;

    default:
        return "Undefined Error";
    }


}




MaximumConnectTest::MaximumConnectTest(void)
{
}

MaximumConnectTest::~MaximumConnectTest(void)
{
}
