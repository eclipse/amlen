/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ism.jsonmsg;

/*
 * Implementa simple listener thread
 */
class JsonListenerThread implements Runnable {
    JsonConnection connect;
    JsonListener   listen;

    /*
     * Create a listener thread
     */
    JsonListenerThread(JsonConnection connect) {
        this.connect = connect;    
        listen = connect.listen;
    }
    
    /*
     * Run the thread.
     * @see java.lang.Runnable#run()
     */
    public void run() {       
        for(;;) {
            try {
                Object obj = connect.recvq.take();
                if (obj==null)
                    break;
                if (obj instanceof JsonMessage) {
                    JsonMessage msg = (JsonMessage)obj;
                    listen.onMessage(msg);
                    if (msg.getQoS() > 0) {
                        connect.acknowledge(msg, 0);
                    }
                } else if (obj instanceof Throwable ) {       /* Disconnect with exception */
                    listen.onDisconnect(-1, (Throwable)obj);
                    break;
                } else if (obj instanceof Integer) {          /* Disconnect with return code */
                    listen.onDisconnect((Integer)obj, null);
                    break;
                }
            } catch (InterruptedException iex) { 
            }
        }
    }
    
}
