IBM® MessageSight™ JMS Client V1.0
April 2013

Contents
1. Description
2. IBM MessageSight JMS Client package content
3. Working with sample applications
4. Importing the JMS client into Eclipse
5. Limitations and known problems


DESCRIPTION
===========
This README file for the IBM MessageSight JMS Client details content,
updates, fixes, limitations, and known problems.

For more details about IBM MessageSight, refer to the product 
Information Center: https://pic.dhe.ibm.com/infocenter/ism/v1r0m0/index.jsp 


IBM MESSAGESIGHT JMS CLIENT CONTENT
===================================
The ImsJmsClientV1.0.zip package provides the IBM MessageSight client 
implementation of the Java™ Messaging Service (JMS).  It also provides 
documentation for the classes used to create and configure JMS administered 
objects for use with IBM MessageSight.  Finally, it includes Source Components 
and Sample Materials.

Directories and files:
    ImsClient/
        README.txt - this file
        version.txt - contains version information for the IBM MessageSight
            JMS Client
        
        jms/
            .classpath and .project - Eclipse project configuration files
            	that permit the jms subdirectory to be imported as an
            	Eclipse project
            	
            doc/ - contains documentation for creating and configuring JMS 
                administered objects for IBM MessageSight
            
            java/
                samples/ - contains sample applications (Source Components)
                    demonstrating how to create administered objects, how to
                    send and receive messages via IBM MessageSight and how to 
                    write client applications that are enabled for high 
                    availability configurations
                    
            lib/
                imaclientjms.jar - the IBM MessageSight implementation of the 
                    JMS interface
                jms.jar - the JAR file for the JMS 1.1 interface
                jmssamples.jar - the compiled classes (Sample Materials) for 
                    the sample code (Source Components) provided with the
                    IBM MessageSight JMS Client
                fscontext.jar and providerutil.jar - JAR files which implement 
                    a file system JNDI provider
            


	license.zip - zip archive containing the license agreement for the 
	    IBM MessageSight JMS Client
 
                   
WORKING WITH SAMPLE APPLICATIONS
================================
There are four sample applications provided with the client.  They are:
    JMSSampleAdmin
    JMSSample
    HATopicPublisher
    HADurableSubscriber

The JMSSampleAdmin application demonstrates how to create, configure and store 
JMS administered objects for IBM MessageSight. It reads IBM 
MessageSight JMS administered object configurations from an input
file and populates a JNDI repository. This application can populate either LDAP
or file system JNDI repositories.  

The JMSSample application demonstrates how to send messages to and receive 
messages from IBM MessageSight topics and queues. It is implemented
in three classes, JMSSample, JMSSampleSend and JMSSampleReceive. The 
JMSSample application can either retrieve administered objects from a JNDI 
repository or it can create and configure the required administered objects at 
runtime.

HATopicPublisher and HADurableSubscriber demonstrate how to enable JMS client
applications to leverage IBM MessageSight high availability features.

To run the sample applications provided with the client, three JAR files are 
required in the class path; imaclientjms.jar, jms.jar, and jmssamples.jar.  All 
three files are located in ImsClient/jms/lib. In order to use the file system 
JNDI provider when running the JMSSampleAdmin and JMSSsample applications, two 
additional JAR files are required in the class path; fscontext.jar
and providerutil.jar. Both of these JAR files are also available in 
ImsClient/jms/lib.

Running the compiled samples on Linux:
======================================
Set the CLASSPATH environment variable as follows (running from ImsClient/jms)
on Linux:
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar
    
Or, if you want to use the file system JNDI provider, set CLASSPATH to:
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar:lib/fscontext.jar:lib/providerutil.jar

The following example command lines will run each application and will provide a
usage statement for each (running from ImsClient/jms).

    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSample
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HADurableSubscriber

Running the compiled samples on Windows:
========================================
Set the CLASSPATH environment variable as follows (running from ImsClient/jms)
on Windows:
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar
    
Or, if you want to use the file system JNDI provider, set CLASSPATH to:
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar;lib\fscontext.jar;lib\providerutil.jar

The following example command lines will run each application and will provide a
usage statement for each (running from ImsClient/jms).

    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSample
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HADurableSubscriber
    
To build the sample applications locally, only jms.jar and imaclientjms.jar are 
required in the build class path.


IMPORTING THE JMS CLIENT INTO ECLIPSE
=====================================
To import the IBM MessageSight JMS client into Eclipse, follow
these steps:
1. Unzip the contents of ImsJmsClientV1.0.zip 
2. In Eclipse, select File->General->Existing Projects into Workspace
3. Navigate to and select the jms subdirectory in the unpacked zip file content
4. Click the Finish button

Once you do this, you can compile and run the sample applications and your own 
client applications from Eclipse.


LIMITATIONS AND KNOWN PROBLEMS
==============================
1. The IBM MessageSight JMS Client implements only JSE JMS functionality. 
Optional methods and interfaces related to application server (JEE) support 
are not implemented.

2. The IBM MessageSight JMS implementation delivers messages in the order 
received.  Message priority settings have no affect on delivery order.


NOTICES
=======
1. IBM, the IBM logo, and ibm.com are trademarks or registered trademarks of 
International Business Machines Corp., registered in many jurisdictions 
worldwide. Other product and service names might be trademarks of IBM or other 
companies. A current list of IBM trademarks is available on the Web at 
"Copyright and trademark information" at www.ibm.com/legal/copytrade.shtml. 

2. Java and all Java-based trademarks and logos are trademarks or registered 
trademarks of Oracle and/or its affiliates. 
	
