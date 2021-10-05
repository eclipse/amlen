Eclipse Amlen Protocol Plug-in SDK V2.0
June 2016

Contents
1. Description
2. Eclipse Amlen Protocol Plug-in SDK content
3. Working with sample JSON protocol plug-in sample code
4. Importing the sample JSON protocol plug-in SDK and samples into Eclipse
5. Working with sample REST protocol plug-in sample code
6. Importing the sample REST protocol plug-in SDK and samples into Eclipse
7. Limitations and known problems


DESCRIPTION
===========
This README file for the Eclipse Amlen Protocol Plug-in SDK contains
information about content, updates, fixes, limitations, and known problems.
The SDK enables you to extend the set of protocols natively supported by the Eclipse Amlen
server by writing plug-ins in the Java programming language.

For more details about Eclipse Amlen, see the product documentation in the 
IBM Knowledge Center: 
http://ibm.biz/iotms_v20

NEW IN THIS RELEASE
===================
The Eclipse Amlen Protocol Plug-in SDK is now fully supported in a non-HA environment.
Please refer to the accompanying license for usage restrictions.
Plug-in deployment and configuration is now done through a RESTful API

Eclipse Amlen PROTOCOL PLUG-IN SDK CONTENT
=================================================
The ImaTools/ImaPlugin sub-directory in the ProtocolPluginV2.0.zip 
package provides the Eclipse Amlen Protocol Plug-in SDK. It also provides
documentation for the classes that are used to create protocol plug-ins for use 
with Eclipse Amlen. Finally, it includes Source Components and Sample 
Materials.

Directories and files:
    ImaTools/
        license/ - contains the license agreement files for the Eclipse Amlen
            protocol plug-in feature 
        ImaPlugin/
            README.txt - this file
        
            version.txt - contains version information for the Eclipse Amlen
                Protocol Plug-in SDK
            
            doc/ - contains documentation for creating protocol plug-ins 
                using the Eclipse Amlen Protocol Plug-in SDK
             
            lib/
                imaPlugin.jar - the Eclipse Amlen Protocol Plug-in SDK
            
                jsonplugin.zip - a zip archive containing a sample JSON protocol
                    plug-in that can be deployed in Eclipse Amlen.  This  
                    archive contains the compiled classes (Sample Materials) for
                    the sample plug-in code (Source Components) provided in the 
                    samples/jsonmsgPlugin sub-directory.
                
                jsonprotocol.jar - the compiled classes (Sample Materials) for
                    the sample plug-in. This jar file is also in the 
                    jsonplugin.zip file.
                
                restplugin.zip - a zip archive containing a sample REST protocol
                    plug-in that can be deployed in Eclipse Amlen.  This  
                    archive contains the compiled classes (Sample Materials) for
                    the sample plug-in code (Source Components) provided in the 
                    samples/restmsgPlugin sub-directory.
                
                restprotocol.jar - the compiled classes (Sample Materials) for
                    the sample REST protocol plug-in. This jar file is also in 
                    the restplugin.zip file.
                
            samples/
                jsonmsgPlugin/
                    .classpath and .project - Eclipse project configuration 
            	        files that permit the jsonmsgPlugin sub-directory to be  
            	        imported as an Eclipse project
            	
            	    build_plugin_zip.xml - An Ant build file you can use to 
            	        create a plug-in archive
            	    
                    src/ - contains Java source code for a sample plug-in  
                        (Source Components) demonstrating how to write a  
                        protocol plug-in for use with Eclipse Amlen
                
                    config/ - contains the JSON descriptor file (Source 
                        Components) required to deploy the compiled sample  
                        plug-in in Eclipse Amlen
                
                    doc/ - contains documentation for the sample plug-in source 
                        code

                jsonmsgClient/
                    .classpath and .project - Eclipse project configuration 
            	        files that permit the jsonmsgClient sub-directory to be  
            	        imported as an Eclipse project
            	    
            	    JSONMsgWebClient.html - the client application (Source 
            	        Components) for the sample plug-in provided in 
            	        jsonmsgPlugin. This client depends on the JavaScript�  
            	        library in the js sub-directory.
            	                
                    js/ - contains the sample JavaScript client library (Source
                        Components) for use with the sample plug-in provided in
                        jsonmsgPlugin.
                    
                    css/ - contains style sheets (Source Components) and image  
                        files used by the sample client application and the  
                        sample client library.
                
                    doc/ - contains documentation for the sample JavaScript 
                        client library.
                    
                restmsgPlugin/
                    .classpath and .project - Eclipse project configuration 
                        files that permit the restmsgPlugin sub-directory to be  
                        imported as an Eclipse project
                
                    build_plugin_zip.xml - An Ant build file you can use to 
                        create a plug-in archive
                    
                    src/ - contains Java source code for a sample plug-in  
                        (Source Components) demonstrating how to write a  
                        protocol plug-in for use with Eclipse Amlen
                
                    config/ - contains the JSON descriptor file (Source 
                        Components) required to deploy the compiled sample  
                        plug-in in Eclipse Amlen
                
                    doc/ - contains documentation for the sample plug-in source 
                        code
                       
                restmsgClient/
                    .classpath and .project - Eclipse project configuration 
                        files that permit the restmsgClient sub-directory to be  
                        imported as an Eclipse project
                    
                    RESTMsgWebClient.html - the client application (Source 
                        Components) for the sample plug-in provided in 
                        restmsgPlugin. This client depends on the JavaScript  
                        library in the js sub-directory.
                                
                    js/ - contains the sample JavaScript client library (Source
                        Components) for use with the sample plug-in provided in
                        restmsgPlugin.
                    
                    css/ - contains style sheets (Source Components) and image  
                        files used by the sample client application and the  
                        sample client library.
                
                    doc/ - contains documentation for the sample JavaScript 
                        client library.
                  
WORKING WITH THE SAMPLE JSON PROTOCOL PLUG-IN AND CLIENT APPLICATION
====================================================================
The JSONMsgWebClient.html application uses the jsonmsg.js JavaScript client
library to communicate with the json_msg plug-in. JSONMsgWebClient.html provides
a web interface for sending and receiving QoS 0 (at most once) messages to
json_msg plug-in.

The json_msg protocol plug-in sample is comprised of two classes, JMPlugin and 
JMConnection. JMPlugin implements the ImaPluginListener interface required to 
initialize and launch a protocol plug-in.  JMConnection implements the 
ImaConnectionListener interface which enables clients to connect to Eclipse
Amlen and to send and receive messages via the json_message protocol. 
The JMConnection class receives JSON objects from publishing clients and uses 
the ImaJson utility class to parse these objects into commands and messages 
which it sends to the Eclipse Amlen server.  Similarly, the JMConnection 
class uses the ImaJson utility class to convert Eclipse Amlen messages to 
JSON objects for subscribing json_msg clients. Finally, the json_msg protocol
plug-in includes a descriptor file named plugin.json.  This descriptor file 
is required in the zip archive that is used to deploy a protocol plug-in and 
must always use this file name.  A sample plug-in archive for the json_msg
protocol, jsonplugin.zip, is included in the ImaTools/ImaPlugin/lib   
directory. It contains a jar file (jsonprotocol.jar with the compiled JMPlugin  
and JMConnection classes) and the plugin.json descriptor file.

To run the sample JavaScript client application, you must first deploy the 
sample plug-in to Eclipse Amlen.  The plug-in must be archived in a zip or
jar file.  This file must contain the jar file (or files) that implement the 
plug-in for the target protocol. It must also contain a JSON descriptor file
that describes the plug-in content.  An archive named jsonplugin.zip is
available in ImaTools/ImaPlugin/lib and contains the json_msg sample 
plug-in. Once the sample plug-in archive is deployed, you can run the sample 
client application by loading JSONMsgWebClient.html (in 
ImaTools/ImaPlugin/samples/jsonmsgClient) into a web browser. You must  
maintain the sub-directory structure where JSONMsgWebClient.html is found in  
order to load the sample JavaScript client library and style sheets for the  
sample application.


Deploying the sample plug-in archive in Eclipse Amlen:
=============================================================
To deploy the jsonplugin.zip to Eclipse Amlen, you must use the 
PUT method to import a plug-in zip file jsonmsg.zip by using cURL: 
	curl -X PUT -T jsonmsg.zip http://127.0.0.1:9089/ima/v1/file/jsonmsg.zip

Next, you must create the plug-in by using the POST method to create a 
protocol plug-in called jsonmsg by using cURL:
    curl -X POST \
       -H 'Content-Type: application/json'  \
       -d  '{ 
               "Plugin": {
                "jsonmsg": {
                 "File": "jsonmsg.zip"
                }
             }
           }
     '  \
    http://127.0.0.1:9089/ima/v1/configuration/

Finally, you must restart Eclipse Amlen Protocol Plug-in server in 
order to launch the plug-in by using cURL:
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

To confirm the plug-in has been successfully deployed by using cURL:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

You should see json_msg in the resulting list.  This value corresponds to the
Name property that is specified in the plugin.json descriptor file.  

To confirm the new protocol has been successfully registered by using cURL:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol
    
You should see json_msg in the resulting list.  This value corresponds to the 
Protocol property that is specified in the plugin.json descriptor file.

If you are using the DemoEndpoint, no further steps are required to access the 
sample json_msg protocol plug-in.  If you are using a different endpoint, then 
you will need to follow the same procedures to specify which protocols are 
allowed by an endpoint, a connection policy or a messaging policy. The 
DemoEndpoint and its policies are pre-configured to allow all defined protocols.

Running JavaScript sample client application:
=============================================
Open JSONMsgWebClient.html in a web browser. Specify the host name or IP address
for Eclipse Amlen in the Server text box.  If you are using the DemoEndpoint,
you can use port 16102 as listed in the Port text box.  Otherwise, specify the
correct port number for the Eclipse Amlen endpoint that is configured with 
the sample plug-in.  For additional connection options, select the Session 
tab.  When you have finished setting connection properties, click the Connect 
button to connect to Eclipse Amlen.  Then use the Topic tab to send messages 
to and receive messages from Eclipse Amlen destinations via the sample 
plug-in.


IMPORTING THE JSON_MSG CLIENT INTO ECLIPSE
==========================================
To import the Eclipse Amlen json_msg client into Eclipse, complete these
steps:
1. Unzip the contents of ProtocolPluginV2.0.zip
2. In Eclipse, select File->Import->General->Existing Projects into Workspace
3. Click Next
4. Choose the "Select root directory" radio button 
5. Click Browse  
6. Navigate to and select the ImaTools/ImaPlugin/samples/jsonmsgClient 
   sub-directory in the unpacked zip file content
7. If the check box for "Copy projects into workspace" is selected, deselect it
8. Click Finish

After you complete these steps, you can run the sample application from Eclipse.

IMPORTING THE JSON_MSG PLUG-IN INTO ECLIPSE
===========================================
To import the Eclipse Amlen json_msg plug-in into Eclipse, complete these 
steps:
1. Unzip the contents of ProtocolPluginV2.0.zip
2. In Eclipse, select File->Import->General->Existing Projects into Workspace
3. Click Next
4. Choose the "Select root directory" radio button 
5. Click Browse
6. Navigate to and select the ImaTools/ImaPlugin/samples/jsonmsgPlugin 
   sub-directory in the unpacked zip file content
7. If the check box for "Copy projects into workspace" is selected, deselect it
8. Click Finish

After you complete these steps, you can compile the sample plug-in and your own 
plug-ins from Eclipse.

BUILDING THE PLUG-IN ARCHIVE IN ECLIPSE
=======================================
If you update the json_msg sample plug-in and you want to create an archive
to deploy in Eclipse Amlen, follow these steps:
NOTE: These steps assume you have imported the jsonmsgPlugin project into
Eclipse and that Eclipse has already compiled the project source files and 
placed the class files in the bin sub-directory.
1. In Eclipse, open the jsonmsgPlugin in the package explorer or navigator.
2. Right click on build_plugin_zip.xml and select Run As -> Ant build.
   A plugin sub-directory is created under jsonmsgPlugin with a new
   jsonplugin.zip file.  You can refresh the jsonmsgPlugin project to see 
   the plugin sub-directory in the Eclipse UI. 


WORKING WITH THE SAMPLE REST PROTOCOL PLUG-IN AND CLIENT APPLICATION
====================================================================
The RESTMsgWebClient.html application uses the restmsg.js JavaScript client
library to communicate with the restmsg plug-in. RESTMsgWebClient.html provides
a web interface for sending and receiving QoS 0 (at most once) messages to
restmsg plug-in.

The restmsg protocol plug-in sample is comprised of two classes, RestMsgPlugin  
and RestMsgConnection. RestMsgPlugin implements the ImaPluginListener interface  
required to initialize and launch a protocol plug-in.  RestMsgConnection  
implements the ImaConnectionListener interface which enables clients to connect  
to Eclipse Amlen and to send and receive messages via the restmsg protocol.
The RestMsgConnection class receives REST objects from publishing clients and
parses these objects into commands and messages which it sends to  the Eclipse
Amlen server. Similarly, the RestMsgConnection class converts Eclipse
Amlen messages to REST objects for subscribing restmsg clients. Finally, 
the restmsg protocol plug-in includes a descriptor file named plugin.json. This 
descriptor file is required in the zip archive that is used to deploy a protocol 
plug-in and must always use this file name.  A sample plug-in archive for the 
restmsg protocol, restplugin.zip, is included in the 
ImaTools/ImaPlugin/lib directory. It contains a jar file 
(restprotocol.jar with the compiled RestMsgPlugin and RestMsgConnection classes)
and the plugin.json descriptor file.

To run the sample JavaScript client application, you must first deploy the 
sample plug-in in Eclipse Amlen.  The plug-in must be archived in a zip or
jar file.  This file must contain the jar file (or files) that implement the 
plug-in for the target protocol. It must also contain a JSON descriptor file
that describes the plug-in content.  An archive named restplugin.zip is 
available in ImaTools/ImaPlugin/lib and contains the restmsg sample 
plug-in. Once the sample restmsg plug-in archive is deployed, you can run the 
sample client application by loading RESTMsgWebClient.html (in 
ImaTools/ImaPlugin/samples/restmsgClient) into a web browser. You must  
maintain the sub-directory structure where RESTMsgWebClient.html is found in  
order to load the sample JavaScript client library and style sheets for the  
sample application.


Deploying the REST sample plug-in archive in Eclipse Amlen:
==================================================================
To deploy the restplugin.zip to Eclipse Amlen, you must use the 
PUT method to import a plug-in zip file jsonmsg.zip by using cURL:
	curl -X PUT -T restplugin.zip http://127.0.0.1:9089/ima/v1/file/restplugin.zip

Next, you must create the plug-in by using the POST method to create a 
protocol plug-in called jsonmsg by using cURL:
    curl -X POST \
       -H 'Content-Type: application/json'  \
       -d  '{ 
               "Plugin": {
                "restmsg": {
                 "File": "restplugin.zip"
                }
             }
           }
     '  \
    http://127.0.0.1:9089/ima/v1/configuration/
   
Finally, you must stop and restart Eclipse Amlen Protocol Plug-in server in 
order to launch the plug-in.

Finally, you must restart Eclipse Amlen Protocol Plug-in server in 
order to launch the plug-in by using cURL:
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

To confirm the plug-in has been successfully deployed by using cURL:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

You should see restmsg in the resulting list.  This value corresponds to the
Name property that is specified in the plugin.json descriptor file.  

To confirm the new protocol has been successfully registered by using cURL:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol
    
You should see restmsg in the resulting list.  This value corresponds to the 
Protocol property that is specified in the plugin.json descriptor file.

If you are using the DemoEndpoint, no further steps are required to access the 
sample restmsg protocol plug-in.  If you are using a different endpoint, then 
you will need to follow the same procedures to specify which protocols are 
allowed by an endpoint, a connection policy or a messaging policy. The 
DemoEndpoint and its policies are pre-configured to allow all defined protocols.

Running JavaScript sample client application:
=============================================
Open RESTMsgWebClient.html in a web browser. Specify the host name or IP address
for Eclipse Amlen in the Server text box.  If you are using the DemoEndpoint,
you can use port 16102 as listed in the Port text box.  Otherwise, specify the
correct port number for the Eclipse Amlen endpoint that is configured with 
the sample plug-in.  For additional connection options, select the Session 
tab.  When you have finished setting connection properties, click the Connect 
button to connect to Eclipse Amlen.  Then use the Topic tab to send messages 
to and receive messages from Eclipse Amlen destinations via the sample 
plug-in.


IMPORTING THE RESTMSG CLIENT INTO ECLIPSE
==========================================
To import the Eclipse Amlen restmsg client into Eclipse, complete these 
steps:
1. Unzip the contents of ProtocolPluginV2.0.zip
2. In Eclipse, select File->Import->General->Existing Projects into Workspace
3. Click Next
4. Choose the "Select root directory" radio button 
5. Click Browse  
6. Navigate to and select the ImaTools/ImaPlugin/samples/restmsgClient 
   sub-directory in the unpacked zip file content
7. If the check box for "Copy projects into workspace" is selected, deselect it
8. Click Finish

After you complete these steps, you can run the sample application from Eclipse.

IMPORTING THE RESTMSG PLUG-IN INTO ECLIPSE
==========================================
To import the Eclipse Amlen restmsg plug-in into Eclipse, complete these
steps:
1. Unzip the contents of ProtocolPluginV2.0.zip
2. In Eclipse, select File->Import->General->Existing Projects into Workspace
3. Click Next
4. Choose the "Select root directory" radio button 
5. Click Browse
6. Navigate to and select the ImaTools/ImaPlugin/samples/restmsgPlugin 
   sub-directory in the unpacked zip file content
7. If the check box for "Copy projects into workspace" is selected, deselect it
8. Click Finish

After you complete these steps, you can compile the sample plug-in and your own 
plug-ins from Eclipse.

BUILDING THE RESTMSG PLUG-IN ARCHIVE IN ECLIPSE
===============================================
If you update the restmsg sample plug-in and you want to create an archive
to deploy in Eclipse Amlen, follow these steps:
NOTE: These steps assume you have imported the restmsgPlugin project into
Eclipse and that Eclipse has already compiled the project source files and 
placed the class files in the bin sub-directory.
1. In Eclipse, open the restmsgPlugin in the package explorer or navigator.
2. Right click on build_plugin_zip.xml and select Run As -> Ant build.
   A plugin sub-directory is created under restmsgPlugin with a new
   restplugin.zip file.  You can refresh the restmsgPlugin project to see 
   the plugin sub-directory in the Eclipse UI. 
   
MONITORING THE PLUG-IN PROCESS
==============================

The heap size, garbage collection rate, and CPU usage of the plug-in process
can be monitored by subscribing to the $SYS/ResourceStatistics/Plugin topic.   

LIMITATIONS AND KNOWN PROBLEMS
==============================
1. There is no Amlen Web UI support for deploying protocol plug-ins. This
   must be done from the command line.
2. Shared subscriptions are not supported for protocol plug-ins.
3. The ImaPluginListener.onAuthenticate() method is not invoked by Eclipse Amlen.
4. High Availability configurations are not supported.


NOTICES
=======
1. IBM, the IBM logo, ibm.com  are trademarks or registered trademarks of 
International Business Machines Corp., registered in many jurisdictions worldwide. Other 
product and service names might be trademarks of IBM or other companies. A current list 
of IBM trademarks is available on the Web at "Copyright and trademark information" at 
www.ibm.com/legal/copytrade.shtml. 

2. Java and all Java-based trademarks and logos are trademarks or registered 
trademarks of Oracle and/or its affiliates. 
