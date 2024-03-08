#include "AWSSimpleDBInterface.h"

void main(AWSSimpleDBInterface *simpleDb)
{
	simpleDb->CreateDomain("called from main");
}
