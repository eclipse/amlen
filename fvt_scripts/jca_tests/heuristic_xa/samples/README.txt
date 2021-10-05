testTools_heurFailRA.rar:
   This resource adapter is a test tool for forcing failures in 2PC global transactions
   at certain times throughout a transaction.
   
   There are 4 configuration properties:
    1. commitFailurePercent - Percentage of time MessageSight should be crashed during commit phase
    2. prepareFailurePercent - Percentage of time MessageSight should be crashed during prepare phase
    3. server - Comma separated list of server IP's
    4. traceFile - File name to trace to
 
testTools_heurFailApp.ear:
   This is an enterprise application containing 1 MDB and 1 EJB. The MDB is configured to
   receive a single message, and call the EJB.
   
   The EJB does the following:
    1. Creates a HeurFailConnection
    2. Connects to MessageSight and sends a reply message.
    
   This results in a container-managed transaction with 3 branches.
    Branch 1: message published to the MDB
    Branch 2: HeurFailConnection
    Branch 3: reply message published from EJB.
    
   In WAS, global transactions are prepared in reverse order (branch 3, 2, 1) and
   they are committed in order (branch 1, 2, 3).
   
   FAILURE IN PREPARE PHASE:
    If the heuristic fail RA is configured to cause failures during prepare, then that means
    branch 3 of the transaction would prepare successfully and branch 1 would not, since
    IMA is crashed during branch 2 prepare.
    
   FAILURE IN COMMIT PHASE:
    If the heuristic fail RA is configured to cause failures during commit phase, then that
    means branch 1 of the transaction would commit successfully and branch 3 would not, since
    IMA is crashed during branch 2 commit. 
    
Required configuration in WAS to use these tools:
 For ima.jmsra.rar: 
  ConnectionFactory with JNDI name "heurReplyCF"
    - At least server and port must be specified
  AdministeredObject with JNDI name "heurSendTopic"
    - Name must be specified
  AdministeredObject with JNDI name "heurReplyTopic"
    - Name must be specified
  ActivationSpecification with JNDI name "HeurFailMDB"
    - port = ...
      server = ...
      SubscriptionName = ...
      SubscriptionShared = NonShared
      SubscriptionDurable = Durable
      ClientID = ...
      destinationLookup = heurSendTopic
      
      NOTE: If in a cluster, don't set ClientID and set SubscriptionShared=Shared
  
Required configuration for MessageSight:
 Endpoint port and policies set to match the WAS configuration
 
NOTE: For the WAS JNDI names, they can actually be named however you want. But to rename
the activation specification JNDI name, you will also have to replace it in
testTools_heurFailApp.ear/testTools_heurFailEJB.jar/META-INF/ibm-ejb-jar-bnd.xml

This can be tested with the JMSTestDriver using jca_heuristic.xml.
Update the server and port on the connection factory in the xml, as well as the topic names.
Usage: java -cp $CLASSPATH com.ibm.ima.jms.test.JMSTestDriver jca_heuristic.xml -n Subscribe -f log
       java -cp $CLASSPATH com.ibm.ima.jms.test.JMSTestDriver jca_heuristic.xml -n Consume -f log
       
Subscribe will publish a message to the MDB.
Consume will receive the message published by the MDB.

For some more in-depth notes on global transactions and how testTools_heurFailRA affects them,
see source file HeurFailXAResource.java