/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

package svt.jms.ism;

import java.util.Date;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.Set;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/**
 * Standalone topic subscriber sample for high availability
 * 
 * This application is intended for use with HATopicPublisher.  Both applications
 * demonstrate how IBM MessageSight JMS client applications can detect that a connection
 * to the server has been lost and can reconnect to the server in order to continue.
 * 
 * To fail over to a backup server when the connection to the primary is lost, the client application must 
 * recreate all of the JMS objects required for the subscription.
 * 
 * There are four positional arguments required to run this application and four optional argument:
 * 1. Server list - the list of server host names or IP addresses where IBM MessageSight is running in a high availability configuration
 * 2. Server port - the connection port for the IBM MessageSight server
 * 3. Topic name - the name of the topic from which messages will be received
 *                 NOTE: This topic name must match the name used for the HATopicPublisher sample
 * 4. Message count - the number of messages to receive
 * 5. Optional receivetimeout - the number of seconds to wait to receive the first message from the server.  The default is 10 seconds.
 * 6. Optional msgWaitTimeOut - the number of seconds to wait to receive subsequent messages from the server.  The default is 10 seconds.
 * 7. Optional forcetimeout - The number of milliseconds to wait before giving giving up on waiting for all messages to be received.  The default is 120 seconds
 * 8. Optional connectTimeout - Apply connection retry logic for up to two minutes before giving up.   The default is 120 seconds
 * 9. Optional Client ID - Client Name at the IBM MessageSight Server
 * 
 * IMPORTANT NOTE:  This sample does not remove the durable subscription so that messages sent by HATopicPublisher 
 * while HADurableSubscriber is is not active will be received when HADurableSubscriber becomes active.  If you prefer
 * to remove the durable topic before this program exits, uncomment the lines in the main program that close the 
 * consumer and unsubscribe the session.
 * 
 * Example command line for the program:
 *      java com.ibm.ima.samples.jms.HADurableSubscriber "server1.mycompany.com, server2.mycompany.com" 16102 durableTopic 2000 clientID
 *      java com.ibm.ima.samples.jms.HADurableSubscriber "server1.mycompany.com, server2.mycompany.com" 16102 durableTopic 2000 clientID  60
 * 
 * To see the usage statement for the program, run it with no arguments:
 *      java com.ibm.ima.samples.jms.HADurableSubscriber
 * 
 */
public class HADurableSubscriber implements Runnable {
    static long forcetimeout = 120000;          /* The number of milliseconds to wait before giving giving up
                                                 * on waiting for all messages to be received. 
                                                 * Timeout is forced in 2 minutes (120 seconds). */
    static int connectTimeout = 120000;         /* Apply connection retry logic for up to two minutes before giving up. */
    
	protected static String clientID;
    static String serverlist = null;            /* A comma delimited list of server host names or IP addresses
                                                 * where IBM MessageSight is running. */ 
    static int serverport;                      /* The connection port for IBM MessageSight. */
    static String topicname = null;             /* The topic name for the durable subscription. */
    static int count;                           /* The number of messages to receive. */
    static int receivetimeout = 10000;          /* The number of milliseconds to wait for the first receive request to succeed. */
	protected static int msgWaitTimeOut = 30000;  /* The number of milliseconds to wait for subsequent receive request to succeed. */

    static ConnectionFactory fact;              /* The connection factory used to create the connection object. */
    static Connection conn;                     /* The connection object used to connect to IBM MessageSight. */
    static Session sess;                        /* The session object used to create the durable subscriber. */
    static MessageConsumer cons;                /* The durable subscriber (consumer) object that receives messages. */

    static boolean recvdone;                    /* Flag that indicates whether the application has finished
                                                 * receiving.  This is set to true when all of the messages
                                                 * have been received or under certain error conditions.
                                                 */

    static int recvcount = 0;                   /* A count of the number of messages received so far. */

    static long lastrecvtime = System.currentTimeMillis();  /* Tracks the last time a message was received. 
                                                             * This time is used to determine whether a 
                                                             * forced timeout is needed. */

	Hashtable<String, LinkedList<Double>> publishersIndexTable = new Hashtable<String, LinkedList<Double>>();
	LinkedList<Double> pubIdxLinkedList = null;

    /**
     * Print simple syntax help.
     * 
     * @param msg   Syntax help message. 
     * 
     */
    public static void syntaxhelp(String msg) {
        System.err.println("java HADurableSubscriber  serverlist serverport topicname count [receivetimeout] [msgwaittimeout] [forcetimeout] [connecttimeout] [clientID]");
		System.err.println("  Example:");
        System.err.println("      java HADurableSubscriber \"server1.mycompany.com, server2.mycompany.com\" 16102 durableTopic 2000 30");
        System.err.println("      java HADurableSubscriber \"server1.mycompany.com, server2.mycompany.com\" 16102 durableTopic 2000 30 60 120 120 clientID ");
        if (msg != null)
            System.err.println("\n" + msg);
        System.exit(1);
    }
    
    /**
     * Main method.
     * 
     * @param args      Command line arguments.  See parseArgs() for details.
     * 
     *  Summary of command line arguments:
     *      serverlist          list of server host names or IP addresses 
     *      serverport          connection port for the IBM MessageSight server
     *      topicname           durable subscription topic name
     *      count               number of messages to receive from the durable topic
     *      [receivetimeout]    number of seconds to wait to receive the first message
     *      [msgtimeout]        number of seconds to wait to receive subsequent messages
     *      [forcetimeout]      number of seconds to retry message receipt
     *      [connectTimeout]    number of seconds to retry connection
     *      [clientID]          Client Name at the IBM MessageSight server
     *      
     */
    public static void main(String[] args) {

        parseArgs(args);

        /*
         * Connect to server
         */
        boolean connected = doConnect();

        if (!connected)
            throw new RuntimeException("Failed to connect!");

        /*
         * Start the receive thread.
         */
        new Thread(new HADurableSubscriber()).start();

        /*
         * Check to see if we have finished receiving messages.
         */
        doCheckDone();

        
        /*
         * Uncomment the following lines if you wish to remove the durable subscription
         * when we have finished receiving messages. Note that this will cause 
         * IBM MessageSight to discard any remaining messages that might exist on 
         * the durable topic.
         */
        //try {
        //    cons.close();
        //    sess.unsubscribe("subscription1");
        //} catch (Exception ex) {
        //    ex.printStackTrace();
        //}
         

        /*
         * Print out the number of message received.
         */
        synchronized (HADurableSubscriber.class) {
            System.out.println(new Date(System.currentTimeMillis())+" Received " + recvcount + " messages");
        }

        /*
         * Close the connection.
         */
        doClose();
//        try {
//            conn.close();
//            System.out.println(new Date(System.currentTimeMillis() )+" Connection closed completed successfully");
//        } catch (Exception e) {
//            e.printStackTrace();
//        }
    }
    

    private static void doClose() {
        try {
            conn.close();
            System.out.println(new Date(System.currentTimeMillis() )+" Connection closed completed successfully");
        } catch (Exception e) {
            e.printStackTrace();
        }

    }
    
    /**
     * Parse command line arguments.
     * 
     * @param args      Command line arguments from the main program. 
     * 
     *  Summary of command line arguments:
     *      serverlist          list of server host names or IP addresses 
     *      serverport          connection port for the IBM MessageSight server
     *      topicname           durable subscription topic name
     *      count               number of messages to receive from the durable topic
     *      [receivetimeout]    number of seconds to wait to receive the first message
     *      [msgtimeout]        number of seconds to wait to receive subsequent messages
     *      [forcetimeout]      number of seconds to retry message receipt
     *      [connectTimeout]    number of seconds to retry connection
     *      [clientID]          Client Name at the IBM MessageSight server
     *      
     */
    public static void parseArgs(String[] args) {
        /* 
         * Provide syntax help if too few or too many arguments appear on the command line.
         */
        if (args.length < 4) {
            if ( args.length == 0)
                syntaxhelp(null);
            syntaxhelp("Too few command line arguments (" + args.length + ")");
        } 
        
		 if (args.length > 9) {
		 syntaxhelp("Too many command line arguments (" + args.length + ")");
		 }
        /* The first required argument is a list of server host names or IP addresses.  This can be a 
         * comma delimited list.  The host names or IP addresses in the list must match those where
         * IBM MessageSight is running in a high availability configuration.
         */
        serverlist = args[0];
        
        /* 
         * The second required argument is the connection port for IBM MessageSight.
         */
        try {
            serverport = Integer.valueOf(args[1]);
        } catch (Exception ex) {
            syntaxhelp(args[1] + " is not a valid server port.  Server port must be an integer value.");
        }
        
        /* 
         * The name of the subscription topic is the third required argument. This name must match the topic
         * name specified in HATopicPublisher.
         */
        topicname = args[2];
        
        /* 
         * The number of messages to receive is the fourth required argument.
         */
        try {
            count = Integer.valueOf(args[3]);
        } catch (Exception e) {
            syntaxhelp(args[3] + " is not a valid count value. Count (number of messages) must be an integer value.");
        }
        
        /* 
         * The fifth argument is optional and specifies how many seconds to wait to receive the first message. The default is 10 seconds.
         * Default is: 10000 milliseconds
         */
        if (args.length > 4) {
            int timeout;
            try {
                timeout = Integer.valueOf(args[4]);
                receivetimeout = timeout * 1000;
            } catch (Exception e) {
                syntaxhelp(args[4] + " is not a valid timeout value. Receivetimeout (number of seconds) must be an integer value.");
            }
        }

        /* 
         * The sixth argument is optional and specifies how many seconds to wait to receive subsequent message. Given is seconds, converted to milliseconds.
         * Default is: 30000 milliseconds
         */
        if (args.length > 5) {
            int timeout;
            try {
                timeout = Integer.valueOf(args[5]);
                msgWaitTimeOut = timeout * 1000;
            } catch (Exception e) {
                syntaxhelp(args[5] + " is not a valid timeout value. Msgtimeout (number of seconds) must be an integer value.");
            }
        }

        /* 
         * The seventh argument is optional and specifies how many seconds to continue to wait to receive all message. Given is seconds, converted to milliseconds.
         * Default is: 120000 milliseconds
         */
        if (args.length > 6) {
            int timeout;
            try {
                timeout = Integer.valueOf(args[6]);
                forcetimeout = timeout * 1000;
            } catch (Exception e) {
                syntaxhelp(args[6] + " is not a valid timeout value. forcetimeout (number of seconds) must be an integer value.");
            }
        }

        /* 
         * The eighth argument is optional and specifies how many seconds to attempt reconnection to IBM SessageSight. Given is seconds, converted to milliseconds.
         * Default is: 120000 milliseconds
         */
        if (args.length > 7) {
            int timeout;
            try {
                timeout = Integer.valueOf(args[7]);
                connectTimeout = timeout * 1000;
            } catch (Exception e) {
                syntaxhelp(args[7] + " is not a valid timeout value. connectTimeout (number of seconds) must be an integer value.");
            }
        }

		 /*
		  * The ninth argument is optional and specifies the name of the Client ID at the IBM MessageSight server. 
		  * This name must be unique at IBM MessageSight and less than 23 chars (MQTT limitation)
		  */
		if (args.length > 8) {
				 clientID = args[8];
			 } else { 
				 clientID = "HASubscriber";
			 }

	}

    /**
     * Establish a connection to the server.  Connection attempts are retried until successful or until
     * the specified timeout for retries is exceeded.
     */
    public static boolean doConnect() {
        int connattempts = 1;
        boolean connected = false;
        long starttime = System.currentTimeMillis();
        long currenttime = starttime;
        
        /* 
         * Try for up to connectTimeout milliseconds to connect.
         */
        while (!connected && ((currenttime - starttime) < connectTimeout)) {
            try { Thread.sleep(5000); } catch (InterruptedException iex) {}
            System.out.println("Attempting to connect to server (attempt " + connattempts + ").");
            connected = connect();
            connattempts++;
            currenttime = System.currentTimeMillis();
        }
        return connected;
    }
    
    /**
     * Run the subscriber thread.
     * 
     * @see java.lang.Runnable#run()
     */
    public void run() {
        try {
            doReceive();
        } catch (Exception ex) {
            ex.printStackTrace(System.err);
            /* 
             * If an exception is thrown while preparing to receive messages, check to see
             * whether the client application is still connected to the server.  If it is not, then 
             * attempt to reconnect and continue receiving.
             */
            if (!recvdone && isConnClosed()) {
                try {                   
                    this.reconnectAndContinueReceiving();
                } catch (JMSException jex) {
                    /* 
                     * If an exception is thrown while attempting to handle the previous exception, 
                     * then reconnect and start a new receive thread.
                     */
                    reconnectAndStartNewReceiveThread();
                }
            } else {
                recvdone = true;
            }
        }
    }

    /**
     * Check to see if we are finished receiving messages.
     */
    public static void doCheckDone() {
        boolean done = false;
        while (!done) {
            synchronized (HADurableSubscriber.class) {
                if (!recvdone) {
                    try {
                        HADurableSubscriber.class.wait(500);
                    } catch (Exception e) {
                    }
                    if (System.currentTimeMillis() - lastrecvtime > forcetimeout)
                        recvdone = true;
                }
                done = recvdone;
            }
        }
    }
    
    /**
     * Connect to the server
     * 
     * @return Success (true) or failure (false) of the attempt to connect.
     */
    public static boolean connect() {
        /*
         * Create connection factory and connection
         */
        try {
            fact = getCF();
            conn = fact.createConnection();
            /* 
             * Check that we are using IBM MessageSight JMS administered objects before calling
             * provider specific check on property values. 
             */
            if (fact instanceof ImaProperties) {
                /* 
                 * For high availability, the connection factory stores the list of IBM MessageSight server
                 * host names or IP addresses.  Once connected, the connection object contains only the host name or IP 
                 * to which the connection is established.
                 */
                System.out.println("Connected to " + ((ImaProperties)conn).getString("Server")+" @ "+new Date(System.currentTimeMillis() ));
            }
			conn.setClientID(clientID);
            sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
            System.out.println("Client ID is: " + conn.getClientID());

            /* 
             * Connection has succeeded.  Return true.
             */
            return true;
        } catch (JMSException jmse) {
            System.err.println(jmse.getMessage());
            /* 
             * Connection has failed.  Return false.
             */
            return false;
        }
    }
    
    /**
     * Receive messages via durable subscription.
     */
    public void doReceive() throws JMSException {
    	Topic durabletopic = getTopic();
    	cons = sess.createDurableSubscriber(durabletopic, "subscription1");
    	conn.start();
    	int i, firstmsg;
    	i = firstmsg = recvcount;

    	do {
    		int timeout = msgWaitTimeOut;
    		if (i == firstmsg)
    			timeout = receivetimeout;
    		try {
    			Message msg = cons.receive(timeout);
    			if (msg == null) {
    				throw new JMSException("Timeout in receive message: " + recvcount );
    			} else {
    				if (msg instanceof TextMessage) {
    					TextMessage tmsg = (TextMessage) msg;
    					String[] smsg = tmsg.getText().split(" ");
    					String theKey = smsg[smsg.length-2];
    					String msgIndex = smsg[smsg.length-1];
    					if (msg.getJMSRedelivered()) {
    						// JMS Redelivered msg, is it a DUP or first time redelivered?
    						pubIdxLinkedList = publishersIndexTable.get(theKey);
    						if (pubIdxLinkedList == null) {
    							pubIdxLinkedList = new LinkedList<Double>();
    							publishersIndexTable.put(theKey, pubIdxLinkedList);
    						}
    						if ( pubIdxLinkedList.contains(new Double(msgIndex)) ) {
    							// duplicate
    							System.out.println("  %===>  Received Duplicate Redelivered JMS Message: " + recvcount +" : " + printStringArray(smsg) +" :");
    						} else {
    							pubIdxLinkedList.add(new Double(msgIndex) );
    							synchronized (HADurableSubscriber.class) {
    								recvcount++;
    								lastrecvtime = System.currentTimeMillis();
    							}
    							if ( Integer.parseInt(msgIndex) == recvcount ) {
        							System.out.println("Received Redelivered JMS message: " + recvcount + ": " + printStringArray(smsg) +" :");
    							} else {
        							System.out.println("Received Redelivered JMS message out of order: " + recvcount + ": " + printStringArray(smsg) +" :");
        						}
    						}

    					} else {
    						// First time for the MSG to be delivered to Client
    						pubIdxLinkedList = publishersIndexTable.get(theKey);
    						if (pubIdxLinkedList == null) {
    							pubIdxLinkedList = new LinkedList<Double>();
    							publishersIndexTable.put(theKey, pubIdxLinkedList);
    						}
    						if ( pubIdxLinkedList.contains(new Double(msgIndex)) ) {
    							// duplicate here is TOTALLY Unexpected!
    							System.out.println("  %===>  Received Duplicate (yet Not marked ReDelivered) JMS Message: " + recvcount +" : " + printStringArray(smsg) +" :");
    						} else {
    							pubIdxLinkedList.add(new Double(msgIndex) );
    							// JMS TextMessage delivered first time (not Re-Delivered, nor Duplicate)
    							if ( Integer.parseInt(msgIndex) == recvcount ) {
    								System.out.println("Received message " + recvcount + ": " + printStringArray(smsg) +" :");
    							} else {
    								System.out.println("  %===>  Received message out of order: " + recvcount + " : " + printStringArray(smsg) +" :");
    							}
    							synchronized (HADurableSubscriber.class) {
    								recvcount++;
    								lastrecvtime = System.currentTimeMillis();
    							}
    						}



    					}
    				} else {
    					// Not an instance of JMS TextMessage
    					synchronized (HADurableSubscriber.class) {
    						recvcount++;
    						lastrecvtime = System.currentTimeMillis();
    					}
    					System.out.println("Received (nonText) JMS Message: " + recvcount);
    				}
    				i++;
    			}

    		} catch (Exception ex) {
    			ex.printStackTrace(System.err);
    			/* 
    			 * If an exception is thrown while messages are received, check to see whether the connection
    			 * has closed.  If it has closed, then attempt to reconnect and continue receiving.
    			 */
    			if (!recvdone && isConnClosed()) {
    				this.reconnectAndContinueReceiving();
    			} else {
    				synchronized (HADurableSubscriber.class) {
    					/* 
    					 * If all messages have already been received or if the connection remains active despite
    					 * the exception, the error should not be addressed with an attempt to reconnect.
    					 * Set recvdone to true in order to exit this sample application.
    					 */
    					recvdone = true;
    				}
    			}
    			break;
    		}
    	} while (recvcount < count);
    	synchronized (HADurableSubscriber.class) {
    		recvdone = true;
    		if (recvcount != count) {
    			System.err.println(new Date(System.currentTimeMillis() ) +" doReceive: Not all messages were received ("+recvcount+") per expected count: " + count);
    		}
    		
            Set<String> keys = publishersIndexTable.keySet();
            for(String key: keys){

            	LinkedList<Double> thisLinkedList = publishersIndexTable.get(key);
        		if (count != thisLinkedList.size() ) {
        			System.err.println(new Date(System.currentTimeMillis() ) +" doReceive: Not all messages were received ("+recvcount
        					+") and logged in list.size: (" + thisLinkedList.size() 
        					+") where expected count was: " + count);
        		}
            }

    	}
    	doReceiveLinger();
    }

    private void doReceiveLinger()  throws JMSException {
		// Catch any unexpected messages arriving after test case thinks it's complete
    	if ( doConnect() ) {
    	Topic durabletopic = getTopic();
    	cons = sess.createDurableSubscriber(durabletopic, "subscription1");
    	conn.start();
    	long startTime = System.currentTimeMillis();
    	long endTime = startTime + receivetimeout;
    	do {
//    		int timeout = msgWaitTimeOut;
    		try {
    			Message msg = cons.receive(msgWaitTimeOut);
    			if (msg == null) {
    				System.out.println("Timeout wait to receive any messages during the Linger after msg#: " + recvcount +" of expected count of: "+ count);
    			} else {
    				if (msg instanceof TextMessage) {
    					TextMessage tmsg = (TextMessage) msg;
    					String[] smsg = tmsg.getText().split(" ");
    					String theKey = smsg[smsg.length-2];
    					String msgIndex = smsg[smsg.length-1];
    					if (msg.getJMSRedelivered()) {
    						// JMS Redelivered msg, is it a DUP or first time redelivered?
    						pubIdxLinkedList = publishersIndexTable.get(theKey);
    						if (pubIdxLinkedList == null) {
    							pubIdxLinkedList = new LinkedList<Double>();
    							publishersIndexTable.put(theKey, pubIdxLinkedList);
    						}
    						if ( pubIdxLinkedList.contains(new Double(msgIndex)) ) {
    							// duplicate
    							System.out.println(" receiveLinger() %===>  Received Duplicate Redelivered JMS Message: " + recvcount +" : " + printStringArray(smsg) +" :");
    						} else {
    							pubIdxLinkedList.add(new Double(msgIndex) );
    							synchronized (HADurableSubscriber.class) {
    								recvcount++;
    								lastrecvtime = System.currentTimeMillis();
    							}
    							if ( Integer.parseInt(msgIndex) == recvcount ) {
        							System.out.println(" receiveLinger() %===>  Received Redelivered JMS message: " + recvcount + ": " + printStringArray(smsg) +" :");
    							} else {
        							System.out.println(" receiveLinger() %===>  Received Redelivered JMS message out of order: " + recvcount + ": " + printStringArray(smsg) +" :");
        						}
    						}

    					} else {
    						// First time for the MSG to be delivered to Client
    						pubIdxLinkedList = publishersIndexTable.get(theKey);
    						if (pubIdxLinkedList == null) {
    							pubIdxLinkedList = new LinkedList<Double>();
    							publishersIndexTable.put(theKey, pubIdxLinkedList);
    						}
    						if ( pubIdxLinkedList.contains(new Double(msgIndex)) ) {
    							// duplicate here is TOTALLY Unexpected!
    							System.out.println(" receiveLinger() %===>  Received Duplicate (yet Not marked ReDelivered) JMS Message: " + recvcount +" : " + printStringArray(smsg) +" :");
    						} else {
    							pubIdxLinkedList.add(new Double(msgIndex) );
    							// JMS TextMessage delivered first time (not Re-Delivered, nor Duplicate)
    							if ( Integer.parseInt(msgIndex) == recvcount ) {
    								System.out.println(" receiveLinger() %===>  Received message " + recvcount + ": " + printStringArray(smsg) +" :");
    							} else {
    								System.out.println(" receiveLinger() %===>  Received message out of order: " + recvcount + " : " + printStringArray(smsg) +" :");
    							}
    							synchronized (HADurableSubscriber.class) {
    								recvcount++;
    								lastrecvtime = System.currentTimeMillis();
    							}
    						}



    					}
    				} else {
    					// Not an instance of JMS TextMessage
    					synchronized (HADurableSubscriber.class) {
    						recvcount++;
    						lastrecvtime = System.currentTimeMillis();
    					}
    					System.out.println(" receiveLinger() %===>  Received (nonText) JMS Message: " + recvcount);
    				}

    			}

    		} catch (Exception ex) {
    			ex.printStackTrace(System.err);
    			/* 
    			 * If an exception is thrown while messages are received, check to see whether the connection
    			 * has closed.  If it has closed, then attempt to reconnect and continue receiving.
    			 */
    			if (!recvdone && isConnClosed()) {
    				this.reconnectAndContinueReceiving();
    			} else {
    				synchronized (HADurableSubscriber.class) {
    					/* 
    					 * If all messages have already been received or if the connection remains active despite
    					 * the exception, the error should not be addressed with an attempt to reconnect.
    					 * Set recvdone to true in order to exit this sample application.
    					 */
    					recvdone = true;
    				}
    			}
    			break;
    		}
    	} while ( System.currentTimeMillis() < endTime );
    	
    	synchronized (HADurableSubscriber.class) {
    		recvdone = true;
    		if (recvcount != count) {
    			System.err.println(new Date(System.currentTimeMillis() ) +" receiveLinger() %===>  Unexpected receive count for messages received ("+recvcount+") per expected count: " + count);
    		}
    		
            Set<String> keys = publishersIndexTable.keySet();
            for(String key: keys){

            	LinkedList<Double> thisLinkedList = publishersIndexTable.get(key);
        		if (count != thisLinkedList.size() ) {
        			System.err.println(new Date(System.currentTimeMillis() ) +" receiveLinger() %===>  Unexpected receive count for messages received ("+recvcount
        					+") and logged in list.size: (" + thisLinkedList.size() 
        					+") where expected count was: " + count);
        		}
            }

    	}
    	doClose();
    	} else {
    		System.err.println(" receiveLinger() %===> Failed to connect to servers.");
    	}
	}

	private String printStringArray(String[] smsg) {
		// TODO Auto-generated method stub
    	String theString = "";
    	for ( int i=0 ; i < smsg.length ; i++ ) {
    		theString += smsg[i] +" " ;
    	}
		return theString;
	}

    /**
     * Check to see if connection is closed.
     * 
     * When the IBM MessageSight JMS client detects that the connection to the server has been
     * broken, it marks the connection object closed.
     * 
     * @return True if connection is closed or if the connection object is null, false otherwise.
     * 
     */
    public static boolean isConnClosed() {
        if (conn != null) {
            /* 
             * Check the IBM MessageSight JMS isClosed connection property
             * to determine whether the connection state is closed.
             * This mechanism for checking whether the connection is closed 
             * is a provider-specific feature of the IBM MessageSight 
             * JMS client.  So check first that the IBM MessageSight JMS 
             * client is being used.
             */
        	try {
    			System.out.println("  %==> 'conn' object is NOT NULL, and is associated with client: "+ conn.getClientID());
    		} catch (JMSException e) {
    			// TODO Auto-generated catch block
    			e.printStackTrace();
    		}
            if (conn instanceof ImaProperties) {
            	System.out.println("  %==> The conn object is ImaProperty and STATE of 'isClosed': "+ ((ImaProperties) conn).getBoolean("isClosed", false) );
                return ((ImaProperties) conn).getBoolean("isClosed", false);
            } else {
                /* 
                 * We cannot determine whether the connection is closed so return false
                 */
            	System.out.println("  %==> The conn object is NOT an ImaProperty -- this is TOTALLY UNEXPECTED!  rtn FALSE" );
                return false;
            }
        } else {
        	System.out.println("  %==> The conn object is NULL (UNEXPECTED?) - rtn TRUE!" );
            return true;
        }
    }
    
    /**
     * Reconnect and continue receiving messages.
     */
    public void reconnectAndContinueReceiving() throws JMSException {
        System.out.println("Connection is closed.  Attempting to reconnect and continue receiving at message #." + recvcount);
        boolean reconnected = doConnect();

        if (reconnected) {
            doReceive();
        } else {
            /* 
             * If we fail to reconnect in a timely manner, then set recvdone to true in order to 
             * exit from this sample application.
             */
            synchronized (HADurableSubscriber.class) {
                System.err.println("Timed out while attempting to reconnect to the server.");
                recvdone = true;
            }
        }
    }
    
    /**
     * Reconnect to the server and start a new receive thread to continue receiving messages.
     */
    public static void reconnectAndStartNewReceiveThread() {
        System.out.println("Connection is closed.  Attempting to reconnect and continue receiving.");
        boolean reconnected = doConnect();

        if (reconnected) {
            new Thread(new HADurableSubscriber()).start();
        } else {
            synchronized (HADurableSubscriber.class) {
                System.err.println("Timed out while attempting to reconnect to the server.");
                recvdone = true;
            }
        }
    }

    /**
     * Get connection factory administered object.
     *
     * @return ConnectionFactory object.
     * 
     * Note: In production applications, this method would retrieve a
     *       ConnectionFactory object from a JNDI repository.
     */
    public static ConnectionFactory getCF() throws JMSException {
        ConnectionFactory cf = ImaJmsFactory.createConnectionFactory();
        /*
         * NOTE: For high availability configurations, the serverlist
         * must contain the list of IBM MessageSight server host names or IPs so that the client
         * can attempt to connect to alternate hosts if connection (or reconnection)
         * to the primary fails.
         */
        ((ImaProperties) cf).put("Server", serverlist);
        ((ImaProperties) cf).put("Port", serverport);
        
        /* 
         * After setting properties on an administered object, it is a best practice to
         * run the validate() method to assure all specified properties are recognized
         * by the IBM MessageSight JMS client. 
         */
        ImaJmsException errors[] = ((ImaProperties) cf).validate(true);
        if (errors != null)
            throw new RuntimeException("Invalid properties provided for the connection factory.");
        return cf;       
    }
    
    /**
     * Get topic administered object.
     * 
     * @return Topic object.
     * 
     * Note: In production applications, this method would retrieve a
     *       Topic object from a JNDI repository.
     */
    public static Topic getTopic() throws JMSException {
        Topic dest = ImaJmsFactory.createTopic(topicname);
        return dest;
    }
}
