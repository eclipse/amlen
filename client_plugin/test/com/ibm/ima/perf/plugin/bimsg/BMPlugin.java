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

/*
 * Plug-in listener object for the json_msg protocol
 */

package com.ibm.ima.perf.plugin.bimsg;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaPluginListener;

/**
 *
 */
public class BMPlugin implements ImaPluginListener {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaPlugin  plugin;

    /*
     * @see com.ibm.ima.plugin.ImaPluginListener#initialize(com.ibm.ima.plugin.ImaPlugin)
     */
    public void initialize(ImaPlugin plugin) {
        this.plugin = plugin;
    }

    /*
     * @see com.ibm.ima.plugin.ImaPluginListener#onAuthenticate(com.ibm.ima.plugin.ImaConnection)
     */
    public boolean onAuthenticate(ImaConnection connect) {
        return false;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaPluginListener#onAuthenticate(com.ibm.ima.plugin.ImaConnection)
     */
    public void onConfigUpdate(String str, Object obj1, Object obj2) {
        return;
    }
    
    /* 
     * @see com.ibm.ima.plugin.ImaPluginListener#onConnection(com.ibm.ima.plugin.ImaConnection, byte[])
     */
    public int onProtocolCheck(ImaConnection connect,byte[] data) {
    	// check protocol match "perf"
    	//System.err.println("onProtocolCheck: " + ((int)data[0]) + ((char) data[4]));
        if (data[0] == 0x10 && data[4]=='p' && data[5]=='e' && data[6]=='r' && data[7]=='f')
            return 0;
        return -1;
    }

    /* 
     * @see com.ibm.ima.plugin.ImaPluginListener#onConnection(com.ibm.ima.plugin.ImaConnection, java.lang.String)
     */
    public ImaConnectionListener onConnection(ImaConnection connect, String protocol) {
        return new BMConnection(connect, plugin);
    }

    /* 
     * @see com.ibm.ima.plugin.ImaPluginListener#startMessaging(boolean)
     */
    public void startMessaging(boolean active) {
    }

    /* 
     * @see com.ibm.ima.plugin.ImaPluginListener#terminate(int)
     */
    public void terminate(int reason) {
    }

    public String getProtocolName() {
        return "perf";
    }

}
