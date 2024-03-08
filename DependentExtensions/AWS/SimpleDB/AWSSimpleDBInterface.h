#ifndef AWS_SIMPLE_DB_INTERFACE_H
#define AWS_SIMPLE_DB_INTERFACE_H

#include "DS_Table.h"

class AWSSimpleDBInterface
{
public:
	virtual void CreateDomain(const char *domainName);
	// For table cells, only string and numeric are supported
	// Due to the implementation details, a column must exist in the table named exactly "ItemName" without the quotes.
	virtual void BatchPutAttributes( const char *domainName, DataStructures::Table *table );
	// columns and values must be the same size and are treated as a pair
	// Operations finds rorws for a given itemName, finds the column/value pair that matches, and deletes it (I think)
	// columns and values are optional - if left out, all rows for that item is deleted
	virtual void DeleteAttributes( const char *domainName,
		const char *itemName,
		const DataStructures::List<RakNet::RakString> &columns,
		const DataStructures::List<RakNet::RakString> &values );
	virtual void DeleteDomain(const char *domainName);
	// Timestamp 	The data and time when metadata was calculated in Epoch (UNIX) time.
	// ItemCount 	The number of all items in the domain.
	// AttributeValueCount 	The number of all attribute name/value pairs in the domain.
	// AttributeNameCount 	The number of unique attribute names in the domain.
	// ItemNamesSizeBytes 	The total size of all item names in the domain, in bytes.
	// AttributeValuesSizeBytes 	The total size of all attribute values, in bytes.
	// AttributeNamesSizeBytes 	The total size of all unique attribute names, in bytes. 
	virtual void DomainMetadata(const char *domainName, RakNet::RakString &itemCount, RakNet::RakString &columnCount, RakNet::RakString &rowCount);
	virtual void GetAttributes(const char *domainName);
	virtual void ListDomains(const char *domainName);

	// See http://docs.amazonwebservices.com/AmazonSimpleDB/latest/DeveloperGuide/UsingSelect.html
	virtual void Select(const char *domainName);
};

#endif
