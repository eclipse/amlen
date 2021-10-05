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
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;

public class MQTTSimpleTest implements Runnable {
    String host;
    int    port;
    Socket sock;
    InetSocketAddress ipaddr;
    OutputStream	ostream;
    int   frame;
    private boolean isClosed = false; 
    
    public static final int ClientVersion = 1;
    public static final int FRAME_WS = 1;
    public static final int FRAME_MQTT = 2;
    
	public void connect(String host, int port) throws IOException {
        System.out.println("new client: "+this.host+":"+this.port);
        
        try {
            ipaddr = new InetSocketAddress(host, port);
        } catch(Exception ex) {
        	throw new IllegalArgumentException("IP address not valid: " + host + ":" + port, ex);
        }
		if (ipaddr.isUnresolved()) 
			throw new IllegalArgumentException(ipaddr.getHostName());
        	/* Connect to the server */
        	synchronized (this) {
                sock = new Socket();
    		    sock.setTcpNoDelay(true);
    		    sock.connect(ipaddr);
    		    ostream = sock.getOutputStream();
			}
		    
		    /* Start a thread to receive messages */
		    Thread thread = new Thread(this);
		    thread.start();
		    
		    /* Send the init action */


	}

	

	/*
	 * Close the client
	 */
	public void close() {
		if (isClosed)
			return;
		isClosed = true;
		if (sock != null)
		    try { sock.close(); } catch (IOException e) { }
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
	public void send(byte[] buf, int start, int len, int cmd) throws IOException {
		synchronized (this) {
		//    int len = act.bodylen-4;
		//    act.putIntBig(0, len);
		//    ostream.write(act.body, 0, act.bodylen);
		}
	}
	
	public void receive(byte [] buf, int start, int len, int cmd) {
		
	}
	
	/*
	 * Run the receive thread
	 * @see java.lang.Runnable#run()
	 */
	public void run() {
		int pos;
		int readlen;
		int len;
		int len1;
	    byte[] result = new byte[32 * 1024];
		BufferedInputStream in;
		
		try {
			in = new BufferedInputStream(sock.getInputStream());
			for(;;) {
			    int cmd = in.read();
			    len = in.read();
			    if (frame == FRAME_WS) {
			        if (len > 125) {
			    	    len = in.read()<<8 + in.read();
			        }
			    } else {
			    	if ((len & 0x80) != 0) {
			    	    len1 = len;
			    	    len = 0;
			            while ((len1 & 0x80) != 0) {
			        	    len = (len<<7) + (len1&0x7f);
			        	    len1 = in.read();
			            }
			            len = (len<<7) + len1;
			        }
			    }
			    if (len > result.length) {
			    	result = new byte[len];
			    }
			    pos = 0;
			    while (pos < len) {
			    	readlen = in.read(result, pos, len-pos);
				    if (readlen < 0) {
					    throw new IOException("Socket connection read error after reading: " + pos + " bytes.");  
				    } else {
					    pos += readlen;
				    }
			    }
			    receive(result, 0, len, cmd);
		    }    
			
		} catch (IOException e1) { }
		close();
	}
}
