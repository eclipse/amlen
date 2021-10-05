/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.security.PrivilegedActionException;
import java.security.PrivilegedExceptionAction;
import java.util.Iterator;
import java.util.TimerTask;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.zip.CRC32;

import javax.jms.JMSException;
import javax.jms.Queue;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaAction.ItemTypes;

/*
 * A client is able to send and receive actions to the server.
 * 
 * When a client is created, it must start the communications with the server
 * by sending the initConnection action.  This contains the version of the
 * jms protocol used by this client.  The server responds with its version and
 * the current time.
 * 
 * All further communications are initiated by the jms logic.
 * 
 * It is the responsibility of the client to implement whatever methods are required
 * to receive messages and invoke the complete method of the Connection.
 */
abstract  class ImaClient extends ImaReadonlyProperties implements Runnable {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    private static final long PING_INTERVAL = 30000;
    protected final AtomicBoolean      isClosed         = new AtomicBoolean(false);
	private static final AtomicInteger COUNTER = new AtomicInteger(0);
	ImaConnection connect;
	String host;
	int    port;
	int    server_version;
	long   server_time;
	long   client_time;
	InetSocketAddress ipaddr;
	boolean forTest = false;
	StringBuffer strbuf = new StringBuffer();
	public static final int ClientVersion = 1;
	private final ByteBuffer pingBB = ByteBuffer.allocateDirect(5);
    protected long  lastPingReceived = 0;       
    private String  threadName = "";
    
	/*
	 * Constructor for common client code
	 */
	ImaClient(ImaProperties props) {
		super(props);
		pingBB.mark();
		pingBB.put(4,(byte)0);
	}

	static final int	CLIENT_ID_POS	= ImaAction.ITEM_POS;

	/*
	 * Close the client
	 */
    final boolean close() {
        if (isClosed.compareAndSet(false, true)) {
            if (ImaTrace.isTraceable(7)) {
                ImaTrace.trace("ImaClient is closing: thread=" + threadName + " connect="
                        + connect.toString(ImaConstants.TS_Common));
            }
            closeImpl();
            return true;
        }
        return false;
	}
	abstract protected void closeImpl();

	/*
	 * Send an action to the client
	 * @param  act   The action to send
	 */
	abstract void send(ByteBuffer bb, boolean flush) throws IOException ;

	/*
	 * Find producer from producer ID
	 */
    ImaProducer getMessageProducer(int producer_id) {
		Integer id = new Integer(producer_id);
		Iterator <ImaSession> sess_it = this.connect.sessionsMap.values().iterator();
		while (sess_it.hasNext()) {
			ImaSession sess = sess_it.next();
            ImaProducer producer = sess.producerList.get(id);
			if (producer != null)
				return producer;
		}
		return null;
	}

	/*
	 * Initialize the client
	 */
	void init(ImaConnection connect, String host, int port, ImaProperties props) throws JMSException {
		this.props.putAll(((ImaPropertiesImpl)props).props);
		this.host = host;
		this.port = port;
		this.connect = connect;

		try {
		    final String xhost = host;
		    final int    xport = port;
            ipaddr = (InetSocketAddress) AccessController.doPrivileged(new PrivilegedExceptionAction<Object>() {
                public Object run() {
                    return new InetSocketAddress(xhost, xport);
                }
            });
		} catch (PrivilegedActionException pae) {
            ImaSecurityException sx = new ImaSecurityException("CWLNC0027", pae, "The client failed to connect to MessageSight at the specified host and port ({0}:{1}) because the client does not have the required Java privilege.", host, port);
            ImaTrace.traceException(2, sx);
            throw sx; 
		} 
		catch(IllegalArgumentException ex) {
			ImaJmsExceptionImpl iex = new ImaJmsExceptionImpl("CWLNC0028", ex, "The client failed to connect to MessageSight because either the specified host is null or the specified port number is not in the range 0 to 65535: Host={0} Port={1}.", host, port);
			ImaTrace.traceException(2, iex);
			throw iex;
		}
		if (ipaddr.isUnresolved()) {
			ImaIllegalArgumentException iex = new ImaIllegalArgumentException("CWLNC0029", "The client failed to connect to MessageSight because the host {0} could not be resolved.",ipaddr.getHostName());
			ImaTrace.traceException(2, iex);
			throw iex;
		}
	}

	/*
	 * Start the client
	 */
	void startClient() throws JMSException {
		/* Start a thread to receive messages */	
	    threadName = this.getClass().getSimpleName()+'.'+COUNTER.getAndIncrement();
	    final String thisThreadName = threadName;
	    final ImaClient thisClient = this;
		AccessController.doPrivileged(new PrivilegedAction<Object>() {
		    public Object run() {
		        Thread th = new Thread(thisClient, thisThreadName);
		        th.start();
		        return null;
		    }
		});     

		/* Send the init action */
        ImaAction act = new ImaConnectionAction(ImaAction.initConnection,this);
        act.outBB.putInt(CLIENT_ID_POS, ImaConnection.Client_version);
        act.outBB = ImaUtils.putStringValue(act.outBB, "ImaJMS");
        act.outBB = ImaUtils.putStringValue(act.outBB, ImaConstants.Client_build_id);
        int timeout;
        if (connect.disableTimeout != 0) {
        	/* Allow negative to be set for testing, this will fail verification */
        	if (connect.disableTimeout <= -30)
        		timeout = -90;
        	else
        		timeout = 0;
        } else {
        	timeout = 90;
        }
        act.outBB = ImaUtils.putIntValue(act.outBB, timeout);
        act.setHeaderCount(3);
        act.request();
        
        /* Read server response */
        int headerCount = act.inBB.get(ImaAction.HDR_COUNT_POS);
        if (headerCount >= 2) {
            server_version = ImaUtils.getInteger(act.inBB);
            connect.metadata.setProviderVersion(server_version);
        }
        if (headerCount >= 3) {
            Object obj = ImaUtils.getObjectValue(act.inBB);
            if (obj instanceof Long)
                server_time = (Long) obj;
        }
        String server_buildID = "";
        if (headerCount >= 4) {
            Object obj = ImaUtils.getObjectValue(act.inBB);
            if (obj != null) {
                server_buildID = " "+obj;
                connect.metadata.setProviderBuild(server_buildID);
            }
        }
		connect.instanceHash = instanceHash(server_time);
		client_time = System.currentTimeMillis() * 1000000;
		setPingTimer();
		if (ImaTrace.isTraceable(3)) {
		    synchronized (ImaMessage.class) {
	            ImaTrace.trace("Connected to messaging server: connect="+connect.toString(ImaConstants.TS_All) +
	                            "\n    client_version=" + showVersion(ImaConnection.Client_version) + 
	                            " " + ImaConstants.Client_build_id +
	                            " server_version=" + showVersion(server_version) + server_buildID +
	                            " client_time=" + ImaMessage.iso8601.format(client_time/1000000) + 
	                            " server_time=" + ImaMessage.iso8601.format(server_time/1000000));
		    }
		}
	}
	
	/*
	 * Set up the ping timer for a connection
	 */
	final protected void setPingTimer() {
		/* 
		 * If DisableTimeout is set we will disable the client ping.
		 */
		if (connect.disableTimeout != 0) {
			if (ImaTrace.isTraceable(7)) {
				ImaTrace.trace("Disable client to server ping: connect=" + connect.toString(ImaConstants.TS_All));
			}
			return;
		}	
		if (ImaTrace.isTraceable(7)) {
			ImaTrace.trace("Set up client to server ping timer: connect=" + connect.toString(ImaConstants.TS_Common));
		}
        final TimerTask tt = new TimerTask() {
            public void run() {
                if (isClosed.get()) {
                    cancel();
                    return;
                }
                long currentTime = System.currentTimeMillis();
                long delay = currentTime - lastPingReceived;
                if (delay > (PING_INTERVAL*3)) {
                    ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0224", 
                    		"The connection to MessageSight was closed after {0} seconds because MessageSight was not responding. Connection = {1}.  This can occur if MessageSight is stopped, or if the network is down.", delay/1000, connect.toString(ImaConstants.TS_All));
                  	ImaTrace.traceException(3, jex);
                    onError(jex);
                    cancel();
                    return;
                }
                try {
                    send(pingBB,true);
                    pingBB.reset();
                } catch (IOException e) {
                    onError(e);
                    return;
                }
            }
        };
        lastPingReceived = System.currentTimeMillis();
        ImaConnection.jmsClientTimer.schedule(tt, PING_INTERVAL, PING_INTERVAL);
	}
	
	/*
	 * Process a ping request from the server
	 */
	final void processPingRequest() {
		/* Special code for test, must not be used in production */
		if (connect.disableTimeout < 0) {
			ImaTrace.trace(6, "ping from server ignored: connection=" + connect.toString(ImaConstants.TS_Common));
			return;
		}	
        if (ImaTrace.isTraceable(7)) {
            ImaTrace.trace("processPingRequest: connection="+connect.toString(ImaConstants.TS_Common));
        }
		final TimerTask tt = new TimerTask() {
			public void run() {
                if (isClosed.get())
			        return;
                try {
                    ByteBuffer bb = ByteBuffer.allocate(5);
                    bb.put(4,(byte)1);
                    send(bb,true);
                } catch (IOException e) {
                    onError(e);
                    return;
                }
			}
		};
		ImaConnection.jmsClientTimer.schedule(tt, 0);
	}
	
	
	/*
	 * Process data from the server
	 */
	final void processData(ByteBuffer data) {
		if (data.limit() == 5) {
		    byte b = data.get(4);
		    if (b == 0)
		        processPingRequest();
			return;
		}
		int messageType = data.get(ImaAction.ITEM_TYPE_POS);

		if (data.get(ImaAction.ACTION_POS) == ImaAction.raiseException) {
			/*
			 * Currently only send message failures are reported.
			 * Header 1 is the rc value.
			 * Header 2 is the session ID
			 * Header 3 is the consumer or producer ID
			 * Header 3 is domain type (set to 0 when producer is known)
			 * Header 4 is domain name (set to null when producer is known)
			 * Header 5 is server IP
			 * Header 6 is server port
			 * Header 7 is the endpoint name
			 */

			try {
				JMSException e = null;
				int headerCount = data.get(ImaAction.HDR_COUNT_POS);
				data.position(ImaAction.DATA_POS);
				int rc = ImaUtils.getInteger(data);
				if (headerCount < 2) {
					return;
				}

				/* Get session Id */
				ImaUtils.getInteger(data);

				String domainString = null;
				String destinationName = null;
				Integer producerId = null;
				producerId = ImaUtils.getInteger(data);
				if (producerId != 0) {
                    final ImaProducer prod = getMessageProducer(producerId);
					if (prod != null) {
						domainString = (prod.domain == ImaJms.Topic)?"topic":"queue";
						destinationName = (prod.domain == ImaJms.Topic)?((Topic)prod.dest).getTopicName():((Queue)prod.dest).getQueueName();
					} else {
						boolean connclosed = ((ImaProperties)this.connect).getBoolean("isClosed", false);
						if (connclosed && ImaTrace.isTraceable(3)) {
							ImaTrace.trace("Cannot find domain or destination for producer "+producerId+" because the connection is closed.");
						}
						domainString = "unknown domain";
						destinationName = "unknown destination";
					}
				} else {
					int domain = ImaUtils.getInteger(data);
					if (domain != 0) {
					    domainString = (domain == ImaJms.Topic)?"topic":"queue";
					    destinationName = (String)ImaUtils.getObjectValue(data);
					} else {
						domainString = "unknown domain";
						destinationName = "unknown destination";
					}
				}

				switch (rc) {
				case ImaReturnCode.IMARC_MsgTooBig:
					e = new ImaJmsExceptionImpl("CWLNC0215", "A call to send() or publish() to {0} {1} from producer {2} failed because the message size exceeds the maximum size permitted by MessageSight endpoint {3}.",
							domainString, destinationName, (producerId != 0) ? producerId:"\"null\"", this.host+":"+this.port);
					break;
				case ImaReturnCode.IMARC_DestinationFull:
					e = new ImaJmsExceptionImpl("CWLNC0218", "A call to send() or publish() to {0} {1} from producer {2} failed because the destination is full.",
							domainString, destinationName, (producerId != 0) ? producerId:"\"null\"");
					break;
				case ImaReturnCode.IMARC_DestNotValid:
					e = new ImaJmsExceptionImpl("CWLNC0219", "A call to send() or publish() to {0} {1} from producer {2} failed because the destination is not valid.",
							domainString, destinationName, (producerId != 0) ? producerId:"\"null\"");
					break;
	            case ImaReturnCode.IMARC_AllocateError:
	                e = new ImaJmsExceptionImpl("CWLNC0231", "A call to send() or publish() to {0} {1} from producer {2} failed because MessageSight is out of memory.",
	                        domainString, destinationName, (producerId != 0) ? producerId:"\"null\"");
	                break;
				case ImaReturnCode.IMARC_ServerCapacity:
					e = new ImaJmsExceptionImpl("CWLNC0223", "A call to send() or publish() a persistent JMS message to {0} {1} from producer {2} failed because MessageSight could not store the message.",
							domainString, destinationName, (producerId != 0) ? producerId:"\"null\"", rc);
					break;
	            case ImaReturnCode.IMARC_BadSysTopic:
	                e = new ImaJmsExceptionImpl("CWLNC0225", "A request to create a message producer or a durable subscription, or a request to publish a message failed because the destination {0} is a system topic.",
	                        destinationName);
	                break;
				default:
					e = new ImaJmsExceptionImpl("CWLNC0216", "A call to send() or publish() to {0} {1} from producer {2} failed with MessageSight return code = {3}.",
							domainString, destinationName, (producerId != 0) ? producerId:"\"null\"", rc);
					break;
				}

				connect.raiseException(e);

			} catch (JMSException e1) {
				connect.raiseException(e1);
			}

			return;
		}

		/*
		 * Process a message
		 */
		if (messageType == ItemTypes.None.value()) {
			int consumer_id = data.getInt(ImaAction.ITEM_POS);
			ImaConsumer consumer = connect.consumerMap.get(Integer.valueOf(consumer_id));
			if (consumer != null) {
				try {
					consumer.handleMessage(data);
				} catch (JMSException e) {
					if (ImaTrace.isTraceable(3)) {
						ImaTrace.trace("Exception in process data: consumer=" + 
								 consumer.toString(ImaConstants.TS_All));
						ImaTrace.traceException(e);
					}    
					connect.raiseException(e);
				}
				return;
			}
			ImaTrace.trace(3, "Message found for unknown consumer ID: id=" + consumer_id + " connect=" +
						connect.toString(ImaConstants.TS_Common));
			return;
				
		} else {
			long respid = data.getLong(ImaAction.MSG_ID_POS);
			ImaAction act = ImaAction.getAction(respid);
			/* complete the action */
			if (act != null) {
				act.complete(data);				
			} else {
				ImaTrace.trace(3, "Reply message with unknown response id was received: id=" + respid + " connect=" +
						connect.toString(ImaConstants.TS_Common) + "msg header: action=" +
						data.get(ImaAction.ACTION_POS) + " itemType=" + data.get(ImaAction.ITEM_TYPE_POS) +
						" item=" + data.get(ImaAction.ITEM_POS));				
			}
		}

	}


	/*
	 * Convert a version number to a version string
	 */
	static String showVersion(int version) {
		return ""+(version/1000000)+'.'+((version/10000)%100)+'.'+((version/100)%100)+'.'+(version%100);	
	}


	/*
	 * Convert a version string to an integer
	 */
	static int makeVersion(String s) {
		int version = 0;
		int val = 0;
		for (int i=0; i<s.length(); i++) {
			char ch = s.charAt(i);
			if (ch=='.') { 
				version = version*100 + val;
				val = 0;
			} else if (ch>='0' && ch<='9') {
				val = val*10  + (ch-'0');
			}
		}
		version = version*100+val;
		return version;
	}

	/*
	 * Create a hash value for a connection
	 */
	static byte [] instanceHash(long key) {
		byte [] k = new byte [8];
		byte [] h = new byte [4];
		k[0] = (byte)(key>>56);
		k[1] = (byte)(key>>48);
		k[2] = (byte)(key>>40);
		k[3] = (byte)(key>>32);
		k[4] = (byte)(key>>24);
		k[5] = (byte)(key>>16);
		k[6] = (byte)(key>>8);
		k[7] = (byte)key;
		CRC32 crc = new CRC32();
		crc.update(k);
		long crcval = crc.getValue();
		if (crcval == 0)
			crcval = 0xdeadbeef;
		h[0] = (byte)(crcval>>24);
		h[1] = (byte)(crcval>>16);
		h[2] = (byte)(crcval>>8);
		h[3] = (byte)(crcval);
		return h;
	}

	/*
	 * Get a big-endian int
	 */
	static protected int getIntBig(byte[] buff, int offset) {
		return ((buff[offset]&0xff)<<24)   |
				((buff[offset+1]&0xff)<<16) |
				((buff[offset+2]&0xff)<<8)  |
				(buff[offset+3]&0xff);    	
	}

	/*
	 * Forward the raise exception to the connection exception listener
	 */
    void onError(Exception e) {
        if ((e != null) && (close())) {
			connect.onError(e);
		} else {
	        if((e != null) && ImaTrace.isTraceable(9)) {
	            ImaTrace.trace("client.onError: connection="+connect.toString(ImaConstants.TS_All) + " reason=" + e);
	        }		    
		}
	}
}
