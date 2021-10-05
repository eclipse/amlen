/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
/*
 * POC sample for ATB plugin
 */
package com.ibm.ima.poc.atb;

import java.nio.charset.Charset;
import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaDestinationType;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaMessageType;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaReturnCode;
import com.ibm.ima.plugin.ImaSubscription;

/**
 * The ATBConnection represents either an incoming ATB HTTP connection or
 * an outgoing server connections.
 * 
 * One of the issues with the data and especially the periodic data is that it is
 * not byte aligned.  Some amount of effort should be taken to help the performance
 * of this parsing.  Some study shows that using specific size methods to get the 
 * big endian data and putting the shifting inline gives best performance.  
 * 
 */
public class ATBConnection implements ImaConnectionListener {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaConnection  connect;          /* The base connection object this listener is associated with */
    ImaPlugin      plugin;           /* The plug-in object this listener is associated with         */
    ATBPlugin     listener;
    int            which;
    static Charset utf8 = Charset.forName("UTF-8");  /* UTF-8 encoding  */
    String clientID = "";
    boolean connected = false;
    int  channel = 0;
    
    /**
     * Constructor for the connection listener object.
     * 
     * This is called when a connection is created and it saves the connection and plugin objects
     * for later use. 
     */
    ATBConnection(ImaConnection connect, ImaPlugin plugin, ATBPlugin listener, int which) {
        this.connect  = connect;
        this.plugin   = plugin;
        this.listener = listener;
        this.which    = which;
        if (which < 0) {
            channel = listener.getChannel();
        }
    }

    /**
     * Handle the close of a connection.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onClose(int, java.lang.String)
     */
    public void onClose(int rc, String reason) {
        /*
         * In the case of an error termination, restart the connection 
         */
        if (which >= 0) {
            synchronized (this) {
                listener.virtconn[which] = null;
            }
            if (rc != ImaReturnCode.IMARC_ServerTerminating && rc != ImaReturnCode.IMARC_EndpointDisabled) {
                plugin.createConnection("atb", listener.endpoint);
            }    
        }   
    }
    
    
    /**
     * Handle a request to check if the connection is alive.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onLivenessCheck()
     */
    public boolean onLivenessCheck() {
        return true;
    }
    
    
    /**
     * Handle data from the client.
     * 
     * When we receive data from the client determine the appropriate action to take.
     * If we find one call the method associated with the action.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onHttpData(java.lang.String, java.lang.String, java.lang.String, java.lang.String, byte[])
     */
    public void onHttpData(String op, String path, String query, String content_type, byte [] data) {
        if (plugin.isTraceable(9)) {
            plugin.trace("onHttpData op " + op);
            plugin.trace("onHttpData path " + path);
            plugin.trace("onHttpData content_type " + content_type);
        }
        
        if (path.length() < 2 || path.charAt(0) != '/') 
            connect.sendHttpData(400, "text/plain;charset=utf-8", "The path is not valid");

        if ("POST".equals(op)) {
            onHttpDataPOST(path, data);
        } else {
            connect.sendHttpData(400, "text/plain;charset=utf-8", "The operation is not known");
        }
    }

    /*
     * Get an integer values.
     * 
     * Testing shows that having separate calls by length is much faster than
     * having a single call which handles multiple lengths.
     */ 
    static int bigInt1(byte [] data, int offset) {
        return data[offset]&0xff;
    }

    /*
     * Get a two byte value
     */
    static int bigInt2(byte [] data, int offset) {
        return ((data[offset]&0xff) << 8) + (data[offset+1&0xff]);
    }
    /*
     * Get a three byte value
     */
    static int bigInt3(byte [] data, int offset) {
        return ((data[offset]&0xff) << 16) + ((data[offset+1]&0xff) << 8) + (data[offset+2]&0xff);
    }
    
    /*
     * Get a four byte value
     */
    static int bigInt4(byte [] data, int offset) {
        return ((data[offset]&0xff) << 24) + ((data[offset+1]&0xff) << 16) + 
                ((data[offset+2]&0xff) << 8) + (data[offset+3]&0xff);
    }
    
    /*
     * Get a five byte value which is used to get a not byte aligned 4 byte value
     */
    static long bigLong5(byte [] data, int offset) {
        return (((long)data[offset]&0xff) << 32) + ((data[offset]&0xff) << 24) + 
            ((data[offset+1]&0xff) << 16) + ((data[offset+2]&0xff) << 8) + (data[offset+3]&0xff);
    }
    
    /*
     * Convert an ATP date into an ISO8601 timestamp.
     * @param atptime  Time in ATP encoding
     * @param tzone    Hours east of UTC.  If an invalid value is given, no timezone is shown.
     */
    static String getTimeString(int atptime, int tzone) {
        int year   = (atptime>>26) & 0x3f;
        int month  = (atptime>>22) & 0xf;
        int day    = (atptime>>17) & 0x1f;
        int hour   = (atptime>>12) & 0x1f;
        int minute = (atptime>>6)  & 0x3f;
        int second = atptime & 0x3f;

        /* Just expand this out manually to get the leading zeros */
        char [] date = new char[25];
        int len = 19;
        date[0]  = '2';
        date[1]  = '0';
        date[2]  = (char) ('0'+ (year/10));
        date[3]  = (char) ('0' + (year%10));
        date[4]  = '-';
        date[5]  = (char) ('0' + (month/10));
        date[6]  = (char) ('0' + (month%10));
        date[7]  = '-';
        date[8]  = (char) ('0' + (day/10));
        date[9]  = (char) ('0' + (day%10));
        date[10] = ' ';
        date[11] = (char) ('0' + (hour/10));
        date[12] = (char) ('0' + (hour%10));
        date[13] = ':';
        date[14] = (char) ('0' + (minute/10));
        date[15] = (char) ('0' + (minute%10));
        date[16] = ':';
        date[17] = (char) ('0' + (second/10));
        date[18] = (char) ('0' + (second%10));
        if (tzone == 0) {
            date[19] = 'Z';
            len = 20;
        } else if (tzone > -15 && tzone < 15) {
            if (tzone > 0) {
                date[19] = '+';
            } else {
                date[19] = '-';
                tzone = 0-tzone;
            }
            date[20] = (char) ('0' + (tzone/10));
            date[21] = (char) ('0' + (tzone%10));
            date[22] = ':';
            date[23] = '0';
            date[24] = '0';
            len = 25;
        }
        return new String(date, 0, len);
    }

    /*
     * Parse a periodic data message.
     * 
     * Show an example of parsing non byte aligned data.
     */
    void parsePeriodic(byte [] data, int offset, int tzone) {
        int  val;
        char [] v = new char [17];
        int date_collect = bigInt4(data, offset);
        int date_sent = bigInt4(data, offset+4);
        
        /* Convert the VIN.  This is a lot more work as it is not byte aligned */
        val = bigInt4(data, offset+11);
        v[0] = (char)((val>>22) & 0xff);
        v[1] = (char)((val>>14) & 0xff);
        v[2] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+14);
        v[3] = (char)((val>>22) & 0xff);
        v[4] = (char)((val>>14) & 0xff);
        v[5] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+17);
        v[6] = (char)((val>>22) & 0xff);
        v[7] = (char)((val>>14) & 0xff);
        v[8] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+20);
        v[9] = (char)((val>>22) & 0xff);
        v[10] = (char)((val>>14) & 0xff);
        v[11] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+23);
        v[12] = (char)((val>>22) & 0xff);
        v[13] = (char)((val>>14) & 0xff);
        v[14] = (char)((val>>6)  & 0xff);
        val = bigInt3(data, offset+26);
        v[15] = (char)((val>>14) & 0xff);
        v[16] = (char)((val>>6)  & 0xff);
        String vin = new String(v);
        plugin.trace("Periodic vin="+vin + " collected=" + getTimeString(date_collect, tzone) +
                     " sent=" + getTimeString(date_sent, tzone));
    }
    
    /**
     * Handle a POST request. 
     * 
     * This gives an example of parsing the ATB headers.
     * It is not clear how much of the message we need to parse here.
     * 
     * The objects are packed in big-endian order, both for bytes and bits.
     * In the spec, the byte offset is 1 based and the bit number is 0 based.
     * Thus 13.6 represents the 0x02 bit in the byte at offset 12.
     * 
     * @param path           The REST request path minus leading alias
     * @param data           byte array of data associated with request.
     */
    private void onHttpDataPOST(String path, byte[] data) {
        String topic = "atb";
        String vis1 = "";
        boolean wait = false;
        /* TODO parse and send the message */
        if (data.length > 42) {
            if (data[0] == 0x01) {
                /* parse data from NGTP */
                vis1 = new String(data, 4, 10);
                int dsp = bigInt2(data, 16);
                int msgid = bigInt2(data, 20);
                int service= bigInt1(data, 24);
                int msg_type = bigInt1(data, 27);
                int tstamp = bigInt4(data, 30);
                String tstring = getTimeString(tstamp, 0);
                int protover = bigInt1(data, 36);
                int datalen = bigInt3(data, 39);
                if (plugin.isTraceable(8)) {
                    plugin.trace("ngtp vin="+vis1+" dsp="+dsp+" msg="+msgid+" service="+service+" msgtype="+msg_type+
                        " time="+tstring+" verion="+protover+" datalen="+datalen);
                }    
                if (msg_type == 54) {
                    parsePeriodic(data, 44, 0); 
                }
                /* TODO: Make topic from decoded header */
                topic = "atb/" + vis1;
            } else if (data[0] == 'A' && data.length >68) {
                /* parse data from ATP */
                vis1 = new String(data, 4, 10);
                String vis2 = new String(data, 14, 10);
                String imei = new String(data, 24, 15);
                String imsi = new String(data, 39, 16);
                String lang = new String(data, 55, 2);
                String mcc  = new String(data, 57, 3);
                /* Timezone in hours only, does not work for Indiz and parts of Canada */
                int    tzone = bigInt1(data, 60) - 12; 
                int    comp_ver = bigInt2(data, 61);
                int    firm_ver = bigInt2(data, 63);
                int    soft_ver = bigInt2(data, 65);
                int    msgcount = bigInt2(data, 67);
                int    msgoffset = 69;
                if (plugin.isTraceable(8)) {
                    plugin.trace("atb vis1="+vis1+" vis2="+vis2+" imei="+imei+" imsi="+imsi+
                        " lang="+lang+" mcc="+mcc+" timezone="+tzone+
                        " comp_ver="+comp_ver+" firm_ver="+firm_ver+
                        " soft_ver="+soft_ver+" msg_count="+msgcount);
                }   
                if (msgcount > 0 && data.length >= msgoffset+4 && data[msgoffset]==0x07) {
                    parsePeriodic(data, msgoffset+4, tzone);    
                }
                topic = "atb/" + vis1;
            }
        }
        ImaMessageType mtype = ImaMessageType.bytes;

        
        /* Create a message with the appropriate message type.  */
        ImaMessage msg = plugin.createMessage(mtype);
        msg.setBodyBytes(data);

        ATBConnection sconnect = listener.getConnection(channel);
        if (!wait) {
            sconnect.connect.send(msg, ImaDestinationType.topic, topic, null);
            int opt = ImaConnection.HTTP_Option_Close | ImaConnection.HTTP_Option_ReadMsg;
            connect.sendHttpData(200, opt, "text/plain", "");
        } else {
            sconnect.connect.send(msg, ImaDestinationType.topic, topic, connect);
        }
    }

    
    /**
     * Handle data from the client.
     * 
     * This is only called when the virtual connection is first established
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onData(byte[])
     */
    public int onData(byte[] data, int offset, int length) {
        clientID = "__ATB_" + which;
        connect.setKeepAlive(0);         /* We do not want to time out */
        connect.setIdentity(clientID, null, null, ImaConnection.Authenticated, false);
        return -1;
    }
    
        /**
     * Handle the completion of a connection and send a connection message to the client.
     *
     * We do not need to do anything as the http response will indicate the connection
     * status.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onConnected(int, java.lang.String)
     */
    public void onConnected(int rc, String reason) {
        connected = true;
    }

    
    /**
     * Handle a message from the server.
     * 
     * This is not called as we have no subscriptions
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onMessage(ImaSubscription sub, ImaMessage msg) {
    }

    
    /**
     * Handle a completion event.
     * 
     * The only completion is for a persistent message, and the correlation object is
     * the connection on which to send the response.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onComplete(java.lang.Object, int, java.lang.String)
     */
    public void onComplete(Object obj, int rc, String message) {
        if (plugin.isTraceable(9) || rc != 0) {
            plugin.trace("onComplete Connect=" + this + " Object= " + obj + " rc=" + rc);
        }
        if (obj != null) {
            if (obj instanceof ImaConnection) {
                /* TODO better error handling */
                ImaConnection rconnect = (ImaConnection)obj;
                if (rc == 0) {
                    int opt = ImaConnection.HTTP_Option_Close | ImaConnection.HTTP_Option_ReadMsg;
                    rconnect.sendHttpData(200, opt, "text/plain", (byte[])null);
                } else {
                    rconnect.sendHttpData(400, "text/plain", "Messge not sent: error="+rc);
                }
            } else {
                plugin.trace(3, "onComplete unknown complete object: Connect=" + this + " Object=" + obj);
            }
        } else {
            plugin.trace(3, "onComplete null object: Connect=" + this);
        }
    }
    
    
    /**
     * Give a simple string representation of this connection listener object.
     * 
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "ATBConnection " + connect.getClientID();
    }
    
    
    /**
     * Handle a message from the server.
     * 
     * This is not called as we do not ask for retained messages.
     *  
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onGetMessage(Object correlate, int rc, ImaMessage msg) {
    }

}
