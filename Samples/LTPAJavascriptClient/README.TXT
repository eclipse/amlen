The README is for running end-to-end LTPA test using CustomLoginModule. 

1. setup Worklight 6.0 on IBM WebSphere Application Server Version 8.5.5.0 Liberty Profile
   a. Install Liberty Profile. The default install location is:
	 /opt/IBM/WebSphere/Liberty
   b. create a new HTTP server using server command then start it:
	/opt/IBM/WebSphere/Liberty/bin/server create Worklightserver
	/opt/IBM/WebSphere/Liberty/bin/server start Worklightserver
   c. install Worklight 6.0 server.  The default isntall location is:
	/opt/IBM/Worklight/WorklightServer
      choose Worklightserver that is just created as the HTTP server and derby as the Database.
   d. stop Worklightserver and run database configuration as the following:
	ant -f /opt/IBM/Worklight/WorklightServer/configuration-samples/configure-liberty-derby.xml database
      It will create WRKLIGHT and WRKREPORT tables which are needed by all the worklight appliactions unless specified inside the application.
   e. login http://serverip:9080/appcenterconsole to make sure the server is ready. The default user/password is admin/admin


2. configure Worklightserver to use external LDAP.
    a. backup Worklightserver server.xml. The file is in:
	 /opt/IBM/WebSphere/Liberty/usr/servers/Worklightserver/
    b. add the following ldapRegistry right after httpEndpoint:
    <ldapRegistry id="ldap" baseDN="ou=SVT,o=IBM,c=US" host="9.3.179.32"
        ldapType="Custom" port="389" realm="AppCenterLdap"
        recursiveSearch="true">
    <idsFilters
     groupFilter="(&amp;(cn=%v)(objectclass=groupOfNames))"
     userFilter="(&amp;(uid=%v)(objectclass=inetOrgPerson))"
     groupMemberIdMap="*:member"
     userIdMap="*:uid"/>
    </ldapRegistry>

    c. Choose an existing LDAP group for "appcenteradmin" and "appcenterconsole". Edit jnidEntry for ldap configuration. Test the setting by login appconsole using the user specified inside the group. Check the Worklight infocenter for detail info.

    d. Edit bootstrap.properties, add ldap configurations.


3. import and build CustomLoginModule project. The project contains 3 pieces, all can be found under ${workspace}/module_23_1_CustomLoginModule/bin/ after build: 
	server side war file:  module_23_1_CustomLoginModule.war
	worklight adapter:     DummyAdapter.adapter
	worklight appliaction: AuthDemo-all.wlapp or wlapp files target a specific platform.	
    a. from worklight 6.0 studio, import archived project CustomLoginModuleProject_0926.zip. 

    b. build AuthDemo app using Build For Remote Server ...


4. configure IBM MessageSight Appliance.
    a. copy the ltpa.keys from Liberty Profile. The file can be found under:
	/opt/IBM/WebSphere/Liberty/usr/servers/Worklightserver/resources/security
    b. create IMA LTPAProfile based on ltpa.keys file. If using the defaul Liberty Profile ltpa.keys, the password is WebAS

    c. Enable a securityProfile using the LTPAProfile. Enable an Endpoint using the securityProfile and SSL.

5. import CustomLoginModule into Worklightserver.
    a. install CustomLoginModule war file. modify CustomLoginModule_install.xml make sure the pathes are correct.
 
    b. modify module_23_1_CustomLoginModule/apps/AuthDemo/common/js/messageUtils.js, make sure the following var are correct:
	var SERVER = "IMA ipaddress";
	var PORT = 17001;
	
    c. run ant install script.
	 ant -f CustomLoginModule_install.xml install
       it will add CustomLoginModule to Worklightserver

    d. login IBM Worklight Console
	http://worklightserverip:9080/worklight/console/#catalog
       select "ChooseFile" for Deploy application or adapter, choose DummyAdapter.adapter and Submit

    e. repeat the step c for .wlapp files.


6. test the CustomLoginModule.
    a. Make sure Liberty, Worklight server, LDAP and IMA are all configured correctly.

    b. get IMA SSL certificate by accessing:
	https://ima_appliance_ip:port/mqtt
       * This step should be addressed in the real world by attaching the IMA certificate into worklight server http server. In this test, it will be a onetime manual step.

    c. run the sample from:
	http://Worklightserver_ip:9080/worklight/apps/services/www/AuthDemo/desktopbrowser/default/AuthDemo.html
