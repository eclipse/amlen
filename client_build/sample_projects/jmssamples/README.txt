Eclipse Amlen JMS Client V5.0
Jan 2019

Contents
1. Description
2. Eclipse Amlen JMS Client package content
3. Working with sample applications
4. Importing the JMS client into Eclipse
5. Limitations and known problems


DESCRIPTION
===========
This README file for the Eclipse Amlen JMS Client package contains
information about content, updates, fixes, limitations, and known problems.

For more details about Eclipse Amlen, see the product documentation in the 
IBM Knowledge Center: 
http://ibm.biz/iotms_v20

NEW IN THIS RELEASE
===================
There are no new Eclipse Amlen JMS Client features in this release. 

Eclipse Amlen JMS CLIENT CONTENT
===================================
The ImaJmsClientV5.0.zip package provides the Eclipse Amlen client 
implementation of the Java™ Messaging Service (JMS) and the Eclipse Amlen
Resource Adapter. It also provides documentation for the classes that are used 
to create and configure JMS administered objects for use with Eclipse Amlen.
Finally, it includes Source Components and Sample Materials.

Directories and files:
    ImaClient/
        README.txt - this file 
        
        license/ - contains the Eclipse Amlen JMS Client and the IBM 
            WIoTP Message Gateway Resource Adapter license agreement files 
                
        jms/
            version.txt - contains version information for the Eclipse Amlen
                JMS Client
                
            .classpath and .project - Eclipse project configuration files
            	that permit the jms sub-directory to be imported as an
            	Eclipse project
            	
            doc/ - contains documentation for creating and configuring JMS 
                administered objects for Eclipse Amlen
            
            samples/ - contains sample applications (Source Components)
                demonstrating how to create administered objects, how to
                send and receive messages via Eclipse Amlen and how to 
                write client applications that are enabled for high 
                availability configurations
                    
            lib/
                imaclientjms.jar - the Eclipse Amlen implementation of the 
                    JMS interface
                jms.jar - the JAR file for the JMS 1.1 interface
                jmssamples.jar - the compiled classes (Sample Materials) for 
                    the sample code (Source Components) provided with the
                    Eclipse Amlen JMS Client
                fscontext.jar and providerutil.jar - JAR files which implement 
                    a file system JNDI provider
 
         ImaResourceAdapter/
            version.txt - contains version information for the Eclipse Amlen
                Resource Adapter
                
            ima.jmsra.rar - The Eclipse Amlen Resource Adapter archive
                   
WORKING WITH SAMPLE APPLICATIONS
================================
There are four sample applications provided with the client.  These sample 
applications are:
    JMSSampleAdmin
    JMSSample
    HATopicPublisher
    HADurableSubscriber

The JMSSampleAdmin application demonstrates how to create, configure and store 
JMS administered objects for Eclipse Amlen. This application reads 
Eclipse Amlen JMS administered object configurations from an input
file and populates a JNDI repository. This application can populate either LDAP
or file system JNDI repositories.  

The JMSSample application demonstrates how to send messages to and receive 
messages from Eclipse Amlen topics and queues. This application is 
implemented in three classes, JMSSample, JMSSampleSend and JMSSampleReceive. The 
JMSSample application can either retrieve administered objects from a JNDI 
repository or it can create and configure the required administered objects at 
runtime.

HATopicPublisher and HADurableSubscriber demonstrate how to enable JMS client
applications to use Eclipse Amlen high availability features.

To run the sample applications that are provided with the client, three JAR 
files are required in the class path; imaclientjms.jar, jms.jar, and 
jmssamples.jar.  All three files are located in ImaClient/jms/lib. In order 
to use the file system JNDI provider when running the JMSSampleAdmin and 
JMSSample applications, two additional JAR files are required in the class 
path; fscontext.jar and providerutil.jar. Both of these JAR files are also 
available in ImaClient/jms/lib.

Running the compiled samples on Linux:
======================================
Running from ImaClient/jms, set the CLASSPATH environment variable in the 
following way:
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar
    
Or, if you want to use the file system JNDI provider, set CLASSPATH to:
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar:lib/fscontext.jar:lib/providerutil.jar

Running from ImaClient/jms, the following example command lines run each 
application and provide a usage statement for each.

    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSample
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HADurableSubscriber

Running the compiled samples on Windows:
========================================
Running from ImaClient/jms, set the CLASSPATH environment variable in the 
following way:
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar
    
Or, if you want to use the file system JNDI provider, set CLASSPATH to:
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar;lib\fscontext.jar;lib\providerutil.jar

Running from ImaClient/jms, the following example command lines run each 
application and provide a usage statement for each.

    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSample
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HADurableSubscriber
    
To build the sample applications locally, only jms.jar and imaclientjms.jar are 
required in the build class path.


IMPORTING THE JMS CLIENT INTO ECLIPSE
=====================================
To import the Eclipse Amlen JMS Client into Eclipse, complete the following 
steps:
1. Unzip the contents of ImaJmsClientV2.0.zip 
2. In Eclipse, select File->Import->General->Existing Projects into Workspace
3. Click Next
4. Choose the "Select root directory" radio button 
5. Click Browse 
6. Navigate to and select the jms sub-directory in the unpacked zip file content
7. If the check box for "Copy projects into workspace" is selected, remove the 
   check
8. Click Finish

After you complete these steps, you can compile and run the sample applications 
and your own client applications from Eclipse.


LIMITATIONS AND KNOWN PROBLEMS
==============================
1. The Eclipse Amlen JMS implementation delivers messages in the order in  
which they are received.  Message priority settings have no affect on delivery 
order.

2. The Eclipse Amlen Resource Adapter is supported on WebSphere 
Application Server version 8.0 or later on all platforms except z/OS®.

NOTICES
=======
1. IBM, the IBM logo, ibm.com, and Watson IoT Platform - Message Gateway are trademarks
jurisdictions worldwide. Other product and service names might be trademarks of
IBM or other companies. A current list of IBM trademarks is available on the 
Web at "Copyright and trademark information" at 
www.ibm.com/legal/copytrade.shtml. 

2. Java and all Java-based trademarks and logos are trademarks or registered 
trademarks of Oracle and/or its affiliates. 
	
