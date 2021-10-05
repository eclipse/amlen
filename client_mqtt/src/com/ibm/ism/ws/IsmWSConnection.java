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

package com.ibm.ism.ws;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.KeyStore;
import java.util.zip.CRC32;

import javax.net.ssl.KeyManager;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;


public class IsmWSConnection {
	String       ip;
	String       sni_ip;
	int          port;
	String []    ipList;
	int []       portList;
	protected boolean      isReconnect;
	int          reconnectPos;
	protected String       path;
	String       protocol;
	String       clientkey;
	Socket       sock;
	int          version = WebSockets13;
	OutputStream ostream;
	InputStream  istream;
	byte []      readbuf;
	byte []      sendbuf;
	protected  boolean connected;
	InetSocketAddress ipaddr;
	Object       readlock = new Object();
	Object       writelock = new Object();
	byte[]       mask;
	int          read_avail;
	int          read_pos;
	int          read_used;
	protected boolean      verbose;
	protected int          chunksize = 99;
	int          secure = 0;
	SSLContext   tlsCtx = null;
	String       keystore_name;
	String       keystore_pw;
	KeyManager []  keystore_mgr;
	volatile long         readBytes;
	volatile long         sendBytes;
	
	/*
	 * JDK 11 adds support for a subset of TLSv1.3
	 */
	
	final String [] ciphers = {
	    "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",  /* TLSv1.2 ciphers */
	    "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256",
	    "TLS_RSA_WITH_AES_128_GCM_SHA256",
	    "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA",
	    "TLS_RSA_WITH_AES_128_CBC_SHA"
	};
	
	final String [] ciphers13 = {
	    "TLS_AES_128_GCM_SHA256",                 /* TLSv1.3 ciphers */   
        "TLS_AES_256_GCM_SHA384",
	    "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",  /* TLSv1.2 ciphers */
        "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256",
        "TLS_RSA_WITH_AES_128_GCM_SHA256",
        "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA",
        "TLS_RSA_WITH_AES_128_CBC_SHA"
	};    

	/** Version for WebSockets version 13 */
	public static final int WebSockets13 = 13;
	
	/** Version for binary MQTT V3.1 */
	public static final int MQTT_TCP     = 3;
	public static final int MQTT3WS      = 4;
	public static final int MQTTWS       = 5;
	public static final int JSONMSG      = 6;
	
	
	/**
	 * Create a WebSockets connection obejct.
	 * @param ip       The IP address of the server
	 * @param port     The port number of the server
	 * @param path     The path for the websockets URI
	 * @param protocol The sub-protocol to use
	 */
	public IsmWSConnection(String ip, int port, String path, String protocol) {
		 this.ip       = ip;
		 this.sni_ip   = ip;
		 this.port     = port;
		 this.path     = path;
		 this.protocol = protocol;
	}
	
	/**
	 * Get the bytes read in a connection
	 * @return the bytes read in the connection
	 */
	public long getReadBytes() {
	    return readBytes;
    }
	
	/**
	 * Get the bytes sent in a connection
	 * @return the bytes sent in the connection
	 */
	public long getSendBytes() {
	    return sendBytes;
	}    
	
	
	/**
	 * Set the version of WebSockets to use.
	 * Only a few documented versions are supported.
	 * @param version  The version to use
	 * @return This connection object
	 */
	public IsmWSConnection setVersion(int version) {
		if (version != MQTT_TCP && version != MQTT3WS && version != MQTTWS &&
		        version != JSONMSG && version != WebSockets13  ) {
			throw new IllegalArgumentException("Unknown WebSockets version");
		}			
		this.version = version;
		if (version == MQTT_TCP)
			protocol = "mqtt";
		else if (version == MQTT3WS)
			protocol = "mqttv3.1";
	    else if (version == MQTTWS)
	        protocol = "mqtt";
		return this;
	}
	
	/**
	 * Set the keysotre for the connection
	 * @param name  The name of the keystore
	 * @param
	 */
	public IsmWSConnection setKeystore(String name, String pw) {
	    keystore_name = name;
	    keystore_pw = pw;
	    return this;
	}
	
	/*
	 * Get the trust manager
	 */
	static TrustManager[] trustmgr = null;
	static TrustManager[] getTrustManager() {
	    TrustManager [] ret = null;
	    String truststore;
	    String password;
	    String path;
	    truststore = System.getenv("MqttTrustStore");
	    if (truststore == null) {
	        truststore = System.getProperty("MqttTrustStore");
	        if (truststore == null) {
	            path = System.getenv("USERPROFILE");
	            if (path == null)
	                path = System.getenv("HOME");
	            if (path == null)
	                truststore = "truststore";
	            else
	                truststore = path + "/.truststore";
	        }    
	    }
	    password  = System.getenv("MqttTrustStorePW");
	    if (password == null) {
	        password = System.getProperty("MqttTrustStorePW") ;
	        if (password == null)
	            password = "password";
	    }
	    System.out.println("Use trust store: "+ truststore);
	    try {
	        File f = new File(truststore);
	        if (f.exists()) {
	            TrustManagerFactory tmf;
	            try {
	                tmf = TrustManagerFactory.getInstance("SunX509");
	            } catch (Exception e) {
	                tmf = TrustManagerFactory.getInstance("IbmX509");
	            }
	            KeyStore ts = KeyStore.getInstance("JKS");
	            ts.load(new FileInputStream(f), password.toCharArray());
	            tmf.init(ts);
	            ret = tmf.getTrustManagers();
	        } else {
	            System.out.println("Trust store not found: "+ truststore);
	        }
	    } catch (Exception e) {   
	        e.printStackTrace();
	    }
	    return ret;
	}
	
	/*
	 * Get the key manager
	 */
	static KeyManager[] keymgr = null;
	static KeyManager[] getKeyManager(String name, String pw) {
	    KeyManager [] ret = null;
	    String keystore;
	    String password;
	    String path;
	    /* 
	     * If the keystore for the connection is not set, use the global keystore.
	     */
	    if (name == null) {
    	    name = System.getenv("MqttKeyStore");
    	    if (name == null) {
    	        name = System.getProperty("MqttKeyStore");
    	        if (name == null)
    	            name = ".keystore";
    	    }   
	    }
	    if (name.indexOf('/') < 0 && name.indexOf('\\') < 0) {
    	    path = System.getenv("USERPROFILE");
    	    if (path == null) {
    	        path = System.getenv("HOME");
    	        if (path == null)
    	            path = ".";
    	    }    
    	    keystore = path + "/" + name;
	    } else {
	        keystore = name;
	    }
	    if (pw == null) {
    	    password  = System.getenv("MqttKeyStorePW");
    	    if (password == null) {
    	        password = System.getProperty("MqttKeyStorePW") ;
    	        if (password == null)
    	            password = "password";
    	    }
	    } else {
	        password = pw;
	    }
	    System.out.println("Use key store: "+ keystore);
	    try {
	        File f = new File(keystore);
	        if (f.exists()) {
	            KeyManagerFactory kmf;
	            try {
	                kmf = KeyManagerFactory.getInstance("SunX509");
	            } catch (Exception e) {
	                kmf = KeyManagerFactory.getInstance("IbmX509");
	            }
	            KeyStore ks = KeyStore.getInstance("JKS");
	            ks.load(new FileInputStream(f), password.toCharArray());
	            kmf.init(ks, password.toCharArray());
	            ret = kmf.getKeyManagers();
	        } else {
	            System.out.println("Key store not found: "+ keystore);
	        }
	    } catch (Exception e) {   
	        e.printStackTrace();
	    }
	    return ret;
	}
	
	/**
	 * Return the IP address
	 * @return The IP address
	 */
	public String getAddress() {
	    return ip;
	}
	
	/**
	 * Return the port
	 * @return the port
	 */
	public int getPort() {
	    return port;
	}
	
	public void setAddressList(String [] ipList) {
	    this.ipList = ipList;
	    isReconnect = false;
	    reconnectPos = 0;
	}
	
	public void setPortList(int [] portList) {
	    this.portList = portList;
	    isReconnect = false;
	    reconnectPos = 0;
	}
	
	
	/*
	 * Make the TLS Context for this connection
	 */
	SSLContext getTLSContext(int tlslvl) throws Exception {
	    String method = "TLSv1.2";
	    if (tlslvl == 1)
	        method = "TLSv1";
	    else if (tlslvl == 2)
	        method = "TLSv1.1";
	    else if (tlslvl == 4)
	        method = "TLSv1.3";
	    SSLContext ret;   
        if ("TLSv1.3".equals(method))
            ret = SSLContext.getInstance(method, "SunJSSE");   /* BUG: JDK 11 broken for TLSv1.3 search */
        else
            ret = SSLContext.getInstance(method);
	    if (trustmgr == null)
	        trustmgr = getTrustManager();
	    
	    if (keystore_name != null) {
	        /* Use configured keystore */
	        if (keystore_mgr == null)
	            keystore_mgr = getKeyManager(keystore_name, keystore_pw);
	        ret.init(keystore_mgr, trustmgr, null);
	    } else {
	        /* Use global keystore */
    	    if (keymgr == null)
    	    	keymgr = getKeyManager(null, null);
    	    ret.init(keymgr, trustmgr, null);
	    }
	    return ret;
	}
	
	
	/**
	 * Set the verbose flag. 
	 * This is used for problem determination.
	 * @param verbose
	 * @return This connection object
	 */
	public IsmWSConnection setVerbose(boolean verbose) {
		this.verbose = verbose;
		return this;
	}
	

	/**
	 * Set the connection secure
	 * @param secure  The TLS level: 0=none, 1=TLSv1, 2=TLSv1.1, 3=TLSv1.2
	 * @return This connection
	 */
	public IsmWSConnection setSecure(int secure) {
	    this.secure = secure;
	    return this;
    }
	
	/**
	 * Set the SNI host name
	 * The hostname must include a dot and must not be numeric
	 * @param hostname  The host name
	 * @return This connection
	 */
	public IsmWSConnection setHost(String hostname) {
	    this.sni_ip = hostname;
	    return this;
	}
	 
	static final String[] protocols = new String[] {"TLSv1.3", "TLSv1.2"};
	/**
	 * Make the connection.  
	 * @throws IOException
	 */
	public void connect() throws IOException {
		String origin;
		
		String con_ip = ip;
		int    con_port = port;
		
		if (isReconnect) {
		    con_ip = null;
		    if (ipList != null && ipList.length > reconnectPos)
		        con_ip = ipList[reconnectPos];
		    if (portList != null && portList.length > reconnectPos)
		        con_port = portList[reconnectPos];
		    reconnectPos++;
		} else {
		    reconnectPos = 0;
		}
		
		if (con_ip == null) {
		    throw new IOException("No server in ip list");
		}
		
		try {
		    URI uri = new URI(secure>0?"wss":"ws", null, ip, port, path, null, null);
		    origin = ""+uri;
		} catch (URISyntaxException ure) {
			origin = "ws://"+ip+":"+port;
		}
		
		
		/* 
		 * Make the TCP connection 
		 */
	    ipaddr = new InetSocketAddress(ip, port);
		if (ipaddr.isUnresolved()) 
			throw new IllegalArgumentException(ipaddr.getHostName());
		
		if (secure == 0) {
            sock = new Socket();
            sock.setTcpNoDelay(true);
            sock.connect(ipaddr);
		} else {
		    try {
		        if (tlsCtx == null)
		            tlsCtx = getTLSContext(secure);
		        Socket s = new Socket();
		        s.connect(ipaddr, 20000);
	            s.setTcpNoDelay(true);
		        SSLSocketFactory tsf = (SSLSocketFactory) tlsCtx.getSocketFactory();
		        SSLSocket tlsSock = (SSLSocket) tsf.createSocket(s, sni_ip, port, true);
		        if (secure == 4) {
		            tlsSock.setEnabledProtocols(protocols);
		            tlsSock.setEnabledCipherSuites(ciphers13);
		        } else {    
		            tlsSock.setEnabledCipherSuites(ciphers);
		        }    
		        sock = (Socket)tlsSock;
		    } catch (Exception e) {
		        e.printStackTrace();
		        throw new IOException("Unable to create TLS socket", e);
		    }
		}

    	ostream = sock.getOutputStream();
    	istream = sock.getInputStream();
    	if (readbuf == null)
    	    readbuf = new byte[16*1024]; 
    	if (sendbuf == null)
    		sendbuf = new byte[16*1024];
    	if (mask == null)
    		mask    = new byte[4];
    	read_avail = 0;
    	read_pos = 0;
    	if (secure > 0)
    	    ((SSLSocket)sock).startHandshake();
    	
		/*
		 *  Make the WS connection 
		 */
		if (version != MQTT_TCP && version != JSONMSG) {
			String ipstring = ip.trim();
						
			/* IP address have been resolved, so it is valid. 
			 * Need to ensure that for numeric IPv6 addresses
			 * WebSocket handshake contains [ ] in Host field.*/
			if (ipstring.contains(":")) {
				/*  
				 * IPv6 address in numeric format, 
				 * add "[" and "]", if missing */
				if (!ipstring.startsWith("[")) {
					ipstring = "[" + ipstring + "]";
				}
			}
			
			/* Construct the http request */
			clientkey = "ISM_Client-"+uniqueString(ip);
			StringBuffer cmd = new StringBuffer();
			cmd.append("GET ").append(path).append(" HTTP/1.1\r\n");
			cmd.append("Upgrade: WebSocket\r\nConnection: Upgrade\r\n");
			cmd.append("Host: ").append(ipstring).append("\r\n");
			cmd.append("Sec-WebSocket-Version: 13\r\n");
	        cmd.append("Sec-WebSocket-Protocol: ").append(protocol).append("\r\n");
			cmd.append("Sec-WebSocket-Key: ").append(toBase64(clientkey)).append("\r\n\r\n");
			byte [] cmdbytes;
			try { 
	            cmdbytes = cmd.toString().getBytes("UTF-8"); 
			} catch (UnsupportedEncodingException uex) {
				throw new Error("UTF-8", uex);
			}
			/* Send the http request to the server */
			ostream.write(cmdbytes, 0, cmdbytes.length);
			sendBytes += cmdbytes.length;
			
			/* Get the http response from the server */
			int readbytes = istream.read(readbuf, 0, readbuf.length);
			if (readbytes < 0) {
				throw new IOException("The WebSockets connection was not accepted");
			}
			readBytes += readbytes;
			validate_response(new String(readbuf, 0, readbytes));
		} else {
			connected = true;
		}
	}

	
	/*
	 * Validate the returned http response
	 */
	void validate_response(String hdr) throws IOException {
//		System.out.println("hdr:[" + hdr +"]");
		if (!hdr.startsWith("HTTP/")) {
			throw new IOException("The response from the WebSockets server is not valid");
		}
		int pos = hdr.indexOf(' ')+1;
		int rc = 0;
		while (hdr.charAt(pos)>='0' && hdr.charAt(pos)<='9') {
			rc = rc*10 + hdr.charAt(pos)-'0';
			pos++;
		}
		if (rc != 101) {
			throw new IOException("The return from the WebSockets server is not valid: " + rc);
		}
		connected = true;
		/* TODO: validate key */
	}
	
	
	 /**
     * Check if the connection is active.
     * @return true if the connection is active
     */
    public boolean isConnected() {
    	return connected;
    }
    
    
    /*
     * Throw an exception if the connection is active.
     * This is used to prevent a connection object from being modified when the 
     * connection is active. 
     */
    protected void verifyNotActive() {
    	if (connected)
    		throw new IllegalStateException("The connection is already active");
    }

    /*
     * Throw an exception if the connection is active.
     * This is used to prevent a connection object from being modified when the 
     * connection is active. 
     */
    protected void verifyActive() {
    	if (!connected)
    		throw new IllegalStateException("The connection is not active");
    }
	
    
	/**
	 * Send a String message.
	 * This is sent as a text message.
	 * @param payload The text of the message
	 * @throws IOException
	 */
	public void send(String payload) throws IOException {
	    byte [] paybytes = null;
	    if (verbose)
	        System.out.println("send: " + payload);
		try { paybytes = payload.getBytes("UTF-8"); } catch (UnsupportedEncodingException e) { }
		send(paybytes, 0, paybytes.length, IsmWSMessage.TEXT);
	}
	
	
	/**
	 * Send a byte array message.
	 * This is sent as a binary message
	 * @param payload  The payload of the message as a byte array
	 * @throws IOException
	 */
	public void send(byte[] payload) throws IOException {
	    send(payload, 0, payload.length, IsmWSMessage.BINARY);
	}
	
	
	/**
	 * Send a message from a byte array, specifying the start, end, and message type.
	 * @param payload   The payload as a byte array
	 * @param start     The start offset within the byte array
	 * @param len       The length of message in bytes
	 * @param mtype     The message type: TEXT or BINARY
	 * @throws IOException
	 */
	public void send(byte[] payload, int start, int len, int mtype) throws IOException {
		int pos = 0;
		int dlen = 0;
		synchronized (writelock) { 
			/* Extend the buffer if required */
			if (len+20 >= sendbuf.length) {
				int newlen = sendbuf.length;
				while (len+20 >= newlen) {
					newlen += 32*1024;
				}
				sendbuf = new byte[newlen];
			}

			if (version == WebSockets13) {
				/* Create a random mask value */
				long maskval = (long)(Math.random() * 4294967295.0);
				mask[0] = (byte)(maskval&0xff);
				mask[1] = (byte)((maskval>>8)&0xff);
				mask[2] = (byte)((maskval>>16)&0xff);
				mask[3] = (byte)((maskval>>24)&0xff);
				
				/* Construct the WebSockets frame */
				sendbuf[0] = (byte)((mtype&0xf) | 0x80);
				if (len <= 125) {
					sendbuf[1] = (byte)(len|0x80);
					pos = 2;
				} else if (len < 64*1204) {
					sendbuf[1] = (byte)(126|0x80);
					sendbuf[2] = (byte)(len>>8);
					sendbuf[3] = (byte)len;
					pos = 4;
				} else {
					sendbuf[1] = (byte)(127|0x80);
					sendbuf[2] = 0;
					sendbuf[3] = 0;
					sendbuf[4] = 0;
					sendbuf[5] = 0;
					sendbuf[6] = (byte)(len>>24);
					sendbuf[7] = (byte)(len>>16);
					sendbuf[8] = (byte)(len>>8);
					sendbuf[9] = (byte)len;
					pos = 10;
				}
				System.arraycopy(mask, 0, sendbuf, pos, 4);
				pos += 4;
				
				/* Copy and mask the payload */
				System.arraycopy(payload, 0, sendbuf, pos, len);
				for (int i=0; i<len; i++) {
					sendbuf[pos+i] ^= mask[i&3];
				}
				ostream.write(sendbuf, 0, pos+len);
				sendBytes += (pos+len);
			} 
			
			/*
			 * Put on the
			 */
			else if (version == MQTT_TCP) {
				sendbuf[0] = (byte)mtype;
			    if (len<=127) {
			        pos = 2;
			        sendbuf[1] = (byte)(len&0x7f);
			    } else if (len <= 16383) {
			        pos = 3;
			        sendbuf[1] = (byte)((len&0x7f)|0x80);
			        sendbuf[2] = (byte)(len>>7);
			    } else if (len <= 2097151) {
			        pos = 4;
			        sendbuf[1] = (byte)((len&0x7f)|0x80);
			        sendbuf[2] = (byte)((len>>7)|0x80);
			        sendbuf[3] = (byte)(len>>14);
			    } else {
			        pos = 5;
			        sendbuf[1] = (byte)((len&0x7f)|0x80);
			        sendbuf[2] = (byte)((len>>7)|0x80);
			        sendbuf[3] = (byte)((len>>14)|0x80);
			        sendbuf[4] = (byte)(len>>21);
			    }

			    if (len > 0)
			        System.arraycopy(payload, 0, sendbuf, pos, len);
			    ostream.write(sendbuf, 0, pos+len);
			    sendBytes += (pos+len);
			}
			
			else if (version == JSONMSG) {
			    ostream.write(payload, 0, len);
			    sendBytes += len;
			}
			
			/*
			 * For MQTT binary over WebSockets, put on both headers 
			 */
			else if (version == MQTT3WS || version == MQTTWS) {
			    int offset = 0;
                dlen = 2;
                if (len > 127) {
                    dlen = 3;
                    if (len > 16383) {
                        dlen = 4;
                        if (len > 2097151) {
                            dlen = 5;
                        }
                    }
                }
                byte [] mbuf = new byte[dlen+len];
                mbuf[0] = (byte)mtype;
                if (len <= 127) {
                    mbuf[1] = (byte)(len&0x7f);
                    pos = 2;
                } else if (len <= 16383) {
                    mbuf[1] = (byte)((len&0x7f)|0x80);
                    mbuf[2] = (byte)(len>>7);
                    pos = 3;
                } else if (len < 2097151) {
                    mbuf[1] = (byte)((len&0x7f)|0x80);
                    mbuf[2] = (byte)((len>>7)|0x80);
                    sendbuf[pos+3] = (byte)(len>>14);
                    pos = 4;
                } else {
                    mbuf[1] = (byte)((len&0x7f)|0x80);
                    mbuf[2] = (byte)((len>>7)|0x80);
                    mbuf[3] = (byte)((len>>14)|0x80);
                    mbuf[4] = (byte)(len>>21);
                    pos = 5;
                }
                if (len > 0) 
                    System.arraycopy(payload, 0, mbuf, pos, len);
                
                int fulllen = len + dlen;
                int chunks = chunksize;
                if (chunks <= 0) {
                    if (chunks == 0)
                        chunks = 0xffffff;
                    else {
                        chunks = -chunks;
                        if (mtype>>4 == 3) {
                            byte [] oldmbuf = mbuf;
                            mbuf = new byte[oldmbuf.length * 2];
                            System.arraycopy(oldmbuf, 0, mbuf, 0, oldmbuf.length);
                            System.arraycopy(oldmbuf, 0, mbuf, oldmbuf.length, oldmbuf.length);
                            fulllen *= 2;
                        }
                    }
                }
			    while (fulllen > 0) {
			        if (fulllen > chunks) 
			            len = chunks;
			        else
			            len = fulllen;
			        fulllen -= len;

    				/* Create a random mask value */
    				long maskval = (long)(Math.random() * 4294967295.0);
    				mask[0] = (byte)(maskval&0xff);
    				mask[1] = (byte)((maskval>>8)&0xff);
    				mask[2] = (byte)((maskval>>16)&0xff);
    				mask[3] = (byte)((maskval>>24)&0xff);
    				
    				/* Construct the WebSockets frame */
    				sendbuf[0] = (byte)0x82;
    				if (len <= 125) {
    					sendbuf[1] = (byte)(len|0x80);
    					pos = 2;
    				} else if (len < 64*1204) {
    					sendbuf[1] = (byte)(126|0x80);
    					sendbuf[2] = (byte)(len>>8);
    					sendbuf[3] = (byte)len;
    					pos = 4;
    				} else {
    					sendbuf[1] = (byte)(127|0x80);
    					sendbuf[2] = 0;
    					sendbuf[3] = 0;
    					sendbuf[4] = 0;
    					sendbuf[5] = 0;
    					sendbuf[6] = (byte)(len>>24);
    					sendbuf[7] = (byte)(len>>16);
    					sendbuf[8] = (byte)(len>>8);
    					sendbuf[9] = (byte)len;
    					pos = 10;
    				}
    				System.arraycopy(mask, 0, sendbuf, pos, 4);
    				pos += 4;
    				System.arraycopy(mbuf, offset, sendbuf, pos, len);
 
    			    offset += len;
    				for (int i=0; i<len; i++) {
    					sendbuf[pos+i] ^= mask[i&3];
    			    }  
    	            /* Send the data */
    				// System.out.println("write " + len + " bytes");
    	            ostream.write(sendbuf, 0, pos+len);
    	            sendBytes += (pos+len);
    			}
			}
		}
	}
	
	
	/**
	 * 
	 * Send a message.
	 * @param msg
	 * @throws IOException
	 */
	public void send(IsmWSMessage msg) throws IOException {
	    byte [] msgb = msg.getPayloadBytes();
	    send(msgb, 0, msgb.length, msg.getType());
	}
	
	static long getLongBig(byte[] buff, int offset) {
    	return ((long)(buff[offset]&0xff)<<56)   |
    	       ((long)(buff[offset+1]&0xff)<<48) |
    	       ((long)(buff[offset+2]&0xff)<<40) |
    	       ((long)(buff[offset+3]&0xff)<<32) |
    	       ((long)(buff[offset+4]&0xff)<<24) |
    	       ((long)(buff[offset+5]&0xff)<<16) |
    	       ((long)(buff[offset+6]&0xff)<<8) |
    	       ((long)(buff[offset+7]&0xff));
    }
	
	
	/**
	 * Receive a message.  
	 * This call will block until a message is available or the connection terminates.
	 * It is common to run the receive in a separate thread.
	 * @return The message which is received 
	 * @throws IOException
	 */
	public IsmWSMessage receive() throws IOException {
		switch (version) {
		case MQTT_TCP:      return receiveMQTT();
		case MQTT3WS:       return receiveMQTTWS();
        case MQTTWS:        return receiveMQTTWS();
        case JSONMSG:       return receiveTCP();
        case WebSockets13:  return receiveWS();
		}
		throw new IllegalStateException("Unknown version");
	}
	
	
	/*
	 * Receive a message using WebSockets framing
	 */
	public IsmWSMessage receiveWS() throws IOException {	
		int  mtype;
		int  mlen;
		int  need = 2;
		int  pos  = 0;
		int  bread;
		synchronized(readlock) {
			for (;;) {
				if (read_avail-read_pos < need) {
					int  left = readbuf.length-read_avail;
					if (left < 1024) {
						if (read_used > 0) {
							if (read_avail > read_used) {
							    System.arraycopy(readbuf, read_used, readbuf, 0, read_avail-read_used);
							}    
							read_pos -= read_used;
							read_avail -= read_used;
							read_used = 0;
						} else {
							int newlen = readbuf.length*2;
							byte [] newbuf = new byte[newlen];
							System.arraycopy(readbuf, 0, newbuf, 0, read_avail);
							readbuf = newbuf;
						}
					}
					bread = istream.read(readbuf, read_avail, readbuf.length-read_avail);
					if (bread < 0)
                        throw new IOException("Disconnect from server");
					readBytes += bread;
				    read_avail += bread;
				}	
				pos = read_pos;
				mtype = readbuf[pos];
				mlen  = readbuf[pos+1];
				if ((mlen & 0x80) != 0)
					throw new IOException("Mask set from server");
				pos += 2;
				if (mlen > 125) {
				    if (mlen == 126) {
				    	if (read_avail-read_pos < 4) {
				    		need = 4;
				    		continue;
				    	}		
				    	mlen = ((int)(readbuf[pos]&0xff))<<8 | (readbuf[pos+1]&0xff);
                        pos += 2;				    	
				    } else {
				    	if (read_avail-read_pos < 10) {
				    		need = 10;
				    		continue;
				    	}
				    	long lmlen = getLongBig(readbuf, pos);
				    	pos += 8;
				    	if (lmlen > 16*1024*1204) {
				    		throw new IOException("The message is too long");
				    	}
				    	mlen = (int)lmlen;
				    }
				}
				int msgpos = pos;
				pos += mlen;
				if (read_avail < pos) {
					need = pos;
					continue;
				}
				read_used = pos;
				if (read_used == read_avail) {
					read_used = 0;
					read_avail = 0;
					read_pos = 0;
				} else {
					read_pos = pos;
				}
				return new IsmWSMessage(readbuf, msgpos, mlen, mtype);
			}
		}			
	}
	
	/*
	 * Receive from TCP with no framing
	 */
	public IsmWSMessage receiveTCP() throws IOException {
        int bread = istream.read(readbuf, 0, readbuf.length);
        if (bread < 0)
            throw new IOException("Disconnect from server");
        readBytes += bread;
        return new IsmWSMessage(readbuf, 0, bread, 0);
	}
	
	
	/*
	 * Receive a message using MQTT binary framing
	 */
	public IsmWSMessage receiveMQTT() throws IOException {	
		int  mtype;
		int  mlen;
		int  need = 2;
		int  pos  = 0;
		int  bread;
		synchronized(readlock) {
			for (;;) {
				if (read_avail-read_pos < need) {
					int  left = readbuf.length-read_avail;
					if (left < 1024) {
						if (read_used > 0) {
							if (read_avail > read_used) {
							    System.arraycopy(readbuf, read_used, readbuf, 0, read_avail-read_used);
							}    
							read_pos -= read_used;
							read_avail -= read_used;
							read_used = 0;
						} else {
							int newlen = readbuf.length*2;
							byte [] newbuf = new byte[newlen];
							System.arraycopy(readbuf, 0, newbuf, 0, read_avail);
							readbuf = newbuf;
						}
					}
					bread = istream.read(readbuf, read_avail, readbuf.length-read_avail);
					if (bread < 0)
	                    throw new IOException("Disconnect from server");
					readBytes += bread;
				    read_avail += bread;
				}	
				pos   = read_pos;
				mtype = readbuf[pos];
				mlen  = readbuf[pos+1];
				int     count = 2;
				int     multshift = 7;
			    if ((mlen & 0x80) != 0) {
			        mlen &= 0x7f;
			        do {
			            if (pos >= read_avail)
			                need = mlen + count;
			            mlen += (readbuf[pos+count] & 0x7f) << multshift;
			            multshift += 7;
			            count++;
			        } while ((readbuf[pos+count-1]&0x80) != 0);
			    }
				pos += count;
				int msgpos = pos;
				pos += mlen;
				if (read_avail < pos) {
					need = pos;
					continue;
				}
				read_used = pos;
				if (read_used == read_avail) {
					read_used = 0;
					read_avail = 0;
					read_pos = 0;
				} else {
					read_pos = pos;
				}
				return new IsmWSMessage(readbuf, msgpos, mlen, mtype);
			}
		}		
	}	
	
	/*
	 * Receive an MQTT binary over WebSockets
	 */
	public IsmWSMessage receiveMQTTWS() throws IOException {	
		IsmWSMessage wsmsg = receiveWS();
		byte [] bytes = wsmsg.getPayloadBytes();
		int     len = bytes.length;
		int     mtype = 0;
		int     mlen = -1;
		int     count = 0;
		int     multshift;
		
		if (len >= 2) {
		    mtype = bytes[0];
		    mlen  = bytes[1];
		    count = 2;
		    multshift = 7;
            if ((mlen & 0x80) != 0) {
                mlen &= 0x7f;
                do {
	                if (count >= len) {
	                    len = 0;
	                    break;
	                }    
	                mlen += (bytes[count] & 0x7f) << multshift;
	                multshift += 7;
	                count++;
	            } while ((bytes[count-1]&0x80) != 0);
            }    
	    }
		if (count+mlen != len) {
			throw new IOException("Length mismatch between WebSockets and MQTT headers");
		}
		return new IsmWSMessage(bytes, count, mlen, mtype);
	}

	/**
	 * Close the connection normally.
	 */
	public void disconnect() throws IOException {
	    disconnect(0, "");
	}
	
	
	/**
	 * Close the connection with a return code and reason string.
	 * @param code    The return code.  This should use the defined websockets return codes.
	 * @param reason  A reason string.  This is normally human readable text and can be null.
	 */
	public void disconnect(int code, String reason) throws IOException {
//		byte [] msg;
//		if (reason != null && reason.length()>0)
//			msg = ("00"+reason).getBytes();
//		else 
//			msg = new byte[2];
//		msg[0] = (byte)(code>>8);
//		msg[1] = (byte)code;
  		synchronized (writelock) {
 			if (connected) {
  			    connected = false;
//				send(msg, 0, msg.length, 0x08);   /* Send a WebSockets close */
  			    if (secure != 0)
  			        sock.close();
  			    else
  				    sock.shutdownOutput();
  			}
		}
	    
	}
	
	/**
	 * Close the socket - abnormal termination,
	 * e.g., caused by connection timeout.
	 */
    public void terminate() {
        connected = false;
        try {
            sock.close();
        } catch (IOException e) {
        }
    }
	
	/* Map base64 digit to value */
	static final byte [] b64val = {
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 00 - 0F */
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 10 - 1F */
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,   /* 20 - 2F + / */
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,   /* 30 - 3F 0 - 9 = */
		-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,   /* 40 - 4F A - O */
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,   /* 50 - 5F P - Z */
		-1, 26, 27, 28, 29, 30, 31, 32, 33, 24, 35, 36, 37, 38, 39, 40,   /* 60 - 6F a - o */
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 41, -1, -1, -1, -1, -1    /* 70 - 7F p - z */
	};
	
	static final String b64digit = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	/* 
	 * Convert from base64 to base 256
	 */
	static String fromBase64(String from) {
		char to [] = new char[from.length()*3/4];
		int i;
		int j = 0;
		int value = 0;
		int bits = 0;
		
		if (from.length()%4 != 0) {
			throw new IllegalArgumentException("The Base 64 string must be a multiple of 3");
		}
		for (i=0; i<from.length(); i++) {
            char ch = from.charAt(i);
		    byte bval = ch>0x7f ? (byte)-1 : b64val[ch];
		    if (ch == '=' && i+2 < from.length())
		    	bval = -1;
		    if (bval >= 0) {
		        value = value<<6 | bval;
		        bits += 6;
		        if (bits >= 8) {
		            bits -= 8;
		            to[j++] = (char)((value >> bits) & 0xff);
		            if (bval == '=')
		            	break;
		        }
            } else {
            	if (bval == -1)
            		throw new IllegalArgumentException("Base 64 string is not valid");
            }
	    }
	    return new String(to);	
	}
	
	/*
	 * Convert from a base256 string to a base64 string.
	 * The upper 8 bits of the character in the source string is ignored.
	 */
	static String toBase64(String fromstr) {
		int  len = (fromstr.length()+2) / 3 * 3;
		char from [] = new char[len];
		char to [] = new char[(len/3)*4];
		fromstr.getChars(0, fromstr.length(), from, 0);
		
		int j = 0;
		int val = 0;
		int left = fromstr.length();
		for (int i=0; i<len; i += 3) {
			val = (int)(from[i]&0xff)<<16 | (int)(from[i+1]&0xff)<<8 | (int)(from[i+2]&0xff);
			to[j++] = b64digit.charAt((val>>18)&0x3f);
			to[j++] = b64digit.charAt((val>>12)&0x3f);
			to[j++] = left>1 ? b64digit.charAt((val>>6)&0x3f) : '=';
			to[j++] = left>2 ? b64digit.charAt(val&0x3f) : '=';
			left -= 3;			
		}
		return new String(to, 0, j);
	}
	
	/*
	 * Create a unique value
	 */
	String uniqueString(String clientid) {
	    CRC32 crc = new CRC32();
	    crc.update(clientid.getBytes());
	    long now = System.currentTimeMillis();
	    crc.update((int)((now/77)&0xff));
	    crc.update((int)((now/79731)&0xff));
	    long crcval = crc.getValue();
	    char [] uniqueCh = new char[5];
	    for (int i=0; i<5; i++) {
	        uniqueCh[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz".charAt((int)(crcval%62));
	        crcval /= 62;
	    }
	    return new String(uniqueCh);
	}
}
