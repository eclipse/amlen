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

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.SelectorProvider;
import java.security.AccessController;
import java.security.PrivilegedActionException;
import java.security.PrivilegedExceptionAction;

import javax.jms.JMSException;

import com.ibm.ima.jms.ImaProperties;

/*
 * Implement a JMS tcp client.
 * 
 * The wire format of actions and messages consists of a 20 byte header, a set of header fields,
 * a map object, and a body object.
 * <p>
 * Following this header is a set of compressed fields.  The number of fields is specified by header_count.
 * The following field should be a map entry which can be null.  The following field is the body which
 * is always specified as a byte array.  The map and body are optional but the map must be present if the
 * body exists (it can be a one byte null map).
 * <p>
 * When send using the tcp client, there is a four byte length preceding the action.  
 */
public class ImaClientTcp extends  ImaClient implements  Runnable  {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    final  SocketChannel sock;
    private Selector writeSel;
    private Selector readSel;
    private final int recvBufferSize;
    
    public ImaClientTcp(ImaConnection connect, String host, int port, ImaProperties props) throws JMSException, IOException {
        super(new ImaPropertiesImpl(ImaJms.PTYPE_Connection));
        init(connect, host, port, props);
        /* Connect to the server */
        int recvSockBuffer = getInt("RecvSockBuffer", -1);
        synchronized (this) {
            sock = SocketChannel.open();
            /*
             * Do the connect requesting privilege.
             */
            try {
                AccessController.doPrivileged(new PrivilegedExceptionAction<Object>() {
                    public Object run() throws IOException {
                        sock.connect(ipaddr);
                        return null;
                    }
                });  
            } catch (PrivilegedActionException pae) {
                Throwable causex = pae.getCause();
                if (causex != null) {
                    if (causex instanceof RuntimeException)
                        throw (RuntimeException)causex;
                    if (causex instanceof IOException)
                        throw (IOException)causex;
                }
                throw new ImaSecurityException("CWLNC0047", pae, "The JMS client failed to establish a connection to MessageSight at host {0} because it does not have the required authorization in the Java security policy.", ipaddr);
            }
            sock.configureBlocking(false);
            writeSel = SelectorProvider.provider().openSelector();
            sock.register(writeSel, SelectionKey.OP_WRITE);
            readSel = SelectorProvider.provider().openSelector();
            sock.register(readSel, SelectionKey.OP_READ);
            sock.socket().setTcpNoDelay(true);
            if (recvSockBuffer > 0)
                sock.socket().setReceiveBufferSize(recvSockBuffer);
            if (ImaTrace.isTraceable(6)) {
            	int rcvBufSize = 0;
            	/* Circumvent JVM 6 problem with IPv6 on Windows */
            	try {
            		rcvBufSize = sock.socket().getReceiveBufferSize();
            	} catch (Exception ex) {}
                ImaTrace.trace("TCP handshake start: Server=" + host + ", Port="+port+
                        ", RecvSockBuffer=" + rcvBufSize + ", Properties=" + this.props);
            }
            recvBufferSize = getInt("RecvBufferSize", 8192);
        }
    }

    /*
     * Package private constructor for testing 
     */
    ImaClientTcp() {
        super(new ImaPropertiesImpl(ImaJms.PTYPE_Connection));
        sock = null;
        recvBufferSize = 8192;
//        ostream = null;
    }

    /*
     * Close the client
     */
    protected void closeImpl() {
        if (sock != null) {
            try { 
                sock.close();
                writeSel.close();
                readSel.close();
            } catch (Exception e) { 
            } 
        }
    }
    
    /*
     * Close the socket on shutdown
     * @see java.lang.Object#finalize()
     */
    protected void finalize() {
        close();
    }
    
    
    /*
     * Send an action
     */
    public synchronized void send(ByteBuffer bb, boolean flush) throws IOException {    
        int len = bb.limit()-4;
        if ((len == 1) && ImaTrace.isTraceable(8)) {
            ImaTrace.trace("sendPingReply: connection="+connect.toString(ImaConstants.TS_All));
        }
        bb.putInt(0, len);        
        do {
//            if (bb.position() != 0)
//                System.err.println("before write: " + bb.position() + ' ' + bb.remaining() + ' ' + bb.limit());
            sock.write(bb);
//            if (bb.remaining() != 0)
//                System.err.println("after write: " + bb.position() + ' ' + bb.remaining() + ' ' + bb.limit());
            if (bb.remaining() == 0)
                return;
            writeSel.select();
            if (isClosed.get())
                return;
        } while (true);
    }
    
    /*
     * Run the receive thread
     * @see java.lang.Runnable#run()
     */
    public void run() {
        Exception e = null;
        ByteBuffer result;
        synchronized (this) {
            result = ByteBuffer.allocateDirect(recvBufferSize);            
        }
        for(;;) {
            int datalen = 0;
            try {
                readSel.select();
                if (isClosed.get()) {
                    break;
                }
                int rc = sock.read(result);
                if ( rc < 0) {
                    e = new ImaIOException("CWLNC0045", "The client failed because a socket connection read error ({0}) occurred when receiving data from MessageSight.  This can occur if MessageSight closes the connection (for example, during shut down), or if MessageSight is stopped.  It can also occur due to network problems.",rc);
                    break;
                }
//                System.err.println("rc = " + rc);
                if (rc == 0)
                    continue;
                if (result.position() < 4)
                    continue;
            } catch (Exception ioe) {
                e = ioe;
                break;
            }
            lastPingReceived = System.currentTimeMillis();
            result.flip();
            while (result.remaining() > 4) {
                result.mark();
                datalen = result.getInt();
                if ((datalen < ImaAction.HEADER_LENGTH) && (datalen != 1)) {
                    if (ImaTrace.isTraceable(2)) {
                    	ImaTrace.trace("Data received from server with invalid length: connect=" + 
                    			connect.toString(ImaConstants.TS_All) +
                    			" datalen=" + datalen);
                    }
                    e = new ImaIOException("CWLNC0046", "The client failed because it received data from the server that was not the correct length ({0}). This can happen if MessageSight closes the connection (for example, during shut down), or if MessageSight is down.  It can also occur due to network problems.", datalen);
                    break;
                }
                if (result.remaining() < datalen) {
                    result.reset();
                    break;
                }
                result.reset();
                ByteBuffer bb = result.slice();
                bb.limit(datalen+4);
                processData(bb);
                result.position(result.position()+datalen+4);
//                if (result.remaining() > 0)
    //                continue;
            }
            if (e != null)
                break;
            if (result.remaining() == 0) {
                result.clear();
                continue;
            }
            if ((datalen+4) > result.capacity()) {
                ByteBuffer bb = result.slice();
                result = ByteBuffer.allocateDirect(datalen+1024);
                result.put(bb);
            } else {
                if (result.position() > 0) {
                    ByteBuffer bb = result.slice();
                    result.clear();
                    result.put(bb);
                } else {
                    result.position(result.limit());
                    result.limit(result.capacity());
                }
            }
        }
        
        /*
         * Process the exception which terminated us
         */
       	onError(e);
        
        try {
            readSel.close();
            writeSel.close();
        } catch (IOException ex) {
        }
        if(ImaTrace.isTraceable(6)) {
            ImaTrace.trace("End ClientTcpThread: connection="+connect.toString(ImaConstants.TS_All));
        }           
    }

}
