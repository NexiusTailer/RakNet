#include "AWSSimpleDBInterface.h"

void AWSSimpleDBInterface::CreateDomain(const char *domainName) {}
void AWSSimpleDBInterface::BatchPutAttributes( const char *domainName, DataStructures::Table *table ) {}
void AWSSimpleDBInterface::DeleteAttributes( const char *domainName, const char *itemName, const DataStructures::List<RakNet::RakString> &columns, const DataStructures::List<RakNet::RakString> &values ) {}
void AWSSimpleDBInterface::DeleteDomain(const char *domainName) {}
void AWSSimpleDBInterface::DomainMetadata(const char *domainName, RakNet::RakString &itemCount, RakNet::RakString &columnCount, RakNet::RakString &rowCount) {}
void AWSSimpleDBInterface::GetAttributes(const char *domainName) {}
void AWSSimpleDBInterface::ListDomains(const char *domainName) {}
void AWSSimpleDBInterface::Select(const char *domainName) {}