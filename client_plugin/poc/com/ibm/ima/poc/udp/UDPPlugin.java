/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.poc.udp;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.charset.Charset;
import java.util.Map;
import java.util.Random;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaMessageType;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaPluginListener;

/*
 * The ATBPlugin represents a plugin to implement an HTTP based ATB/NGTP protocol.
 * Each incoming HTTP connection is sent as an Connection: close and therefore each
 * connection send a single request and receives the response.
 * 
 * We do not authenticate the incoming HTTP connection, but publish messages to the
 * server using a set of virtual connections.  We use multiple virtual connections to 
 * make proper use of the processor with a number of cores.
 */
public class UDPPlugin implements ImaPluginListener, Runnable {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaPlugin        plugin;
    ImaConnection [] virtconn;
    int              vcount;
    Random           rnd = new Random();
    int              port;
    String           ipaddr;
    DatagramSocket   sock;
    String           endpoint;
    volatile boolean closed = false;
    
    static Charset utf8 = Charset.forName("UTF-8");  /* UTF-8 encoding  */

    /**
     * Default constructor.  
     * <p>
     * This is invoked by IBM MessageSight when a plug-in 
     * is loaded based on the Class property of the plugin.json definition file.
     */
    public UDPPlugin() {
    }
    
    /**
     * Initialize the plug-in
     * @see com.ibm.ima.plugin.ImaPluginListener#initialize(com.ibm.ima.plugin.ImaPlugin)
     */
    public void initialize(ImaPlugin plugin) {
        this.plugin = plugin;
        plugin.trace("Initialize the " + plugin.getName() + " plug-in.");
        plugin.log("UDP2800", ImaPlugin.LOG_NOTICE, "Server", "Initialize plug-in: {0}", plugin.getName());
        
    }

    /**
     * Authenticate a connection.
     * 
     * This method is intended for use by self-authenticating protocols but is
     * not currently invoked by the MessageSight server.  For protocols that
     * are not self-authenticating, always return true.
     * 
     * @see com.ibm.ima.plugin.ImaPluginListener#onAuthenticate(com.ibm.ima.plugin.ImaConnection)
     */
    public boolean onAuthenticate(ImaConnection connect) {
        /* IBM MessageSight currently does not invoke this method. */
        plugin.trace(7, "onAuthenticate is not implemented. Plugin=" + plugin.getName());
        return false;
    }
    
    /*
     * Get the connection to use.
     * This is provided for use by ATBConnection.
     * @param which The server connection to which this connection is assigned
     * @return The server connection to use to publish messages
     */
    ImaConnection getConnection(int which) {
        int i;
        synchronized (this) {
            for (i=0; i<vcount; i++) {
                if (virtconn[which] != null)
                    return virtconn[which];
                if (++which == vcount)
                    which = 0;
            }
        }
        return null;
    }
    
    /*
     * Get a random channel number from among the set of virtual servers.
     * @return an integer from 0 to n-1 where n is the number of server connections
     */
    int getChannel() {
        return rnd.nextInt(vcount);
    }

    /** 
     * Check that this connection is for this protocol.
     *
     * We do not accept connections
     * 
     * @see com.ibm.ima.plugin.ImaPluginListener#onConnection(com.ibm.ima.plugin.ImaConnection, byte[])
     */
    public int onProtocolCheck(ImaConnection connect, byte[] data) {
        return -1;
    }

    /** 
     * When the connection is accepted for this protocol, create a new connection listener. 
     * 
     * @see com.ibm.ima.plugin.ImaPluginListener#onConnection(com.ibm.ima.plugin.ImaConnection, java.lang.String)
     */
    public ImaConnectionListener onConnection(ImaConnection connect, String protocol) {
        int  i;
        int  which = -1;

        /* Server connection */
        synchronized (this) {
            for (i=0; i<vcount; i++) {
                if (virtconn[i] == null)
                    break;
            }
            which = i;
            virtconn[which] = connect;
            plugin.trace(6, "New virtual UDP connection: which=" + which);
        }
        return new UDPConnection(connect, plugin, this, which);
    }

    /**
     * Handle the update of a configuration item.
     * 
     * @see com.ibm.ima.plugin.ImaPluginListener#onConfigUpdate(String, Object, Object)
     */
    public void onConfigUpdate(String name, Object subname, Object value) {
        /* IBM MessageSight currently does not invoke this method. */
        plugin.trace(7, "onConfigUpdate is not implemented. Plugin=" + plugin.getName());
    }
    
    /** 
     * This callback informs us that messaging is now active in the server.
     * 
     * Since we are basing our work on incoming connections, we do not care about this as
     * we will not get a onConnection() callback unless messaging is started.
     * This would be used by a plug-in such as a bridge which wanted to do some 
     * subscriptions at the start of messaging.
     * 
     * @see com.ibm.ima.plugin.ImaPluginListener#startMessaging(boolean)
     */
    public void startMessaging(boolean active) {
        int i;
        Map<String, Object> config = plugin.getConfig();
        Object vobj = config.get("ServerConnections");
        if (vobj != null && vobj instanceof Number) { 
            vcount = ((Number)vobj).intValue();
            if (vcount < 1)
                vcount = 1;
        } else {
            vcount = 2;
        }    
        
        vobj = config.get("Port");
        if (vobj != null && vobj instanceof Number) {
            port = ((Number)vobj).intValue();
        } else {
            port = 9999;
        }
        
        vobj = config.get("IPAddr");
        if (vobj != null && vobj instanceof String) {
            ipaddr = (String)vobj;
        } else {
            ipaddr = null;
        }
        
        vobj = config.get("Endpoint");
        if (vobj != null && vobj instanceof String) {
            endpoint = (String)vobj;
        } else {
            endpoint = "udp";
        }
        
        plugin.trace(3, "startMessaging Plugin=" + plugin.getName() + " IPAddr=" + (ipaddr == null ? "*" : ipaddr) + 
                " Port=" + port + " ServerConnections=" + vcount);
        virtconn = new ImaConnection[vcount];
        for (i=0; i<vcount; i++) {
            try {
                plugin.createConnection("udp!virt", endpoint);
            } catch (Exception rtx) {
                plugin.trace("Unable to create UPD virtual connection");
                plugin.traceException(rtx);
            }
        }    
        
        /*
         * Listen on UDP port
         */
        try {
            SocketAddress saddr;
            if (ipaddr == null)
                saddr = new InetSocketAddress(port);
            else
                saddr = new InetSocketAddress(ipaddr, port);
            sock = new DatagramSocket(null);
            sock.setReuseAddress(true);
            sock.bind(saddr);
            
        } catch (Exception iox) {
            plugin.trace("Unable to create UDP listening socket: IPAddr=" + 
                    (ipaddr == null ? "*" : ipaddr) + " Port=" + port);
            plugin.traceException(iox);
        }
        new Thread(this, "udplisten").start();
    }
    
    /*
     * Send a completion message
     */
    void onComplete(ImaMessage msg, int rc, String reason) {
        Object obj = msg.getUserData();
        if (obj != null && obj instanceof UDPCorrelation) {
            UDPCorrelation corr = (UDPCorrelation)obj;
            String response = "ACK:" + corr.msgid + ":" + rc;
            if (reason != null)
                response += ":" + reason;
            try {
                byte [] responseb = response.getBytes(utf8);
                sock.send(new DatagramPacket(responseb, responseb.length, corr.client));
            } catch (Exception iox) {
                plugin.trace("Unable to send UDP response: Message=" + corr.msgid + " To=" + corr.client); 
                plugin.traceException(iox);
            }
        }
    }

    /**
     * 
     * @see java.lang.Runnable#run()
     */
    public void run() {
        /*
         * Buffer to receive the datagram
         */
        byte [] buf = new byte[8096];
        DatagramPacket packet = new DatagramPacket(buf, buf.length);
        
        /*
         * Loop until we are done
         */
        while (!closed) {
            int msgid = 0;
            String topic = null;
            try {
                sock.receive(packet);
                int packetlen = packet.getLength();
                plugin.trace("UDP receive packetlen="+packetlen);
                
                /*
                 * Parse a message in the form:  MSG:seq:topic:body
                 */
                int pos = 0;
                if (packetlen > 4 && buf[0]=='M' && buf[1]=='S' && buf[2]=='G' && buf[3]==':') {
                    pos = 4;
                    while (pos < packetlen && buf[pos]>='0' && buf[pos]<='9') {
                        msgid = msgid*10 + (buf[pos]-'0');
                        pos++;
                    }
                    if (pos < packetlen && buf[pos]==':') {
                        int topicpos = ++pos;
                        while (pos < packetlen && buf[pos] != ':')
                            pos++;
                        if (buf[pos] == ':') {
                            topic = new String(buf, topicpos, pos-topicpos);
                            pos++;
                        } else {
                            plugin.trace("no topic pos=" + pos + "[" + new String(buf, 0, packetlen) + ']');
                        }
                    } else {
                        plugin.trace("error pos=" + pos + "[" + new String(buf, 0, packetlen) + ']');
                    }
                } else {
                    plugin.trace("no MSG: [" + new String(buf, 0, packetlen) + ']');
                }
                if (topic == null || topic.length() == 0) {
                    plugin.trace("The UDP message is not valid: From=" + packet.getSocketAddress()); 
                } else {
                    ImaMessage msg = plugin.createMessage(ImaMessageType.bytes); 
                    ImaConnection channel = getConnection(getChannel());
                    UDPCorrelation corr = null;
                    if (msgid != 0) {
                        corr = new UDPCorrelation(packet.getSocketAddress(), msgid);
                        msg.setUserData(corr);
                    }  
                    byte [] body = new byte[packetlen-pos];
                    System.arraycopy(buf, pos, body, 0, packetlen-pos);
                    msg.setBodyBytes(body);
                    channel.publish(msg, topic, corr);
                }
            } catch (Exception iox) {
                plugin.traceException(iox);
            }
        }
        
    }

    /** 
     * This callback informs us that the MessgeSight server is terminating.
     * 
     * This is about to terminate the plug-in process as well, so we do not need
     * to do anything at this point.  This callback could be used by close
     * any external resources.  
     * 
     * @see com.ibm.ima.plugin.ImaPluginListener#terminate(int)
     */
    public void terminate(int reason) {
        plugin.trace(3, "terminate Plugin=" + plugin.getName());
        closed = true;
    }
    
    /*
     * Inner object for correlation
     */
    public class UDPCorrelation {
        SocketAddress client;
        int           msgid;
        
        /* Constructor */
        UDPCorrelation(SocketAddress client, int msgid) {
            this.client = client;
            this.msgid = msgid;
        }
    }


}

