using System;
using System.Collections.Specialized;
using System.Configuration;
using System.Linq;
using System.Xml.Serialization;
using System.Collections.Generic;

using Amazon;
using Amazon.SimpleDB;
using Amazon.SimpleDB.Model;

using RakNet;

public class AWSSimpleDBImpl : AWSSimpleDBInterface
{
    public AWSSimpleDBImpl()
    {
        try
        {
            NameValueCollection appConfig = ConfigurationManager.AppSettings;
            sdb = AWSClientFactory.CreateAmazonSimpleDBClient(
                appConfig["AWSAccessKey"],
                appConfig["AWSSecretKey"]
                );
        }
        catch (AmazonSimpleDBException ex)
        {
            PrintException(ex);
        }
    }
    override public void CreateDomain(string domainName)
    {
        try
        {
            CreateDomainRequest createDomain = (new CreateDomainRequest()).WithDomainName(domainName);
            sdb.CreateDomain(createDomain);
        }
        catch (AmazonSimpleDBException ex)
        {
            PrintException(ex);
        }
    }
    override public void BatchPutAttributes(string domainName, Table table)
    {
        try
        {
            if (table.GetRowCount()==0)
                return;
            BatchPutAttributesRequest batchPutAttributesRequest = (new BatchPutAttributesRequest()).WithDomainName(domainName);
            uint i,j;
            for (i=0; i < table.GetRowCount(); i++)
            {
                string outputStr;
                ReplaceableItem replacableItem = new ReplaceableItem();
                for (j=0; j < table.GetColumnCount(); j++)
                {
                    if (table.ColumnName(j)=="ItemName")
                    {
                        table.GetCellValueByIndex(i, j, out outputStr);
                        replacableItem.ItemName = outputStr;
                    }
                    else
                    {
                        ReplaceableAttribute replacableAttribute = new ReplaceableAttribute();
                        replacableAttribute.Name = table.ColumnName(j);
                        Table.ColumnType ct = table.GetColumnType(j);
                        if (ct == Table.ColumnType.STRING)
                        {
                            table.GetCellValueByIndex(i, j, out outputStr);
                        }
                        else if (ct == Table.ColumnType.NUMERIC)
                        {
                            int outputInt;
                            table.GetCellValueByIndex(i, j, out outputInt);
                            outputStr = outputInt.ToString();
                        }
                        else
                            continue;
                        replacableAttribute.Value = outputStr;
                        replacableItem.Attribute.Add(replacableAttribute);
                    }
                }
                batchPutAttributesRequest.Item.Add(replacableItem);
            }
            
            sdb.BatchPutAttributes(batchPutAttributesRequest);
        }
        catch (AmazonSimpleDBException ex)
        {
            PrintException(ex);
        }
    }
    override public void DeleteAttributes(string domainName, string itemName, RakNetListRakString columns, RakNetListRakString values)
    {
        try
        {
            DeleteAttributesRequest deleteAttributesRequest = (new DeleteAttributesRequest()).WithDomainName(domainName).WithItemName(itemName);
            uint i;
            for (i = 0; i < columns.Size(); i++)
                deleteAttributesRequest.Attribute.Add(new Amazon.SimpleDB.Model.Attribute().WithName(columns.Get(i).C_String()).WithValue(values.Get(i).C_String()));
            sdb.DeleteAttributes(deleteAttributesRequest);
        }
        catch (AmazonSimpleDBException ex)
        {
            PrintException(ex);
        }
    }
    override public void DeleteDomain(string domainName)
    {
        try
        {
            DeleteDomainRequest ddr = (new DeleteDomainRequest()).WithDomainName(domainName);
            sdb.DeleteDomain(ddr);
        }
        catch (AmazonSimpleDBException ex)
        {
            PrintException(ex);
        }
    }
    override public void DomainMetadata(string domainName, RakString itemCount, RakString columnCount, RakString rowCount)
    {
        try
        {
            DomainMetadataResponse dmr = sdb.DomainMetadata((new DomainMetadataRequest()).WithDomainName(domainName));
            itemCount = dmr.DomainMetadataResult.ItemCount;
            columnCount = dmr.DomainMetadataResult.AttributeNameCount;
            rowCount = dmr.DomainMetadataResult.AttributeValueCount;
        }
        catch (AmazonSimpleDBException ex)
        {
            PrintException(ex);
        }
    }
    public void PrintException(AmazonSimpleDBException ex)
    {
        Console.WriteLine("Caught Exception: " + ex.Message);
        Console.WriteLine("Response Status Code: " + ex.StatusCode);
        Console.WriteLine("Error Code: " + ex.ErrorCode);
        Console.WriteLine("Error Type: " + ex.ErrorType);
        Console.WriteLine("Request ID: " + ex.RequestId);
        Console.WriteLine("XML: " + ex.XML);
    }

    AmazonSimpleDB sdb;
}
