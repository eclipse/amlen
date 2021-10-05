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

import java.util.Map;
import java.util.Random;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
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
public class ATBPlugin implements ImaPluginListener {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaPlugin  plugin;
    ATBConnection [] virtconn;
    int              vcount;
    String           endpoint;
    Random           rnd = new Random();

    /**
     * Default constructor.  
     * <p>
     * This is invoked by IBM MessageSight when a plug-in 
     * is loaded based on the Class property of the plugin.json definition file.
     */
    public ATBPlugin() {
    }
    
    /**
     * Initialize the plug-in
     * @see com.ibm.ima.plugin.ImaPluginListener#initialize(com.ibm.ima.plugin.ImaPlugin)
     */
    public void initialize(ImaPlugin plugin) {
        this.plugin = plugin;
        plugin.trace("Initialize the " + plugin.getName() + " plug-in.");
        plugin.log("ATB2800", ImaPlugin.LOG_NOTICE, "Server", "Initialize plug-in: {0}", plugin.getName());
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
    ATBConnection getConnection(int which) {
        int i;
        synchronized (this) {
            for (i=0; i<vcount; i++) {
                if (virtconn[which] != null)
                    return (ATBConnection)virtconn[which];
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
     * The endpoint can share among all protocols so we need to be able to determine whether 
     * this connection is being sent to the restmsg protocol.
     * 
     * As we only support HTTP connections in this plugin, this is not configured.
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
        plugin.trace(6, "onConnection protocol=" + protocol);
        if (protocol.charAt(0) == '/') {
            plugin.trace(6, "onConnection Plugin=" + plugin.getName() + " ClientAddress=" + connect.getClientAddress());
            return new ATBConnection(connect, plugin, this, -1);
            /* HTTP connection */
        } else {
            /* Server connection */
            synchronized (this) {
                for (i=0; i<vcount; i++) {
                    if (virtconn[i] == null)
                        break;
                }
                if (i >= vcount)
                    return null;
                which = i;
                virtconn[which] = new ATBConnection(connect, plugin, this, which);
                plugin.trace("virtconn["+which+ "] = " + virtconn[which]);
                return virtconn[which];
            }
        }
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
        
        vobj = config.get("Endpoint");
        if (vobj != null && vobj instanceof String) {
            endpoint = (String)vobj;
        } else {
            endpoint = "atb";
        }
        
        plugin.trace(3, "startMessaging Plugin=" + plugin.getName() + " ServerConnections=" + vcount);
        virtconn = new ATBConnection[vcount];
        for (i=0; i<vcount; i++) {
            plugin.createConnection("atb", endpoint);
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
    }
}
