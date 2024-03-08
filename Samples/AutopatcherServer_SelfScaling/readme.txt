Project: Scalable autopatcher server service

Description: Expanded version of AutopatcherServer_PostgreSQL. It will self-scale to load, using Rackspace Cloud to add additional servers when all servers are full. Load balancing is accomplished with the help of CloudServer / CloudClient. DynDNS is used to point to the host of the system.

To start automatically on bootup I used
http://www.coretechnologies.com/products/AlwaysUp/
Putting the exe in the Startup folder requires logon which won't work for unattended servers
See also http://www.coretechnologies.com/products/AlwaysUp/HowToShowTrayIconsInSession0.html

You also need files in the working directory or the server will ask you information on startup.
apiAccessKey.txt : Contents should be your API key in your Rackspace control panel
databasePassword.txt : The PostgreSQL password you set when you installed it
patcherHostDomainURL.txt : This is the domain that DNS will point to. In the Rackspace control panel, click DNS, click create domain, and enter this value. For example, mygamepatcher.com
patcherHostSubdomainURL.txt : This is the record in the DNS entry. Click Add Record to add it. For example, version1.mygame.com
rackspaceAuthenticationURL.txt: https://identity.api.rackspacecloud.com/v2.0
rackspaceCloudUsername.txt: Your username with Rackspace

Dependencies:
PostgreSQL version 8.2 (http://www.postgresql.org/download/). Dlls should exist with the .exe. See the post-build process which copies "C:\Program Files\PostgreSQL\8.2\bin\*.dll" .\Release
If you want to use xdelta3 to generate patches, this is assumed to be located at c:\xdelta3-3.0.6-win32.exe

Related projects: AutopatcherClientRestarter, AutopatcherPostgreSQLRepository, AutopatcherServer, RackspaceConsole

For help and support, please visit http://www.jenkinssoftware.com
