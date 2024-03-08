#include "DebugTools.h"

DebugTools::DebugTools(void)
{
}

DebugTools::~DebugTools(void)
{
}

void DebugTools::ShowError(RakNet::RakString errorString,bool pause, unsigned int lineNum,const char *fileName)
{

	char pauseStr[512];
    
	printf("%s\nFile:%s \nLine: %i\n",errorString.C_String(),fileName,lineNum);

	if (pause)
	{
		printf("Press enter to continue \n");
		gets(pauseStr);
	}
}
