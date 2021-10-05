/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
import java.nio.charset.StandardCharsets;
import java.security.KeyStore;

import javax.net.ssl.KeyManager;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;

/*
 * An IsmHTTPConnection represents a TCP connection using the HTTP/1.1 protocol.
 * It allows either a TLS or nno-TLS connection.
 */
public class IsmHTTPConnection {
    String       ip;
    String       sni_ip;
    int          port;
    protected String       path;
    String       context = "";
    String       protocol;
    String       clientkey;
    Socket       sock;
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
    int          verbose;
    boolean      bindump;
    int          secure = 0;
    SSLContext   tlsCtx = null;
    String       keystore_name;
    String       keystore_pw;
    String       truststore_name;
    String       truststore_pw;
    KeyManager []  keystore_mgr;
    
    final String [] ciphers = {
        "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",
        "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256",
        "TLS_RSA_WITH_AES_128_GCM_SHA256",
        "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA",
        "TLS_RSA_WITH_AES_128_CBC_SHA"
    };

    
    /**
     * Create a WebSockets connection obejct.
     * @param ip       The IP address of the server
     * @param port     The port number of the server
     */
    public IsmHTTPConnection(String ip, int port, int secure) {
         this.ip       = ip;
         this.sni_ip   = ip;
         this.port     = port;
    }

    
    /**
     * Set the keystore for the connection
     * @param name  The name of the keystore
     * @param
     */
    public IsmHTTPConnection setKeystore(String name, String pw) {
        keystore_name = name;
        keystore_pw = pw;
        return this;
    }
    /**
     * Set the turstrore for the connection
     * @param name  The name of the keystore
     * @param
     */
    public IsmHTTPConnection setTruststore(String name, String pw) {
        truststore_name = name;
        truststore_pw = pw;
        return this;
    } 
    
    /*
     * Get the trust manager
     */
    static TrustManager[] trustmgr = null;
    static TrustManager[] getTrustManager() {
        return getTrustManager(null, null);
    }
    static TrustManager[] getTrustManager(String truststore, String password) {
        TrustManager [] ret = null;
        String path;
        if (truststore == null)
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
        if (password == null)
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
    
    /*
     * Make the TLS Context for this connection
     */
    SSLContext getTLSContext(int tlslvl) throws Exception {
        String method = "TLSv1.2";
        if (tlslvl == 1)
            method = "TLSv1";
        else if (tlslvl == 2)
            method = "TLSv1.1";
        SSLContext ret = SSLContext.getInstance(method);
        if (trustmgr == null)
            trustmgr = getTrustManager(truststore_name, truststore_pw);
        
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
    public IsmHTTPConnection setVerbose(int verbose, boolean bindump) {
        this.verbose = verbose;
        this.bindump = bindump;
        return this;
    }
    

    /**
     * Set the connection secure
     * @param secure  The TLS level: 0=none, 1=TLSv1, 2=TLSv1.1, 3=TLSv1.2
     * @return This connection
     */
    public IsmHTTPConnection setSecure(int secure) {
        this.secure = secure;
        return this;
    }
    
    /**
     * Set the SNI host name
     * The hostname must include a dot and must not be numeric
     * @param hostname  The host name
     * @return This connection
     */
    public IsmHTTPConnection setHost(String hostname) {
        this.sni_ip = hostname;
        return this;
    }
     
    /**
     * Make the connection.  
     * @throws IOException
     */
    public void connect() throws IOException {        
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
                tlsSock.setEnabledCipherSuites(ciphers);
                sock = (Socket)tlsSock;
            } catch (Exception e) {
                throw new IOException("Unable to create TLS socket", e);
            }
        }

        ostream = sock.getOutputStream();
        istream = sock.getInputStream();
        if (readbuf == null)
            readbuf = new byte[64*1024]; 
        if (sendbuf == null)
            sendbuf = new byte[16*1024];
        if (mask == null)
            mask    = new byte[4];
        read_avail = 0;
        read_pos = 0;
        if (secure > 0)
            ((SSLSocket)sock).startHandshake();

        connected = true;
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
        try { paybytes = payload.getBytes("UTF-8"); } catch (UnsupportedEncodingException e) { }
        send(paybytes, 0, paybytes.length);
    }
    
    
    /**
     * Send a byte array message.
     * This is sent as a binary message
     * @param payload  The payload of the message as a byte array
     * @throws IOException
     */
    public void send(byte[] payload) throws IOException {
        send(payload, 0, payload.length);
    }
    
    
    /**
     * Send a message from a byte array, specifying the start, end, and message type.
     * @param payload   The payload as a byte array
     * @param start     The start offset within the byte array
     * @param len       The length of message in bytes
     * @throws IOException
     */
    public void send(byte[] payload, int start, int len) throws IOException {
        synchronized (writelock) { 
            ostream.write(payload, start, len);
        }
    }
    
 
    /**
     * Receive an HTTP response
     * @return The HTTP response
     */
    public IsmHTTPResponse receive() throws IOException {
        /* TODO: handle large reponses */
        int bread = istream.read(readbuf, 0, readbuf.length);
        if (bread < 0)
            throw new IOException("Disconnect from server");
        return new IsmHTTPResponse(readbuf, 0, bread);
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
        synchronized (writelock) {
            if (connected) {
                connected = false;
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
    
    /**
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
    
    /**
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
    
    /**
     * Set the context name used for the dump.
     * When testing this is normally the name of the test case
     * @param context  The context name
     */
    public void setContext(String context) {
        this.context = context != null ? context : "";
    }
    
    /*
     * Put out the characters in a dump
     */
    private int putchars(char [] line, int pos, byte [] data, int offset, int len) {
        int       i;
        line[pos++] = ' ';
        line[pos++] = '[';
        for (i=0; i<len; i++) {
            if (data[offset+i]<0x20 || data[offset+i]>=0x7f)
                line[pos++] = '.';
            else
                line[pos++] = (char)(data[offset+i]&0xff);
        }
        line[pos++] = ']';
        return pos;
    }
    
    char [] hexdigit  = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    
    /**
     * Dump the data for debugging.
     * This is in binary or string form depending on the setting in setVerbose
     * @param header  The header string
     * @param data    The data to dump
     */
    public void dumpData(String header, byte [] data) {
        if (data == null) {
            System.out.println(context + ' ' + header);
        } else {
            System.out.println(context + ' ' + header + " length=" + data.length);
            if (!bindump) {
                System.out.println(new String(data, StandardCharsets.UTF_8));
            } else {    
                int pos = 0;
                int loc = 0;
                int mod;
                int i;
                char [] line = new char[128];
                while (pos < data.length) {
                    line[0] = hexdigit[(pos>>12)&0xf];
                    line[1] = hexdigit[(pos>>8)&0xf];
                    line[2] = hexdigit[(pos>>4)&0xf];
                    line[3] = hexdigit[pos&0xf];
                    loc = 5;
                    mod = 0;
                    for (i=0; i<32; i++, pos++) {
                        if (pos >= data.length) {
                            line[loc++] = ' ';
                            line[loc++] = ' ';
                        } else {
                            line[loc++] = hexdigit[(data[pos]>>4)&0x0f];
                            line[loc++] = hexdigit[data[pos]&0x0f];
                        }
                        if (mod++ == 3) {
                            line[loc++] = ' ';
                            mod = 0;
                        }
                    }
                    loc = putchars(line, loc, data, pos-32, pos<=data.length ? 32 : data.length-pos+32);
                    System.out.println(new String(line, 0, loc));
                }
            }
        }    
    }
}
