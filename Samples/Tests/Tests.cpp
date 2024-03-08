#include "IncludeAllTests.h"

#include "RakString.h"
#include "DS_List.h"

/*

The TestInterface used in this was made to be able to be flexible
when adding new test cases. While I do not use the paramslist, it is available.

*/

int main(int argc, char *argv[])
{

	int returnVal;
	char str[512];
	int numTests=0;
	int testListSize=0;
	int passedTests=0;
	DataStructures::List <TestInterface*> testList;//Pointer list
	DataStructures::List <int> testResultList;//A list of pass and/or fail and the error codes
	DataStructures::List <RakNet::RakString> testsToRun;//A list of tests to run
	DataStructures::List <int> testsToRunIndexes;//A list of tests to run by index

	//Add all the tests to the test list
	testList.Push(new EightPeerTest(),__FILE__,__LINE__);
	testList.Push(new MaximumConnectTest(),__FILE__,__LINE__);
	testList.Push(new PeerConnectDisconnectWithCancelPendingTest(),__FILE__,__LINE__);
	testList.Push(new PeerConnectDisconnectTest(),__FILE__,__LINE__);
	testList.Push(new ManyClientsOneServerBlockingTest(),__FILE__,__LINE__);
	testList.Push(new ManyClientsOneServerNonBlockingTest(),__FILE__,__LINE__);
	testList.Push(new ManyClientsOneServerDeallocateBlockingTest(),__FILE__,__LINE__);
	testList.Push(new ReliableOrderedConvertedTest(),__FILE__,__LINE__);
	testList.Push(new DroppedConnectionConvertTest(),__FILE__,__LINE__);
	testList.Push(new ComprehensiveConvertTest(),__FILE__,__LINE__);
	testList.Push(new CrossConnectionConvertTest(),__FILE__,__LINE__);
	testList.Push(new PingTestsTest(),__FILE__,__LINE__);
	testList.Push(new OfflineMessagesConvertTest(),__FILE__,__LINE__);
	testList.Push(new CompressionConvertTest(),__FILE__,__LINE__);
	testList.Push(new LocalIsConnectedTest(),__FILE__,__LINE__);
	testList.Push(new SecurityFunctionsTest(),__FILE__,__LINE__);
	testList.Push(new ConnectWithSocketTest(),__FILE__,__LINE__);
	testList.Push(new SystemAddressAndGuidTest(),__FILE__,__LINE__);	
	testList.Push(new PacketAndLowLevelTestsTest(),__FILE__,__LINE__);
	testList.Push(new MiscellaneousTestsTest(),__FILE__,__LINE__);

	testListSize=testList.Size();

	bool isVerbose=true;
	bool disallowTestToPause=false;

	DataStructures::List<RakNet::RakString> testcases;

	if (argc>1)//we have command line arguments
	{

		for (int p=1;p<argc;p++)
		{
			testsToRun.Push(argv[p],__FILE__,__LINE__);

		}

	}

	DataStructures::List<RakNet::RakString> noParamsList;

	if (testsToRun.Size()==0&&testsToRunIndexes.Size()==0)
	{
		numTests=testListSize;
		for(int i=0;i<testListSize;i++)
		{
			printf("\n\nRunning test %s.\n\n",testList[i]->GetTestName().C_String());
			returnVal=testList[i]->RunTest(noParamsList,isVerbose,disallowTestToPause);
			testList[i]->DestroyPeers();

			if (returnVal==0)
			{passedTests++;}
			else
			{
				printf("Test %s returned with error %s",testList[i]->GetTestName().C_String(),testList[i]->ErrorCodeToString(returnVal).C_String());
		
			}
		}

	}

	if (testsToRun.Size()!=0)//If string arguments convert to indexes.Suggestion: if speed becoms an issue keep a sorted list and binary search it
	{
		int TestsToRunSize= testsToRun.Size();

		RakNet::RakString testName;
		for(int i=0;i<TestsToRunSize;i++)
		{
			testName=testsToRun[i];

			for(int j=0;j<testListSize;j++)
			{

				if (testList[j]->GetTestName().StrICmp(testName)==0)
				{

					testsToRunIndexes.Push(j,__FILE__,__LINE__);

				}

			}

		}

	}

	if (testsToRunIndexes.Size()!=0)//Run selected indexes
	{
		numTests=testsToRunIndexes.Size();

		for(int i=0;i<numTests;i++)
		{

			if (testsToRunIndexes[i]<testListSize)
			{

				printf("\n\nRunning test %s.\n\n",testList[testsToRunIndexes[i]]->GetTestName().C_String());
				returnVal=testList[testsToRunIndexes[i]]->RunTest(noParamsList,isVerbose,disallowTestToPause);
				testList[i]->DestroyPeers();

				if (returnVal==0)
				{passedTests++;}
				else
				{
					printf("Test %s returned with error %s",testList[testsToRunIndexes[i]]->GetTestName().C_String(),testList[testsToRunIndexes[i]]->ErrorCodeToString(returnVal).C_String());

				}
			}
		}
	}

	if (numTests>0)
	{
		printf("\nPassed %i out of %i tests.\n",passedTests,numTests);
	}

	printf("Press enter to continue \n");
	gets(str);
	//Cleanup
	int len=testList.Size();

	for (int i=0;i<len;i++)
	{
		delete testList[i];

	}
	testList.Clear(false,__FILE__,__LINE__);

	return 0;
}

