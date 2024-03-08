Project: Scalable autopatcher server service

Description: Expanded version of AutopatcherServer_PostgreSQL. It will self-scale to load, using Rackspace Cloud to add additional servers when all servers are full. Load balancing is accomplished with the help of CloudServer / CloudClient. DynDNS is used to point to the host of the system.

Dependencies: PostgreSQL version 8.2 (http://www.postgresql.org/download/). Dlls should exist with the .exe. See the post-build process which copies "C:\Program Files\PostgreSQL\8.2\bin\*.dll" .\Release

Related projects: AutopatcherClientRestarter, AutopatcherPostgreSQLRepository, AutopatcherServer, RackspaceConsole

For help and support, please visit http://www.jenkinssoftware.com
