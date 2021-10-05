testTools_JCA builds testTools_JCAtests.ear into testTools_ship/lib/

    This ear file consists of testTools_JCAtests.jar and testTools_JCAtests.war

    testTools_JCAtests.jar contains several message-driven beans
        MDBTopicBean
        MDBQueueBean
        etc.
    
    For the MDB's to work:
        You must create the appropriate connection factories, topics, queues, and activation specifications.
        Then, when installing the ear file to the server, the MDB's are assigned to these objects.

    testTools_JCAtests.war contains several servlets
        JCAServlet - "http://localhost:9080/testTools_JCAtests/JCAServlet?param=[test number]"
        JMSAPIServlet - "http://localhost:9080/testTools_JCAtests/JMSAPIServlet?param=[hostname]"
    
testTools_JCAra builds ima.evilra.rar into testTools_ship/lib/
    This rar file is a resource adapter used for injecting failures into global transactions.
 
testTools_JCAfail builds testTools_JCAfail.ear into testTools_ship/lib/

    This ear file consists of testTools_JCAfailEJB.jar

    testTools_JCAfailEJB.jar contains 1 message-driven bean
        FailMDB
   
    This MDB is never expected to start. The purpose of the fail application is to test configurations
    that prevent the application from ever starting.