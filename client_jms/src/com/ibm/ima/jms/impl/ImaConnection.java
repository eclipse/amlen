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

package com.ibm.ima.jms.impl;

import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.security.SecureRandom;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

import javax.jms.Connection;
import javax.jms.ConnectionConsumer;
import javax.jms.ConnectionMetaData;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.Queue;
import javax.jms.QueueConnection;
import javax.jms.QueueSession;
import javax.jms.ServerSessionPool;
import javax.jms.Session;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicSession;
import javax.jms.XAConnection;
import javax.jms.XASession;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaProperties;

/**
 * Implement the Connection interface for the IBM MessageSight JMS client.
 */
public class ImaConnection extends ImaReadonlyProperties implements Connection, TopicConnection, QueueConnection,
        XAConnection, ImaTraceable {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    static final int Client_version_1     = 1000000;
    
    /* Add shared subscription and enable global transactions  */
    static final int Client_version_1_1   = 1010000;
    
    /* Current client version */
    static final int Client_version       = Client_version_1_1;

    
    static final Timer jmsClientTimer = new Timer("JmsClientTimer", true);
    static final Timer jmsConnCloseTimer = new Timer("JmsConnCloseTimer", true);

    ImaClient client;
    String  clientid;                                 /* Client ID                     */
    boolean isloopbackclient = false;
    boolean generated_clientid = false;
    ImaMetaData metadata = new ImaMetaData();         /* IMA metadata                  */
    ExceptionListener exlisten;                       /* Exception listener            */

    boolean isStopped = true;                         /* The connection is stopped     */
    final AtomicBoolean isClosed = new AtomicBoolean(false);                /* The connection is closed      */
    boolean    isInited = false;                      /* The connection is initialized */
    int        domain;
    int        disableTimeout;
    AtomicLong seqnum = new AtomicLong();
    ImaTraceImpl traceImpl;
    boolean    isRA = false;
    static HashSet<String> pendingClose = new HashSet<String>();

    /* List of sessions in this connection */
    ConcurrentHashMap<Integer, ImaSession> sessionsMap = new  ConcurrentHashMap<Integer, ImaSession>(64);

    /* Map of consumers in this connection */
    ConcurrentHashMap<Integer, ImaConsumer> consumerMap = new ConcurrentHashMap<Integer, ImaConsumer>(64); 
    
    /* List of destinations in this connection */
    HashMap<String, Object> destList = new HashMap<String, Object>();

    OutputStreamWriter logF = null;                   /* Log output writer            */ 
    int currentLogLeft = Integer.MAX_VALUE;           /* Bytes left until new log     */
    String logName = null;                            /* Current log file name        */
    int    logMax = 0;                                /* Log file max setting         */

    byte [] instanceHash;
    
    static final ThreadLocal<ImaConnection> currentConnection = new ThreadLocal<ImaConnection>();
    
    static String disableUserid = null;
    static {
        try {
            disableUserid = (String) AccessController.doPrivileged(new PrivilegedAction<Object>() {
                public Object run() {
                   return System.getProperty("IMADisableUserid", null);
                }
            });
        } catch (Exception e) {}
    }
    
    
    /*
     * Constructor for a connection.
     */
    public ImaConnection(ImaProperties props, String userid, String password, int domain, int traceLevel) throws JMSException {
        super(new ImaPropertiesImpl(ImaJms.PTYPE_Connection));

        ImaTrace.init();
        if ((traceLevel < 0) || ImaTrace.dynamicTrace) {
            traceImpl = ImaTrace.imaTraceImpl;
        } else { 
            traceImpl = new ImaTraceImpl(ImaTrace.imaTraceImpl, traceLevel%10);
            if (traceLevel >= 10) {
                isRA = true;
            }
        }
        
        this.props.putAll(((ImaPropertiesImpl)props).props);
        clientid = defaultClientID(props, false);
        if (clientid != null) {
            validateClientID(clientid);
            this.props.put("ClientID", clientid);
        }   
        if (userid != null)
            this.props.put("userid", userid);

        this.props.put("isStopped", true);
        this.domain = domain;

        disableTimeout =  getInt("DisableTimeout", 0);

        int protocol = ImaJms.ProtocolEnum.getValue(getString("Protocol", "tcp"));
        if (!((protocol == 1) || (protocol == 2))) {
        	ImaJmsPropertyException jex = new ImaJmsPropertyException("CWLNC0306", "A call to createConnection() or to the ImaProperties validate() method failed because the property {0} was set to {2}.  The value must be one of: {1}.", "Protocol", ImaJms.ProtocolEnum.getNameString(), this.props.get("Protocol"));
        	traceException(2, jex);
        	throw jex;
        }
        String server = getString("Server"); 
        int port = getInt("Port", 16102);
        if (server == null) 
            server = "127.0.0.1";
        String servers[] = server.split("[\\s+]|[,]|[\\s]*,[\\s]*");
        Exception firstex = null;
        for (int i = 0; i < servers.length; i++) {
        	isClosed.set(false); 
            this.props.put("isClosed", false);
        	try {
                synchronized (this) {
                    switch(protocol) {
                	case 1:
                        client = new ImaClientTcp(this, servers[i], port, this.props);
                        break;
                	case 2:
                        client = new ImaClientSSL(this, servers[i], port, this.props);
                        break;
                    }	
        		}
                client.startClient();
                /* If we get this far the connection was successful.  We can 
                 * exit the loop.
                 */
                this.props.put("Server", servers[i]);
                break;
        	} catch (Exception ex) {
        		if (firstex == null)
        			firstex = new Exception("Connection to server " + servers[i] + " has failed", ex);
        		
                if (isTraceable(2)) {
                    trace("Unable to connect to messaging server " + servers[i] +": connect=" + this.toString(ImaConstants.TS_All));
                    traceException(ex);

                }
                
                /* If this is the last entry in the server list, throw an exception */
        		if (i == servers.length - 1) {
        			this.isClosed.set(true);
        			try {
        				this.props.put("isClosed", true);
        			} catch (JMSException jex) {
        				/* Ignore */
        			}
        			ImaJmsExceptionImpl finalex = new ImaJmsExceptionImpl("CWLNC0022", firstex, 
        			        "The client failed to connect to the specified MessageSight hosts. Servers: {0} Port={1} Protocol={2}." + 
        			        "\nTo see failure information for all failed connection attempts, enable client trace at level 2 or above.",
        			        server, ""+port, protocol==2 ? "tcps" : "tcp");
                    if (isTraceable(2)) {
                        trace("Unable to connect to any of the specified messaging servers: connect=" + this.toString(ImaConstants.TS_All));
                        traceException(finalex);
                    }
                    throw finalex;
        		}
        	}
        }
        
        if (isTraceable(7)) {
        	if (password != null)
        		putInternal("password", "*");  /* obfuscate, but show it is set */
	        trace("ImaConnection object is created:" +
	            " JMSVersion=" + metadata.getJMSVersion() +
	            " JMSProviderVersion=" + metadata.getProviderVersion() +
	            " Protocol=" + getString("Protocol", "tcp") +
	            " Properties=" + this.props);
        }
        
        /* Wait until after the trace */
        if (password != null) {
            putInternal("password", hashString(password));
            if (disableUserid != null) {
                if (disableUserid.equals(userid))
                    throw new ImaJmsSecurityException("CWLNC0207", "A client request to MessageSight failed due to an authorization failure.");  /* Exception for TCK */      
            }
        }        
    }   

    
    /*
     * Startup the objects.
     */
    void connect() throws JMSException {
        checkClosed();
        /*
         * Start the transmitter instance
         */
        synchronized (this) {
            if (isInited)         /* Check again now we are in a synchronized section */
                return;

            /* Check if clientID has been changed */
            if (clientid == null) {
                clientid = defaultClientID(props, true);
                this.props.put("ClientID", clientid);
            }    
            if (clientid != null)
                validateClientID(clientid);
            
            if (isTraceable(7)) {
            	trace("Connecting: connect="+toString(ImaConstants.TS_All));
            }
            int i = 1;
            while (true) {
                /*
                 * Send the create connection
                 */
                ImaAction act = new ImaConnectionAction(ImaAction.createConnection, client);
                act.outBB = ImaUtils.putIntValue(act.outBB, Client_version);   /* val0 */
                act.outBB = ImaUtils.putIntValue(act.outBB, domain);           /* val1 */
                act.outBB = ImaUtils.putStringValue(act.outBB, clientid);      /* val2 */
                act.setHeaderCount(3);
                act.outBB = ImaUtils.putMapValue(act.outBB, this);
                act.request();
                if (act.rc == 0)
                	break;

                /* Map return codes */
            	switch(act.rc) {        		
            	case  ImaReturnCode.IMARC_ClientIDInUse:
            		if (i == 3 || clientid.charAt(0)=='_') {
                        this.props.remove("password");
                        isClosed.set(true);
                        this.props.put("isClosed", true);
                        client.close();
    	            	trace(2, "The connection failed: reason=The client id is already in use. connect=" + 
    	            			toString(ImaConstants.TS_All));
    	            	ImaInvalidClientIDException jex = new ImaInvalidClientIDException("CWLNC0205", "A request to connect to MessageSight failed because client ID {0} is in use.", clientid);
               			traceException(2, jex);
               			throw jex;
            		}
					try {
						Thread.sleep(1000*i);
					} catch (InterruptedException e) {
					}
            		i++;
            		break;
           		case ImaReturnCode.IMARC_NotAuthorized:
           		case ImaReturnCode.IMARC_NotAuthenticated: 
                    this.props.remove("password");
                    isClosed.set(true);
                    this.props.put("isClosed", true);
                    client.close();
           			trace(2, "Connection failed: reason=Authorization Failed. connect=" +
	            			toString(ImaConstants.TS_All));
           			ImaJmsSecurityException jex = new ImaJmsSecurityException("CWLNC0207", "A client request to MessageSight failed due to an authorization failure.");
       	            traceException(2, jex);
       	            throw jex;
           			
           		default:
                    this.props.remove("password");
                    isClosed.set(true);
                    this.props.put("isClosed", true);
                    client.close();
            		trace(2, "The connection failed: rc=" + act.rc + " connect=" +
	            			toString(ImaConstants.TS_All));
            		ImaJmsExceptionImpl jex2 = new ImaJmsExceptionImpl("CWLNC0208", "A request to connect to MessageSight failed with return code = {0}.", act.rc);
           			traceException(2, jex2);
           			throw jex2;
            	}
            }
            this.props.remove("password");
            if (isTraceable(4)) {
            	trace("Connection successful: connect=" + toString(ImaConstants.TS_All));
            }                        
            isInited = true;
        }    
    }

    /*
     * This package private constructor is only used for unit testing.
     */
    ImaConnection(Boolean setId) throws JMSException {
        super((ImaProperties)new ImaPropertiesImpl(ImaJms.PTYPE_Connection));
        ImaTrace.init();
        traceImpl = ImaTrace.imaTraceImpl;
        if (setId) {
            clientid = "_TEST_";
            this.props.put("ClientID", clientid);
        }
    }
        
    static char [] base62 = {
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
        'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
        'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
        'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
    };

    /* 
     * Create a default client ID
     */
    private String defaultClientID(ImaProperties props, boolean force) throws JMSException{
        String clientid = null;
        if (props != null)
            clientid = props.getString("ClientID");           /* Look in properties */
        if (clientid == null || clientid.length()==0) {
            if (!isRA) {
                try {
                    clientid = (String)AccessController.doPrivileged(new PrivilegedAction<Object>() {
                        public Object run() { 
                            return System.getenv("ClientID");     /* Look in another env variable */
                        }
                    });    
                } catch (Exception e) { }
            }
            
            /*
             * Generate a client ID.  If we invent one, it is not usable for non-shared durable subscriptions
             */
            if ((clientid == null || clientid.length() == 0) && force) {                  
            	int count = 8;
            	int where = 1;
            	char [] cidbuf = new char[20];
            	cidbuf[0] = '_';
            	try {
            		trace(4, "Generating clientID for connection "+this.toString(ImaConstants.TS_Common));
                    clientid = InetAddress.getLocalHost().getHostName();
            	} catch (java.net.UnknownHostException e) {
            	    clientid = "client";
            	}
                if (clientid.length() < 8)
                	count = clientid.length();
                for (int i=0; i<count; i++) {
                	if (clientid.charAt(i)=='.')
                		break;
                	if (clientid.charAt(i) >= 0xd800)
                		break;
                    if (Character.isJavaIdentifierPart(clientid.charAt(i)))
                        cidbuf[where++] = clientid.charAt(i);
                }
                if (where==1)
                    cidbuf[where++] = 'C';
            	cidbuf[where++] = '_';
            	
            	/*
            	 * Use secure random to create about 48 bits of randomness
            	 */
            	byte [] rbuf = new byte [6];
            	SecureRandom rnd = new SecureRandom();
            	rnd.nextBytes(rbuf);
            	long rval = 0;
            	for (int i=0; i<6; i++) {
            	    rval = (rval<<8) + (rbuf[i]&0xff);
            	}    
                for (int i=0; i<8; i++) {
                    cidbuf[where++] = base62[(int)(rval%62)];
                    rval /= 62;
                }
                clientid = new String(cidbuf, 0, where);
                 // System.out.println(clientid);
                trace(4, "Generated clientID " + clientid + " for connection " + this.toString(ImaConstants.TS_Common));
                generated_clientid = true;
            }
        }
        if (clientid != null && clientid.length() == 0)
            clientid = null;
        return clientid;
    }


    /*
     * Make sure the clientID is valid
     */
    private void validateClientID(String clientid) throws JMSException {
        if (!ImaUtils.isValidUTF16(clientid)) {
        	ImaInvalidClientIDException jex = new ImaInvalidClientIDException("CWLNC0032", "The string that is specified as the clientID for a connection is not a valid UTF-16 string.");
            traceException(2, jex);
            throw jex;
        }
    }


    /* 
     * Close a connection.
     * This does an implied stop.
     * 
     * @see javax.jms.Connection#close()
     */
    public void close() throws JMSException {
        
        ImaConnectionFactory.checkAllowed(this);
        
        if (!isClosed.compareAndSet(false, true))
            return;
        
        /*
         * Close all sessions
         */
        synchronized (this) {
			if (isTraceable(7)) {
				trace("Connection is closing: connect=" + toString(ImaConstants.TS_Common));
			}

            /* TODO: Do we need to do this, or should we just do this at the server */
            props.put("isClosed", true);
            Iterator <ImaSession> it = sessionsMap.values().iterator();
            while (it.hasNext()) {
                ImaSession sess = it.next();
                it.remove();
                sess.closeExternal();
                sess = null;
            }

            ImaAction act = new ImaConnectionAction(ImaAction.closeConnection, client);
            act.request();            
            client.close();
            if (isTraceable(4)) {
				trace("Connection is closed: connect=" + toString(ImaConstants.TS_Common));
            }
        }
    }

    /*
     * Close the client on finalize of the application failed to close it.
     * @see java.lang.Object#finalize()
     */
    protected void finalize() {
        client.close();
    }


    /* 
     * Check if the session is closed
     */
    void checkClosed() throws JMSException {
        if (isClosed.get()) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0008", "A call to a Connection object method failed because the connection is closed.");
            traceException(2, jex);
            throw jex;
        }
    }


    /* 
     * Create a connection consumer.
     * This optional function is not supported.
     * @see javax.jms.Connection#createConnectionConsumer(javax.jms.Destination, java.lang.String, javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createConnectionConsumer(Destination arg0, String arg1, ServerSessionPool arg2, int arg3)
            throws JMSException {
        checkClosed();
        throw ImaJms.notImplemented("createConnectionConsumer");
    }


    /* 
     * Create a durable connection consumer.
     * This optional function is not supported.
     * @see javax.jms.Connection#createDurableConnectionConsumer(javax.jms.Topic, java.lang.String, java.lang.String, javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createDurableConnectionConsumer(Topic arg0, String arg1, String arg2,
            ServerSessionPool arg3, int arg4) throws JMSException {
        checkClosed();
        throw ImaJms.notImplemented("createDurableConnectionConsumer");
    }


    /* 
     * Create a session.
     * @see javax.jms.Connection#createSession(boolean, int)
     */
    public Session createSession(boolean transacted, int ackmode) throws JMSException {
        return createSessionInternal(transacted, ackmode, false);
    }

    ImaSession createSessionInternal(boolean transacted, int ackmode, boolean isXA) throws JMSException {
        checkClosed();
        connect();
        String domainName = ((ImaProperties)this).getString("ObjectType");
        int domain = ImaJms.Common;
        if (domainName != null) {
            if (domainName.equals("queue"))
                domain = ImaJms.Queue;
            if (domainName.equals("topic"))
                domain = ImaJms.Topic;
        } else {
            domainName = "common";
        }
        final ImaSession session;
        /* TODO: handle ackmode = -1 */
        if (isXA) {
            session = new ImaXASession(this, props, domain);
        } else {
            session = new ImaSession(this, props, transacted, ackmode, domain, false);
        }
        session.putInternal("ObjectType", domainName);
        onCreateSessionComplete(session);
        if (isTraceable(5) ) {
	        trace("Session is created: session=" + session.toString(ImaConstants.TS_All));
        }
        
        return session;
    }


    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XAConnection#createXASession()
     */
    public XASession createXASession() throws JMSException {
        return (XASession) createSessionInternal(true, Session.AUTO_ACKNOWLEDGE, true);
    }

    /* 
     * Get the client ID.
     * @see javax.jms.Connection#getClientID()
     */
    public String getClientID() throws JMSException {
        checkClosed();
        return clientid;
    }


    /* 
     * Get the exception listener.
     * @see javax.jms.Connection#getExceptionListener()
     */
    public ExceptionListener getExceptionListener() throws JMSException {
        checkClosed();
        return exlisten;
    }


    /* 
     * Get the metadata.
     * @see javax.jms.Connection#getMetaData()
     */
    public ConnectionMetaData getMetaData() throws JMSException {
        checkClosed();
        return metadata;
    }


    /* 
     * Set the client ID.
     * @see javax.jms.Connection#setClientID(java.lang.String)
     */
    public void setClientID(String clientid) throws JMSException {
        checkClosed();
        if (this.clientid != null) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0030", "A call to setClientID() on a Connection object failed because the client ID was previously set.");
            traceException(2, jex);
            throw jex;
        }
        if (clientid == null) {
        	ImaInvalidClientIDException jex = new ImaInvalidClientIDException("CWLNC0031", "A call to setClientID() on a Connection object failed because a null input string was provided for the clientID value.");
            traceException(2, jex);
            throw jex;
        }
        validateClientID(clientid);
        this.props.put("ClientID", clientid);
        this.clientid = clientid;
        connect();
    }


    /* 
     * Set the exception listener.
     * @see javax.jms.Connection#setExceptionListener(javax.jms.ExceptionListener)
     */
    public void setExceptionListener(ExceptionListener exlisten) throws JMSException {
        checkClosed();
        this.exlisten = exlisten;
    }


    /* 
     * Start a connection.
     * Nothing is actually started until a session is started.
     * @see javax.jms.Connection#start()
     */
    public synchronized void start() throws JMSException {
        checkClosed();
        if (isStopped){
        	if (!isInited)
                connect();
            ImaAction act = new ImaStartConnectionAction(this);
            /* For the restart case write last delivered messages sqn for all consumers*/
            long []sqns = new long[consumerMap.size()*2];
            int sqnCount = 0;
            Iterator<ImaConsumer> it = consumerMap.values().iterator();
            while(it.hasNext()){
                ImaConsumer ic = it.next();
                if (ic instanceof ImaMessageConsumer) {
                    ImaMessageConsumer consumer = (ImaMessageConsumer) ic;
                    sqns[sqnCount++] = consumer.consumerid;
                    sqns[sqnCount++] = consumer.lastDeliveredMessage;
                }
            }
            if (((ImaMetaData)this.getMetaData()).getImaServerVersion() < 2.0) {
                if (sqnCount > 0) {
                    act.outBB = ImaUtils.putIntValue(act.outBB,sqnCount);
                    act.setHeaderCount(1);
                    act.outBB = ImaUtils.putNullValue(act.outBB);          /* Properties */                
                    act.outBB = ImaUtils.putNullValue(act.outBB);          /* Body */                
                    for(int i = 0; i < sqnCount; i++){
                        act.outBB = ImaUtils.putIntValue(act.outBB,(int)sqns[i++]);
                        act.outBB = ImaUtils.putLongValue(act.outBB,sqns[i]);
                    }
                }
            }
            act.request();
            if (act.rc == 0) {
                isStopped = false;
                props.put("isStopped", false);
                if (isTraceable(4)) {
                    trace("Connection is started: connect=" + toString(ImaConstants.TS_Common));
                }
            } else {
                ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0098", "A call to start() on a Connection object failed with MessageSight return code: {0}.", act.rc );
                traceException(2, jex);
                throw jex;                
            }
        }

    }


    /* 
     * @see javax.jms.Connection#stop()
     * 
     */
    public synchronized void stop() throws JMSException {
        checkClosed();
        if (calledFromOnMessage()) {
            ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0099", "A call to the stop() method on a Connection object failed because it was called from onMessage().");
            traceException(2, jex);
            throw jex;
            
        }
        if (!isStopped) {
            
            /*
             * First, synchronously stop message delivery on each session.
             */
            if (((ImaMetaData)this.getMetaData()).getImaServerVersion() >= 2.0) {
                Iterator<ImaSession> s_it = sessionsMap.values().iterator();
                while (s_it.hasNext()) {
                	ImaSession is = s_it.next(); 
                	ImaAction act = new ImaSessionAction(ImaAction.stopSession, is);
                    long []sqns = new long[is.consumerList.size()*2];
                    int sqnCount = 0;
                    Iterator<ImaConsumer> it = is.consumerList.iterator();
                    while(it.hasNext()) {
                        ImaConsumer ic = it.next();
                        if (ic instanceof ImaMessageConsumer) {
                            ImaMessageConsumer consumer = (ImaMessageConsumer) ic;
                            consumer.stopDelivery();
                            sqns[sqnCount++] = consumer.consumerid;
                            sqns[sqnCount++] = consumer.lastDeliveredMessage;
                        }
                    }

                    if (sqnCount > 0) {
                        act.outBB = ImaUtils.putIntValue(act.outBB,sqnCount);
                        act.setHeaderCount(1);
                        act.outBB = ImaUtils.putNullValue(act.outBB);          /* Properties */                
                        act.outBB = ImaUtils.putNullValue(act.outBB);          /* Body */                
                        for(int i = 0; i < sqnCount; i++){
                            act.outBB = ImaUtils.putIntValue(act.outBB,(int)sqns[i++]);
                            act.outBB = ImaUtils.putLongValue(act.outBB,sqns[i]);
                        }
                    }

                	act.request(true);
                    if (act.rc == 0) {
                        if (isTraceable(4))
                      	  trace(4, "Stop session: session=" + is.toString(ImaConstants.TS_All));
                    } else {
                    	/* TODO: Throw an exception if there's an error here. */
                        if (isTraceable(2)) {
                            trace("Failed to stop session: rc=" + act.rc + " session=" +
                            		is.toString(ImaConstants.TS_All));
                        }
                	}
                }
            }
            
            /*
             * Finish the stop action by setting the connection status to stopped
             * after message delivery is successfully stopped for all of the sessions.
             */
            ImaAction act = new ImaConnectionAction(ImaAction.stopConnection, client);
            act.request();
            isStopped = true;
            props.put("isStopped", true);
            
            if (isTraceable(4)) {
                trace("Connection is stopped: connect=" + toString(ImaConstants.TS_Common));
            }
        }
    }


    /*
     * Create the connection consumer for a topicConnection.
     * Connection consumers are not supported.
     * @see javax.jms.TopicConnection#createConnectionConsumer(javax.jms.Topic, java.lang.String, javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createConnectionConsumer(Topic arg0, String arg1, ServerSessionPool arg2, int arg3)
            throws JMSException {
        checkClosed();
        throw ImaJms.notImplemented("createConnectionConsumer");
    }


    /*
     * Create a TopicSession.
     * @see javax.jms.TopicConnection#createTopicSession(boolean, int)
     */
    public TopicSession createTopicSession(boolean transacted, int ackmode) throws JMSException {
        checkClosed();
        connect();
        ImaSession session = new ImaSession(this, props, transacted, ackmode, ImaJms.Common, false);
        session.putInternal("ObjectType", "topic");
        onCreateSessionComplete(session);
        if (isTraceable(5)) {
	        trace("TopicSession is created: session=" + session.toString(ImaConstants.TS_All));
        }
        return (TopicSession)session;
    }


    /*
     * Create the connection consumer for a queueConnection.
     * Connection consumers are not supported.
     * @see javax.jms.QueueConnection#createConnectionConsumer(javax.jms.Queue, java.lang.String, javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createConnectionConsumer(Queue arg0, String arg1, ServerSessionPool arg2, int arg3)
            throws JMSException {
        checkClosed();
        throw ImaJms.notImplemented("createConnectionConsumer");
    }

    
    /*
     * Create a QueueSession.
     * @see javax.jms.QueueConnection#createQueueSession(boolean, int)
     */
    public QueueSession createQueueSession(boolean transacted, int ackmode) throws JMSException {
        checkClosed();
        connect();
        ImaSession session = new ImaSession(this, props, transacted, ackmode, ImaJms.Queue, false);
        session.putInternal("ObjectType", "queue");
        onCreateSessionComplete(session);
        if (isTraceable(5)) {
	        trace("QueueSession is created: session=" + session.toString(ImaConstants.TS_All));
        }
        return (QueueSession)session;
    }

    /*
     * 
     */
    synchronized void onCreateSessionComplete(ImaSession session) throws JMSException {
        checkClosed();
        session.isStopped = isStopped;
        sessionsMap.put(new Integer(session.sessid), session);
    }

    /*******************************************************************
     * 
     * Package private implementation methods
     * 
     ******************************************************************/

    /*
     * Remove the session from the session list.
     * This is called from the session close function
     */
    void closeSession(ImaSession session) {
        synchronized (this) {
            sessionsMap.remove(new Integer(session.sessid));
        }    
    }

    /*
     * Send a JMS exception found in an asynchronous location.
     */
    void raiseException(final JMSException jmse) {
    	traceException(2, jmse);
    	TimerTask onExceptinTask = new TimerTask() {
            @Override
            public void run() {
                if (exlisten != null)
                    exlisten.onException(jmse);
                else
                    defaultOnException(jmse);
            }
        };
        ImaConnection.jmsConnCloseTimer.schedule(onExceptinTask, 0);
    }


    /*
     * Get an integer value.
     */
    static int getIntOptionValue(String val, int defval) {
        try {
            return Integer.parseInt(val);
        } catch (Exception e) {
            return defval;
        }
    }
    
    static String a16 = "rtoaphcleduminskgbxqfwjyvz";
    /*
     * Create a hashed string based on the instance hash
     */
    String hashString(String s) {
        int len = ImaUtils.sizeUTF8(s);
        byte [] ba = new byte[len];
        char [] ca = new char[len*3];
        int     calen = 0;
        ImaUtils.makeUTF8(s, len, ByteBuffer.wrap(ba));
        for (int i=0; i<ba.length; i++) {
        	byte b = (byte)(ba[i] ^ instanceHash[i%4]);
        	ca[calen++] = a16.charAt((b>>4)&0xf);
        	ca[calen++] = a16.charAt(b&0x0f);
        	int which = (int)(Math.random() * 26);
        	if (which>15)
        		ca[calen++] = a16.charAt(which);
        }
        return new String(ca, 0, calen);
    }
    
    /*
     * Handle a raise exception for this connection
     */
    void onError(Exception e) {
    	final JMSException jmse;
        if (isTraceable(6)) {
            trace("onError: connection="+this.toString(ImaConstants.TS_All) + " reason=" + e);
        }
        if (e instanceof JMSException)
            jmse = (JMSException)e;
        else
            jmse = new ImaJmsExceptionImpl("CWLNC0023", e, "The client failed unexpectedly.");
    	

        markAllClosed(jmse);
        client.close();
    	try {
        	this.props.put("isClosed", true);
    	} catch (JMSException jex) {
    		/* Ignore */
    	}
    	raiseException(jmse);
    }
    

    /*
     * Return a string representation of the connection.
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "ImaConnection:"+clientid;
    }
    
    /*
     * Detailed toString
     * @see com.ibm.ima.jms.impl.ImaReadonlyProperties#toString(int)
     */
    public String toString(int opt) {
    	return toString(opt, this);
    }

    /*
     * Part of detailed toString
     * @see com.ibm.ima.jms.impl.ImaReadonlyProperties#getClassName()
     */
    public String getClassName() {
    	return "ImaConnection";
    }
    
    /*
     * Determine if this method is being called from an onMessage callback
     */
    private final boolean calledFromOnMessage(){
        return (ImaReceiveAction.currentConnection.get() == this);
    }
    
    /*
     * Cancel actions and mark the connection closed
     */
    void markAllClosed(JMSException jmse) {
        if (isTraceable(7)) {
            trace("Marking connection closed: connect=" + toString(ImaConstants.TS_Common));
        }
        ((ImaPropertiesImpl)props).putInternal("isClosed",true);
        
        LinkedList<ImaAction> pendingActions = ImaAction.getPendingActions();
        for (ImaAction action : pendingActions) {
            if (action instanceof ImaConnectionAction) {                
                ImaConnectionAction conAct = (ImaConnectionAction) action;
                if (conAct.client == this.client)
                    conAct.cancel(jmse); 
            }
        }
        if (isClosed.compareAndSet(false, true)) {
            if (!isStopped) {
                isStopped = true;
                ((ImaPropertiesImpl)props).put("isStopped", true);
                
                if (isTraceable(4)) {
                    trace("Marking connection stopped: connect=" + toString(ImaConstants.TS_Common));
                }
            }
            /*
             * Mark all sessions closed
             */
            Iterator <ImaSession> it = sessionsMap.values().iterator();
            while (it.hasNext()) {
                ImaSession sess = it.next();
                it.remove();
                sess.markClosed();
                sess = null;
            }

            if (isTraceable(4)) {
                trace("Connection is marked closed: connect=" + toString(ImaConstants.TS_Common));
            }
        }
    }

    /* ******************************************************
     * 
     * Implement trace on the connection object
     *  
     ****************************************************** */
    
    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#isTraceable(int)
     */
    public boolean isTraceable(int level) {
        if (!ImaTrace.dynamicTrace)
            return level <= traceImpl.traceLevel;
        return traceImpl.isTraceable(level);
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#trace(int, java.lang.String)
     */
    public void trace(int level, String message) {
        if (!ImaTrace.dynamicTrace) {
            if (level <= traceImpl.traceLevel)
                traceImpl.trace(message);
        } else {
            ((ImaTraceable)traceImpl).trace(level, message);
        }
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#trace(java.lang.String)
     */
    public void trace(String message) {
        traceImpl.trace(message);
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#traceException(java.lang.Throwable)
     */
    public JMSException traceException(Throwable ex) {
        traceImpl.traceException(ex);
        return (ex instanceof JMSException) ? (JMSException)ex : null;
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#traceException(int, java.lang.Throwable)
     */
    public JMSException traceException(int level, Throwable ex) {
        if (!ImaTrace.dynamicTrace) {
            if (level <= traceImpl.traceLevel) 
                traceImpl.traceException(ex);
            return (ex instanceof JMSException) ? (JMSException)ex : null;
        } else {
            return ((ImaTraceable)traceImpl).traceException(level, ex);
        }      
    }


    /*
     * @see javax.jms.Connection#createSession()
     */
    public Session createSession() throws JMSException {
        return createSessionInternal(false, Session.AUTO_ACKNOWLEDGE, false);
    }


    /*
     * @see javax.jms.Connection#createSession(int)
     */
    public Session createSession(int mode) throws JMSException {
        return createSessionInternal(mode==Session.SESSION_TRANSACTED, mode, false);
    }


    /*
     * @see javax.jms.Connection#createSharedConnectionConsumer(javax.jms.Topic, java.lang.String, java.lang.String, javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createSharedConnectionConsumer(Topic topic, String subname, String selector,
            ServerSessionPool pool, int max) throws JMSException {
        checkClosed();
        throw ImaJms.notImplemented("createSharedConnectionConsumer");
    }


    /*
     * (non-Javadoc)
     * @see javax.jms.Connection#createSharedDurableConnectionConsumer(javax.jms.Topic, java.lang.String, java.lang.String, javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createSharedDurableConnectionConsumer(Topic topic, String subname, String selector,
            ServerSessionPool pool, int max) throws JMSException {
        checkClosed();
        throw ImaJms.notImplemented("createSharedConnectionConsumer");
    }
    
    /* For use with RA */
    public void setTraceLevel(int level) {
        traceImpl.setTraceLevel(level%10);
    }
    
    public void setPrintWriter(PrintWriter writer) {
        traceImpl.setPrintWriter(writer);
    }
    
    /*
     * Error handling method invoked when no exception listener has been provided.
     */
    private void defaultOnException(JMSException ex) {
        if (ex instanceof ImaJmsException) {
            int err = ((ImaJmsException) ex).getErrorType();
            switch (err) {
            case 223:
                defaultCloseOnError("\"store capacity exceeded\"");
                break;
            case 231:
                defaultCloseOnError("\"out of memory\"");
                break;
            }
        }
    }
    
    /*
     * Method to close a connection when invoked from defaultOnException().
     */
    private void defaultCloseOnError(final String errString) {
        boolean closeInProgress = false;
        synchronized (pendingClose) {
            if (!pendingClose.isEmpty()) {
                closeInProgress = pendingClose.contains(clientid);
            }
            if (!closeInProgress)
                pendingClose.add(clientid);
        }
        if (!closeInProgress && !isClosed.get()) {
            trace(1, "No exception listener specified. Automatically closing connection for client ID " + clientid + " due to server " + errString + " error.");
            try {
                close();
            } catch (JMSException e) {
                traceException(1, e);
                trace(1, "Failed to close connection to server after receiving " + errString + " error.");
                markAllClosed(e);
            }
            synchronized (pendingClose) {
                pendingClose.remove(clientid);
            }
        }
    }
}   
