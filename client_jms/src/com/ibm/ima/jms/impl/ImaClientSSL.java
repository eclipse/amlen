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
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Constructor;
import java.nio.ByteBuffer;
import java.security.AccessController;
import java.security.PrivilegedActionException;
import java.security.PrivilegedExceptionAction;
import java.util.HashSet;
import java.util.StringTokenizer;

import javax.jms.JMSException;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

import com.ibm.ima.jms.ImaProperties;


/*
 * Implement a JMS tcp TLS client.
 * 
 * The wire format of actions and messages consists of a 16 byte header, a set of header fields,
 * a map object, and a body object.
 * <p>
 * <pre> 
 * byte    action
 * byte    flags         
 * byte    header_count
 * byte    body_type
 * int32   session_id
 * int64   response_id or message_id
 * </pre>
 * Following this header is a set of compressed fields.  The number of fields is specified by header_count.
 * The following field should be a map entry which can be null.  The following field is the body which
 * is always specified as a byte array.  The map and body are optional but the map must be present if the
 * body exists (it can be a one byte null map).
 * <p>
 * When send using the tcp client, there is a four byte length preceding the action.  
 */
public class ImaClientSSL extends ImaClient implements Runnable {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    final SSLSocket    sock;
    final OutputStream ostream;
    private byte[]     sendBuffer = new byte[1024];
    private final int  recvBufferSize;
    int                recvSockBuffer;

    /*
     * Our default cipher list.
     * All of these are supported by MessageSight, but they might not be supported 
     */
    final static String defaultCiphers = 
            "TLS_EMPTY_RENEGOTIATION_INFO_SCSV," + 
            "TLS_RSA_WITH_AES_128_GCM_SHA256," + 
            "TLS_RSA_WITH_AES_256_GCM_SHA384," + 
            "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA," +   
            "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA," +
            "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256," +
            "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA256," +
            "TLS_DHE_RSA_WITH_AES_128_CBC_SHA," +
            "TLS_DHE_DSS_WITH_AES_128_CBC_SHA," +
            "TLS_RSA_WITH_AES_128_CBC_SHA256," +
            "TLS_RSA_WITH_AES_128_CBC_SHA," + 
            "TLS_RSA_WITH_AES_256_CBC_SHA256";
       
    /*
     * Client constructor
     */
    public ImaClientSSL(ImaConnection connect, String host, int port, ImaProperties props) throws JMSException,
            IOException, PrivilegedActionException {
        super(new ImaPropertiesImpl(ImaJms.PTYPE_Connection));
        init(connect, host, port, props);
        /* Connect to the server */
        recvSockBuffer = getInt("RecvSockBuffer", -1);
        synchronized (this) {
            PrivilegedExceptionAction<SSLSocket> pa = new PrivilegedExceptionAction<SSLSocket>() {
                public SSLSocket run() throws JMSException, IOException {
                    SSLSocketFactory sslsocketfactory = getSSLSocketFactory();
                    return (SSLSocket) sslsocketfactory.createSocket();
                }
            };

            sock = AccessController.doPrivileged(pa);

            /*
             * Set allowed security protocols Allowed protocol names: "SSLv3, TLSv1, TLSv1.1, TLSv1.2"
             */
            String protocols;
            if (exists("SecurityProtocols")) {
                protocols = getString("SecurityProtocols");
            } else {
                protocols = "TLSv1,TLSv1.1,TLSv1.2";
            }  
            if (protocols.length() > 0) {
                StringTokenizer st = new StringTokenizer(protocols, ",");
                String[] supportedProtocols = sock.getSupportedProtocols();
                HashSet<String> allowed = new HashSet<String>();
                String[] requested = new String[supportedProtocols.length];
                int   counter = 0;
                for (int i = 0; i < supportedProtocols.length; i++) {
                    allowed.add(supportedProtocols[i]);
                }
                while (st.hasMoreTokens()) {
                    String p = st.nextToken();
                    if (allowed.contains(p))
                        requested[counter++] = p;
                }
                if (counter == 0) {
                    StringBuffer error = new StringBuffer("");
                    for (int i = 0; i < supportedProtocols.length; i++) {
                        error.append(supportedProtocols[i]).append(' ');
                    }
                    ImaIllegalArgumentException iex = new ImaIllegalArgumentException("CWLNC0102", "The client failed to establish a secure connection to MessageSight because none of the protocols requested by the JVM is supported. Supported protocols are: {0}",error.toString());
                    ImaTrace.traceException(2, iex);
                    throw iex;
                }
                String[] sa = new String[counter];
                System.arraycopy(requested, 0, sa, 0, counter);
                if (ImaTrace.isTraceable(8)) {
                    StringBuffer error = new StringBuffer("Set enabled secuirty protocols: ");
                    for (int i=0; i<sa.length; i++) {
                        error.append(requested[i]).append(", ");
                    }
                    ImaTrace.trace(""+error);
                }
                sock.setEnabledProtocols(sa);
            }
           
            /*
             * Set allowed Cipher Suites Allowed protocol names:
             * http://docs.oracle.com/javase/7/docs/technotes/guides/security/StandardNames.html#ciphersuites
             * 
             * If CipherSuites is not specified, use our default list of ciphers which we know is (or might be)
             * supported on the MessageSight server.  If the CipherSuites is specified but has no entries use
             * the JVM default.  If one or more ciphers is specified, check if it is supported by the JVM and
             * add it to the list if it is.
             */
            String suites = getString("CipherSuites");
            if (suites == null) 
                suites = defaultCiphers;
            StringTokenizer st = new StringTokenizer(suites, " ,");
            if (st.countTokens() > 0) {
                String[] supportedSuites = sock.getSupportedCipherSuites();
                HashSet<String> allowed = new HashSet<String>();
                String[] requested = new String[supportedSuites.length];
                int counter = 0;
                for (int i = 0; i < supportedSuites.length; i++) {
                    String s = supportedSuites[i];
                    /* Do not allow low quality ciphers */
                    if (s.indexOf("anon")<0 && s.indexOf("NULL")<0 && s.indexOf("EXPORT")<0 && s.indexOf("KBR")<0)
                        allowed.add(s);
                }
                
                /*
                 * Loop thru the specified ciphers, fix the prefix to match the JVM
                 */
                while (st.hasMoreTokens()) {
                    String p = st.nextToken();
                    if (allowed.contains(p)) {
                        requested[counter++] = p;
                    } else {
                        if (p.startsWith("SSL_")) {
                            p = "TLS_"+p.substring(4);
                            if (allowed.contains(p))
                                requested[counter++] = p;
                        } else if (p.startsWith("TLS_")) {
                            p = "SSL_"+p.substring(4);
                            if (allowed.contains(p))
                                requested[counter++] = p;
                        }
                    }
                }
                if (counter == 0) {
                    StringBuffer error = new StringBuffer("");
                    for (int i = 0; i < supportedSuites.length; i++) {
                        String s = supportedSuites[i];
                        /* Match what we allow */
                        if (s.indexOf("anon")<0 && s.indexOf("NULL")<0 && s.indexOf("EXPORT")<0 && s.indexOf("KBR")<0)
                            error.append(supportedSuites[i]).append(' ');
                    }
                    ImaIllegalArgumentException iex = new ImaIllegalArgumentException("CWLNC0103", "The client failed to establish a secure connection to MessageSight because none of the cipher suites requested by the JVM is supported. Supported cipher suites are: {0}", error.toString());
                    ImaTrace.traceException(2, iex);
                    throw iex;
                }
                String[] sa = new String[counter];
                System.arraycopy(requested, 0, sa, 0, counter);
                if (ImaTrace.isTraceable(8)) {
                    StringBuffer error = new StringBuffer("Set enabled cipher suites: ");
                    for (int i=0; i<sa.length; i++) {
                        error.append(sa[i]).append(", ");
                    }
                    ImaTrace.trace(""+error);
                }
                sock.setEnabledCipherSuites(sa);
            }    
            
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
                throw new ImaSecurityException("CWLNC0047", pae, "The JMS client failed to establish a connection to MessageSight at host {0} because it does not have the required authorization in the Java security policy." + ipaddr);
            }
            
            sock.setTcpNoDelay(true);

            if (recvSockBuffer > 0)
                sock.setReceiveBufferSize(recvSockBuffer);
            sock.setUseClientMode(true);

            if (ImaTrace.isTraceable(6)) {
                int rcvBufSize = 0;
                /* Circumvent JVM 6 problem with IPv6 on Windows */
                try {
                    rcvBufSize = sock.getReceiveBufferSize();
                } catch (Exception ex) {
                }
                ImaTrace.trace("SSL/TLS handshake start: Server=" + host + ", Port=" + port + ", RecvSockBuffer="
                        + rcvBufSize);
            }

            sock.startHandshake();

            ostream = sock.getOutputStream();
            recvBufferSize = getInt("RecvBufferSize", 8192);

            if (ImaTrace.isTraceable(6)) {
                ImaTrace.trace("SSL/TLS handshake completed: connect=" + connect.toString(ImaConstants.TS_All));
            }
        }
    }

    /*
     * Package private constructor for testing
     */
    ImaClientSSL() {
        super(new ImaPropertiesImpl(ImaJms.PTYPE_Connection));
        sock = null;
        recvBufferSize = 8192;
        ostream = null;
    }

    /*
     * Close the client
     */
    protected void closeImpl() {
        if (sock != null) {
            try {
                sock.close();
            } catch (IOException e) {
            }
        }
    }

    /*
     * Close the socket on shutdown
     * 
     * @see java.lang.Object#finalize()
     */
    protected void finalize() {
        close();
    }

    /*
     * Send an action
     */
    public synchronized void send(ByteBuffer bb, boolean flush) throws IOException {

        int len = bb.limit() - 4;
        if ((len == 1) && ImaTrace.isTraceable(8)) {
            ImaTrace.trace("sendPingReply: connection=" + connect.toString(ImaConstants.TS_All));
        }
        bb.putInt(0, len);
        if (sendBuffer.length < bb.limit())
            sendBuffer = new byte[bb.limit()];
        bb.get(sendBuffer, 0, bb.limit());
        ostream.write(sendBuffer, 0, bb.limit());
    }

    /*
     * Run the receive thread
     * 
     * @see java.lang.Runnable#run()
     */
    public void run() {
        Exception e = null;
        ByteBuffer result = ByteBuffer.allocate(recvBufferSize * 2);
        InputStream is;
        byte[] recvBuffer = new byte[recvBufferSize];
        synchronized (this) {
            try {
                is = sock.getInputStream();
            } catch (IOException e1) {
            	if (ImaTrace.isTraceable(1)) {
            		ImaTrace.trace(1, "Socket connection input stream error: " + e1.getMessage());
            	}
                e = e1;
                onError(e);
                close();
                return;
            }
        }
        for (;;) {
            int datalen = 0;
            try {
                if (isClosed.get())
                    return;
                int rc = is.read(recvBuffer);
                if (rc < 0) {
                    e = new ImaIOException("CWLNC0045", "The client failed because a socket connection read error ({0}) occurred when receiving data from MessageSight.  This can occur if MessageSight closes the connection (for example, during shut down), or if MessageSight is stopped.  It can also occur due to network problems.", rc);
                    break;
                }
                // System.err.println("rc = " + rc);
                if (rc == 0)
                    continue;
                result.put(recvBuffer, 0, rc);
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
                        StringBuffer strbuf = new StringBuffer();
                        strbuf.append("Received message with invalid length: connect=");
                        strbuf.append(connect.toString(ImaConstants.TS_Common)).append(" ");
                        strbuf.append("datalen=").append(datalen).append(" ");
                        strbuf.append("headerlen=").append(ImaAction.HEADER_LENGTH).append(" ");
                        strbuf.append("result=").append(result.toString());
                        ImaTrace.trace(strbuf.toString());
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
                bb.limit(datalen + 4);
                processData(bb);
                result.position(result.position() + datalen + 4);
            }
            if (e != null)
                break;
            if (result.remaining() == 0) {
                result.clear();
                continue;
            }
            if ((datalen + 4) > result.capacity()) {
                ByteBuffer bb = result.slice();
                result = ByteBuffer.allocateDirect(datalen + 1024);
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
        if (ImaTrace.isTraceable(6)) {
            ImaTrace.trace("End ClientSSLThread: connection=" + connect.toString(ImaConstants.TS_All));
        }
    }

    /*
     * Get the TLS socket factory
     */
    private final SSLSocketFactory getSSLSocketFactory() throws JMSException {
        String className = props.getString("SecuritySocketFactory");
        if (className == null) 
            className = (SSLSocketFactory.getDefault()).getClass().getName();
        if (className != null) {
            Object result;
            try {
                ClassLoader cl = Thread.currentThread().getContextClassLoader();
                Class<?> factoryClass = cl.loadClass(className);
                String param = props.getString("SecurityConfiguration");
                if (param != null) {
                    // get constructor taking a String
                    Constructor<?> con = factoryClass.getConstructor(String.class);
                    // create a SSLSocketFactory
                    result = con.newInstance(param);
                } else {
                    result = factoryClass.newInstance();
                }
            } catch (Exception e) {
                /* 
                 * Loading the class by name is not guaranteed to work, especially when running
                 * a security manager.  Therefore use the method which is guaranteed to return a
                 * result, even if the result is not usable.
                 */
                result = SSLSocketFactory.getDefault();
            }
            if (result instanceof SSLSocketFactory) {
                if (ImaTrace.isTraceable(6)) {
                    ImaTrace.trace("Using security socket factory: " + result);
                }
                return (SSLSocketFactory) result;
            }
            throw new ImaJmsExceptionImpl("CWLNC0101", "The client failed to establish a secure connection to MessageSight because the security socket factory class {0} does not implement the javax.net.ssl.SSLSocketFactory interface.", className);
        }
        /* This should never happen.  We could return null but I think a runtime exception is better for this case. */
        throw new ImaSecurityException("CWLNC0104", "The client failed to establish a secure connection to MessageSight because no security socket factory was found. This can happen if there is no default security socket factory available to the JVM.");
    }
}
