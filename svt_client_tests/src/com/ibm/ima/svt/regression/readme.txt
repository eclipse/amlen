SVT Stress Bucket:-

Description:
The intention of these tests is to create different messaging loads on the system while increasing the amount of information in the store. 
Frequent failovers or server stop/starts are injected during this time which puts additional stress on the system.
Some checking is performed manually, i.e. checking the monitoring tabs in the GUI to ensure the system looks correct
You can run some or all of theses tests together depending on what you are attempting to test but the main benefit is to run them all together. 
You need to be aware that the system will get progressively slower with failover/stopping and may impact the automatic checking in some of the test buckets (fine balance between checking
and stressing the system to potential breaking point).

Areas for future improvement:-
* Add more integrity checking into the bucket
* Increase the load (currently limited by the capability of the Hursley client machines)
* Add more functionality to the bucket e.g. security/external systems such as ldap or WAS (at the moment we do not have any 'buckets' which cover this
functionality as the bucket of tests has morphed from the work I was originally doing and not from the perspective of a regression stress bucket)
* Allow more configurable parameters to be passed into each of the bucket tests i.e. alter the number of clients etc (this is hardcoded as they
were not originally built to be a regression bucket).
* Develop simpler tests for a true regression bucket for automation

Known issues:-
As the tests are running for hours and days the system becomes under alot of stress. This results in server stops taking a long time due to the number
of unacked messages etc held by the clients. It also results in failover and server start times increasing up to over 1 hour due to the number of generations
and store memory usage if all the tests are run together.
The clients used in these tests are used for other tests and are therefore always being modified and thus they may be 'buggy' at times.

Current approach to stress testing the systems is to spread the bucket of tests over a number of client machines, start them all together and monitor the system
for a couple of days. If nothing breaks stop/start some of the test buckets and or look for other areas to try and break while the system is running under stress.

The Bucket of Tests:-

1. 8 queues being accessed by approximately JMS consumers and producers
JMS producers committing messages after n number sent
JMS consumers (some with selectors some not) committing or acking messages after n number received and some which never commit or ack any to cause stress on the system
All designed to cope with any outage in that they keep trying to reconnect no matter what the cause of the outage
The intention is that this puts stress on the system as you get a build up of unacked or uncommitted messages on queues. Coupled with some of the clients
using JMS selectors this has exposed defects and areas for improvement (for example on a normal controlled stop it can take a server several minutes to stop
due to the number of unacked messages etc). Note: We do not have any integrity checking in this bucket because we want to leave the clients autonomously accessing the queues.


To start this bucket use:-
com/ibm/ima/svt/regression/GeneralQueueRegression <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <tear down previous> <trace clients> <timeToRun in milliseconds>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<tear down previous> would be either true or false; if true the queues are torn down including any buffered messages
	<trace clients> would be either true or false; if true a trace file for each of the producers and consumers will be created (use with caution as will fill up file system quickly)
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required

Extra properties: 
	Extra tracing can be enabled using the following java properties: 
		"MyTrace": true|false
		"MyTraceFile":stdout|stderr|<filepath>
	
Output:	
    8 Queues within appliance are created with prefix 'SVTREG-'. 
    With <trace clients> enabled trace files matching the queue names will be created within testbucket execution path.    

Errors:
	
2. 11 Topics being accessed by both JMS and MQTT clients
Pproducers consist of both JMS and MQTT (some JMS committing after n number of msgs sent)
Consumers consist of both JMS and MQTT (some JMS acking/committing after n number received and a couple which never commit or ack). 
Some designed to unsubscribe/disconnect and then reconnect after a certain time has passed. A couple of shared subs for MQTT and JMS. Both durable and non durable.
All designed to cope with any outage in that they keep trying to reconnect no matter what the cause of the outage
The intention is that this puts stress on the system as you get a build up of unacked or uncommitted messages on queues. Coupled with some of the clients
using JMS selectors this has exposed defects and areas for improvement (for example on a normal controlled stop it can take a server several minutes to stop
due to the number of unacked messages etc). Note: We do not have any integrity checking in this bucket because we want to leave the clients autonomously accessing the topics.

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralTopicRegression <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <trace clients> <timeToRun in milliseconds>
	
	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<trace clients> would be either true or false; if true a trace file for each of the producers and consumers will be created (use with caution as will fill up file system quickly)
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required

Extra properties: 
	Extra tracing can be enabled using the following java properties: 
		"MyTrace": true|false
		"MyTraceFile":stdout|stderr|<filepath>
	
Output:	
	Topics within appliance are created with prefix 'SVTREG-'.
	With <trace clients> enabled trace files matching the topic will be created within testbucket execution path.
	Client directories with '*-tcp*' are created within testbucket execution path.

Errors:


3. Two topics - one is sent 256MB messages and the other small messages - each has a durable subscriber that is NOT listening. 
When the memory usage gets to around 25% left on the system the durable subscriber for the large message topic unsubscribes which deletes a large number 
of messages. This is designed to make sure we keep generate generations on disk as well as triggering frequent compaction when the large messages get 
deleted. After a certain number has been sent, and assuming a specific time has passed between the last failover we then trigger an outage 
(for standalone the server is stopped and restarted and for HA we trigger a failover - this can be alternated between a graceful stop and forced stop).
We have no integrity checking in this bucket as this is simply used to create more and more generations on disk which stresses the engine and store.

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralFailoverRegression <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <time between failovers> <resync time> <type of server stop> <filename> <teardown previous> <timeToRun in milliseconds>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<time between failovers> would be in the form of integer value specifying how long between each failover/server stop - use 1200000 and tweak as appropriate for defects/client machines
	<resync time> would be in the form of integer value specifying how long to wait after primary is back up before the standby is started to resync - start at 360000 and tweak as appropruate for defects  - use 0 for standalone machine as this is ignored
	<type of server stop> would be in the form of mixed || true || false || none - mixed alternated between force and graceful stop, true is forced, false is graceful and none means never failover - typically use mixed unless you need to change this for specific test/defect
	<filename> can be anything but is used so you can see when the next failover might occur
	<tear down previous> would be in the form of true of false; true would delete the subscriptions including any buffered messages before restarting the test
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required

Extra properties: 

Output:
	Argument <filename> creates file within execution path.
	
Errors:


4. MQTT and JMS clients access 6 mapping rules for mqconnectivity (both IMA -> MQ and MQ -> IMA)
All designed to cope with any outage in that they keep trying to reconnect no matter what the cause of the outage
We have no integrity checking in this bucket as this is simply used to bring in mqconnectivity into the mix of messaging patterns running on the system

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralMQRegression <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <mq server ipaddress> <mq tcp port> <mq mqtt port> <mq file location> <mquser in group mqm> <user NOT in mqm group> <mquser password> <mq prefix> <teardown previous> <trace clients> <timeToRun in milliseconds>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<mq server ipaddress> would be in the form of 1.1.1.1 for the ipaddress of the MQ server
	<mq tcp port> would be an interger value which is the TCP port of the MQ qmgr listener
	<mq mqtt port> would be an integer value which is the MQTT port of the MQ qmgr
	<mq file location>  would be the location of the MQ installation to execute MQ commands e.g. /opt/mqm/bin
	<mquser in group mqm>  would be the mquser that is in the mqm group who has authority to create MQ objects etc
	<user NOT in mqm group> would be a user on the system who is NOT in the mqm group as this is used on channel auth
	<mquser password> would be the password of the mquser that is in the mqm group
	<mq prefix> would be used to create the qmgr name on the system - I use the initials of the machines so I can easily identify which qmgr is being used in which test
	<teardown previous> would be in the form of true of false; true would delete both the QMGR and the mapping rules including any buffered messages on those subscriptions before restarting the test
	<trace clients> would be either true or false; if true a trace file for each of the producers and consumers will be created (use with caution as will fill up file system quickly)
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required

5. MQTT and JMS producers and consumers accessing 7 topics and 2 queues
These subscribers check that they get the correct messages and have no duplicates - each producer sends a message with client ID and incremented number
All designed to cope with any outage in that they keep trying to reconnect no matter what the cause of the outage
If any failures occur they will write to a folder called errors running in the directory that the tests is started from. If a failure occurs or if
another test writes any files to the same directory this test will then stop all the clients.

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralIntegrityCheck <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <trace clients> <timeToRun in milliseconds>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<trace clients> would be either true or false; if true a trace file for each of the producers and consumers will be created (use with caution as will fill up file system quickly)
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required

Extra properties: 
	Extra tracing can be enabled using the following java properties: 
		"MyTrace": true|false
		"MyTraceFile":stdout|stderr|<filepath>
Output:
	Creates queues within appliance with prefix 'SVTINT-'
	With <trace clients> enabled trace files matching the topic will be created within testbucket execution path.
	Client directories with '*-tcp*' are created within testbucket execution path.
		
Errors: creates './errors' directory with 'SendAndCheckMessages.txt'


6. MQTT and JMS subscribers and producers sending to 2 topics. These have maximum messages specified from 1 to 500 using the old and new discard messages behaviour. 
Some subscribers are configured to disconnect or unsubscribe and then reconnect/subscribe at different times. The JMS producers check they are not booted off the system unless it is a valid failover. 
The subscribers check they still receive messages every x seconds and if using shared subs check for any duplicate messages across the shared subscription
All designed to cope with any planned outage - i.e a valid failover/server stop
If any failures occur they will write to a folder called errors running in the directory that the tests is started from. If a failure occurs or if
another test writes any files to the same directory this test will then stop all the clients.

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralDiscardMessages <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <trace clients> <timeToRun in milliseconds>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<trace clients> would be either true or false; if true a trace file for each of the producers and consumers will be created (use with caution as will fill up file system quickly)
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required

Extra properties: 
	Extra tracing can be enabled using the following java properties: 
		"MyTrace": true|false
		"MyTraceFile":stdout|stderr|<filepath>
Output:
	Creates queues within appliance with prefix 'SVTDIS-'
	With <trace clients> enabled trace files matching the topic will be created within testbucket execution path.
	Client directories with '*-tcp*' are created within testbucket execution path.
		
Errors:creates './errors' directory with 'SendAndCheckMessages.txt'


7. This test will create mqtt producers alternating between durable and non durable and then will disconnect them (durable will leave disconnected mqtt clients
on the system and thus will create owner records in the store).

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralDisconnectClients <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <tear down previous> <number of clients to create> <trace test>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<tear down previous> would be in the form of true of false; if true it would attempt to remove all the previous clients - use with caustion as this could take a very long time
	<number of clients to create> would be an integer value to indicate the number to create
	<trace test> would be in the form of true or false; if true an file containing the trace information is generated

Extra properties:

Output:
	Creates 'DisconnectedClients' file within executed path
	
Errors:

8. This test will create subscribers alternating between durable and non durable and then will disconnect them (durable will leave subscriptions
on the system and thus will create owner records in the store).

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralSubscriptionClients  <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <tear down previous> <number of subs to create> <trace test>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<tear down previous> would be in the form of true of false; if true it would attempt to remove all the previous subscribers - use with caustion as this could take a very long time
	<number of subs to create> would be an integer value to indicate the number to create
	<trace test> would be in the form of true or false; if true an file containing the trace information is generated

Extra properties:

Output:
	Creates 'CreateSubscriptions' file within executed path
	Creates subscriptions with 'SVTSUB-' prefix within appliance
Errors:


9. This test will create topics containing one retained message (this will create owner records in the store)

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralRetainedTopics <ipaddress> <mgt0 ipaddress> <mgt0 ipaddress> <number of topics to create> <trace test>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<number of topics to create> would be an integer value to indicate the number to create
	<trace test> would be in the form of true or false; if true an file containing the trace information is generated
	
Extra properties:

Output:
	Creates 'CreateTopicsWithRetainedMsg' log file within executed path
	Creates topics with 'SVTRET-' prefix within appliance
	
Errors:


10. This test will create multiple MQTT connections, send a message and then disconnect and repeat. This uses the default hub so the endpoint
must be enabled for this to work

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralConnections <ipaddress> <timeToRun in milliseconds> <prefix> <number of connections> <qos> <trace test>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required
	<prefix> can be anything and is used for clientIDs to make then easy to identify
	<number of connections> should be an integer to indicate the number of connection to create in one go - this should be tweaked depending on the client machine capability
	<qos> shoulg be 0 || 1 || 2 and is the mqtt qos 
	<trace test> would be in the form of true or false; if true an file containing the trace information is generated
	
	
Extra properties:

Output:
    Creates 'MultipleConnections' log file within executed path
	Creates clientIDs and topics with <prefix> arguement
	
Errors:

	
11. This test will delete disconnected mqtt clients

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralDeleteMqttClients <mgt0 ipaddress> <mgt0 ipaddress> <trace test> <timeToRun in milliseconds>

	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<trace test> would be in the form of true or false; if true an file containing the trace information is generated
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required
	
	
Extra properties:

Output:
	Creates 'DeleteDisconnectedClient' log file within executed path 
		
Errors:	
	
	
12. This test will delete durable subscriptions if not in use

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralDeleteSubscriptions <mgt0 ipaddress> <mgt0 ipaddress> <topic name> <trace test> <timeToRun in milliseconds>

	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the first (HA) or only appliance (standalone)
	<mgt0 ipaddress> would be in the form of 1.1.1.1 for the mgt0 ipaddress of the second (HA) appliance OR the string null if running as standalone
	<topic name> is the topic which was used by the subscription
	<trace test> would be in the form of true or false; if true an file containing the trace information is generated
	<timeToRun in milliseconds> would be an integer number used to indicate how long to run the test - typically I set this the highest I can and kill the tests manually if required

Extra properties:

Output:
	Creates 'DeleteSubscriptions' log file within executed path 
		
Errors:	


13. This test will remove any retained messages from a topic

To start this bucket use:-
com/ibm/ima/svt/regression/GeneralDeleteRetainedMessages <ipaddress> <topic prefix> <topic post fix start> <topic post fix end> <trace test>

	<ipaddress> would be in the form of either 1.1.1.1 for a standalone ipaddress for messaging OR 1.1.1.1,2.2.2.2 for HA where you pass in both messaging ip addresses
	<topic prefix> this is the string used for the prefix topic name e.g. SVTRET-/3f982de7/
	<topic post fix start> this is the number to start at e.g. SVTRET-/3f982de7/0 .....<the number will be incremented to be used on the topic prefix>
	<topic post fix end> <trace test> this is the number to end at e.g. SVTRET-/3f982de7/10000
	<trace test> would be in the form of true or false; if true an file containing the trace information is generated

Extra properties:

Output:
	Creates 'DeleteRetainedMessages' log file within executed path 
		
Errors:	

