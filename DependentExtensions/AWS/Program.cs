/*******************************************************************************
* Copyright 2009 Amazon.com, Inc. or its affiliates. All Rights Reserved.
* 
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
* 
* http://aws.amazon.com/apache2.0/
* 
* or in the "license" file accompanying this file. This file is
* distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied. See the License for the specific
* language governing permissions and limitations under the License.
*******************************************************************************/

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

namespace AmazonSimpleDB_Sample
{
    class Program
    {
        public static void Main(string[] args)
        {
            // If it crashes on this very first line, make sure that DependentExtensions\AWS\bin\x86\Debug\RakNet.dll got copied from DependentExtensions\AWS\Debug\RakNet.dll in the post-build step
            AWSSimpleDBImpl impl = new AWSSimpleDBImpl();
            RakNet.RakNet.main(impl);

            /*
            NameValueCollection appConfig = ConfigurationManager.AppSettings;

            AmazonSimpleDB sdb = AWSClientFactory.CreateAmazonSimpleDBClient(
                appConfig["AWSAccessKey"],
                appConfig["AWSSecretKey"]
                );

            try
            {
                Console.WriteLine("===========================================");
                Console.WriteLine("Getting Started with Amazon SimpleDB");
                Console.WriteLine("===========================================\n");

                // Creating a domain
                Console.WriteLine("Creating domain called MyStore.\n");
                String domainName = "MyStore";
                CreateDomainRequest createDomain = (new CreateDomainRequest()).WithDomainName(domainName);
                sdb.CreateDomain(createDomain);

                // Listing domains
                ListDomainsResponse sdbListDomainsResponse = sdb.ListDomains(new ListDomainsRequest());
                if (sdbListDomainsResponse.IsSetListDomainsResult())
                {
                    ListDomainsResult listDomainsResult = sdbListDomainsResponse.ListDomainsResult;
                    Console.WriteLine("List of domains:\n");
                    foreach (String domain in listDomainsResult.DomainName)
                    {
                        Console.WriteLine("  " + domain);
                    }
                }
                Console.WriteLine();

                // Putting data into a domain
                Console.WriteLine("Putting data into MyStore domain.\n");
                String itemNameOne = "Item_01";
                PutAttributesRequest putAttributesActionOne = new PutAttributesRequest().WithDomainName(domainName).WithItemName(itemNameOne);
                List<ReplaceableAttribute> attributesOne = putAttributesActionOne.Attribute;
                attributesOne.Add(new ReplaceableAttribute().WithName("Category").WithValue("Clothes"));
                attributesOne.Add(new ReplaceableAttribute().WithName("Subcategory").WithValue("Sweater"));
                attributesOne.Add(new ReplaceableAttribute().WithName("Name").WithValue("Cathair Sweater"));
                attributesOne.Add(new ReplaceableAttribute().WithName("Color").WithValue("Siamese"));
                attributesOne.Add(new ReplaceableAttribute().WithName("Size").WithValue("Small"));
                attributesOne.Add(new ReplaceableAttribute().WithName("Size").WithValue("Medium"));
                attributesOne.Add(new ReplaceableAttribute().WithName("Size").WithValue("Large"));
                sdb.PutAttributes(putAttributesActionOne);

                String itemNameTwo = "Item_02";
                PutAttributesRequest putAttributesActionTwo = new PutAttributesRequest().WithDomainName(domainName).WithItemName(itemNameTwo);
                List<ReplaceableAttribute> attributesTwo = putAttributesActionTwo.Attribute;
                attributesTwo.Add(new ReplaceableAttribute().WithName("Category").WithValue("Clothes"));
                attributesTwo.Add(new ReplaceableAttribute().WithName("Subcategory").WithValue("Pants"));
                attributesTwo.Add(new ReplaceableAttribute().WithName("Name").WithValue("Designer Jeans"));
                attributesTwo.Add(new ReplaceableAttribute().WithName("Color").WithValue("Paisley Acid Wash"));
                attributesTwo.Add(new ReplaceableAttribute().WithName("Size").WithValue("30x32"));
                attributesTwo.Add(new ReplaceableAttribute().WithName("Size").WithValue("32x32"));
                attributesTwo.Add(new ReplaceableAttribute().WithName("Size").WithValue("32x34"));
                sdb.PutAttributes(putAttributesActionTwo);

                String itemNameThree = "Item_03";
                PutAttributesRequest putAttributesActionThree = new PutAttributesRequest().WithDomainName(domainName).WithItemName(itemNameThree);
                List<ReplaceableAttribute> attributesThree = putAttributesActionThree.Attribute;
                attributesThree.Add(new ReplaceableAttribute().WithName("Category").WithValue("Clothes"));
                attributesThree.Add(new ReplaceableAttribute().WithName("Subcategory").WithValue("Pants"));
                attributesThree.Add(new ReplaceableAttribute().WithName("Name").WithValue("Sweatpants"));
                attributesThree.Add(new ReplaceableAttribute().WithName("Color").WithValue("Blue"));
                attributesThree.Add(new ReplaceableAttribute().WithName("Color").WithValue("Yellow"));
                attributesThree.Add(new ReplaceableAttribute().WithName("Color").WithValue("Pink"));
                attributesThree.Add(new ReplaceableAttribute().WithName("Size").WithValue("Large"));
                attributesThree.Add(new ReplaceableAttribute().WithName("Year").WithValue("2006"));
                attributesThree.Add(new ReplaceableAttribute().WithName("Year").WithValue("2007"));
                
                sdb.PutAttributes(putAttributesActionThree);

                String itemNameFour = "Item_04";
                PutAttributesRequest putAttributesActionFour = new PutAttributesRequest().WithDomainName(domainName).WithItemName(itemNameFour);
                List<ReplaceableAttribute> attributesFour = putAttributesActionFour.Attribute;
                attributesFour.Add(new ReplaceableAttribute().WithName("Category").WithValue("Car Parts"));
                attributesFour.Add(new ReplaceableAttribute().WithName("Subcategory").WithValue("Engine"));
                attributesFour.Add(new ReplaceableAttribute().WithName("Name").WithValue("Turbos"));
                attributesFour.Add(new ReplaceableAttribute().WithName("Make").WithValue("Audi"));
                attributesFour.Add(new ReplaceableAttribute().WithName("Model").WithValue("S4"));
                attributesFour.Add(new ReplaceableAttribute().WithName("Year").WithValue("2000"));
                attributesFour.Add(new ReplaceableAttribute().WithName("Year").WithValue("2001"));
                attributesFour.Add(new ReplaceableAttribute().WithName("Year").WithValue("2002"));
                sdb.PutAttributes(putAttributesActionFour);

                String itemNameFive = "Item_05";
                PutAttributesRequest putAttributesActionFive = new PutAttributesRequest().WithDomainName(domainName).WithItemName(itemNameFive);
                List<ReplaceableAttribute> attributesFive = putAttributesActionFive.Attribute;
                attributesFive.Add(new ReplaceableAttribute().WithName("Category").WithValue("Car Parts"));
                attributesFive.Add(new ReplaceableAttribute().WithName("Subcategory").WithValue("Emissions"));
                attributesFive.Add(new ReplaceableAttribute().WithName("Name").WithValue("O2 Sensor"));
                attributesFive.Add(new ReplaceableAttribute().WithName("Make").WithValue("Audi"));
                attributesFive.Add(new ReplaceableAttribute().WithName("Model").WithValue("S4"));
                attributesFive.Add(new ReplaceableAttribute().WithName("Year").WithValue("2000"));
                attributesFive.Add(new ReplaceableAttribute().WithName("Year").WithValue("2001"));
                attributesFive.Add(new ReplaceableAttribute().WithName("Year").WithValue("2002"));
                sdb.PutAttributes(putAttributesActionFive);

                // Getting data from a domain
                Console.WriteLine("Print attributes with the attribute Category that contain the value Clothes.\n");
                String selectExpression = "Select * From MyStore Where Category = 'Clothes'";
                SelectRequest selectRequestAction = new SelectRequest().WithSelectExpression(selectExpression);
                SelectResponse selectResponse = sdb.Select(selectRequestAction);

                if (selectResponse.IsSetSelectResult())
                {
                    SelectResult selectResult = selectResponse.SelectResult;
                    foreach (Item item in selectResult.Item)
                    {
                        Console.WriteLine("  Item");
                        if (item.IsSetName())
                        {
                            Console.WriteLine("    Name: {0}", item.Name);
                        }
                        foreach (Amazon.SimpleDB.Model.Attribute attribute in item.Attribute)
                        {
                            Console.WriteLine("      Attribute");
                            if (attribute.IsSetName())
                            {
                                Console.WriteLine("        Name: {0}", attribute.Name);
                            }
                            if (attribute.IsSetValue())
                            {
                                Console.WriteLine("        Value: {0}", attribute.Value);
                            }
                        }
                    }
                }
                Console.WriteLine();

                // Deleting values from an attribute
                Console.WriteLine("Deleting Blue attributes in Item_O3.\n");
                Amazon.SimpleDB.Model.Attribute deleteValueAttribute = new Amazon.SimpleDB.Model.Attribute().WithName("Color").WithValue("Blue");
                DeleteAttributesRequest deleteValueAction = new DeleteAttributesRequest().WithDomainName("MyStore").WithItemName("Item_03").WithAttribute(deleteValueAttribute);
                sdb.DeleteAttributes(deleteValueAction);

                //Deleting an attribute
                Console.WriteLine("Deleting attribute Year in Item_O3.\n");
                Amazon.SimpleDB.Model.Attribute deleteAttribute = new Amazon.SimpleDB.Model.Attribute().WithName("Year");
                DeleteAttributesRequest deleteAttributeAction = new DeleteAttributesRequest().WithDomainName("MyStore").WithItemName("Item_03").WithAttribute(deleteAttribute);
                sdb.DeleteAttributes(deleteAttributeAction);

                //Replacing an attribute
                Console.WriteLine("Replace Size of Item_03 with Medium.\n");
                ReplaceableAttribute replaceableAttribute = new ReplaceableAttribute().WithName("Size").WithValue("Medium").WithReplace(true);
                PutAttributesRequest replaceAction = new PutAttributesRequest().WithDomainName("MyStore").WithItemName("Item_03").WithAttribute(replaceableAttribute);
                sdb.PutAttributes(replaceAction);

                //Deleting an item
                Console.WriteLine("Deleting Item_03 item.\n");
                DeleteAttributesRequest deleteItemAction = new DeleteAttributesRequest().WithDomainName("MyStore").WithItemName("Item_03");
                sdb.DeleteAttributes(deleteAttributeAction);

                //Deleting a domain
                Console.WriteLine("Deleting MyStore domain.\n");
                DeleteDomainRequest deleteDomainAction = new DeleteDomainRequest().WithDomainName("MyStore");
                sdb.DeleteDomain(deleteDomainAction);

            }
            catch (AmazonSimpleDBException ex)
            {
                Console.WriteLine("Caught Exception: " + ex.Message);
                Console.WriteLine("Response Status Code: " + ex.StatusCode);
                Console.WriteLine("Error Code: " + ex.ErrorCode);
                Console.WriteLine("Error Type: " + ex.ErrorType);
                Console.WriteLine("Request ID: " + ex.RequestId);
                Console.WriteLine("XML: " + ex.XML);
            }
             
             */

            Console.WriteLine("Press Enter to continue...");
            Console.Read();
        }
    }
}