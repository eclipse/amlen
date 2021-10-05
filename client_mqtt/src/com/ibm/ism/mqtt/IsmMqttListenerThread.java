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

package com.ibm.ism.mqtt;

/*
 * Implementa simple listener thread
 */
class IsmMqttListenerThread implements Runnable {
    IsmMqttConnection connect;
    IsmMqttListener   listen;
    IsmMqttListenerAsync listenAsync;

    /*
     * Create a listener thread
     */
    IsmMqttListenerThread(IsmMqttConnection connect) {
        this.connect = connect;    
        listen = connect.listen;
        listenAsync = connect.listenAsync;
    }
    
    /*
     * Run the thread.
     * @see java.lang.Runnable#run()
     */
    public void run() {      
        int   rc;
        if (listenAsync != null) {
            /*
             * Async listener loop 
             */
            for(;;) {
                try {
                    Object obj = connect.recvq.take();
                    if (obj == null)
                        break;
                    if (obj instanceof IsmMqttMessage) {
                        IsmMqttMessage msg = (IsmMqttMessage)obj;
                        rc = listenAsync.onMessage(msg);
                        /* TODO: validate RC */
                        if (msg.getQoS() > 0) {
                            connect.acknowledge(msg, rc, null);
                        }
                    } else if (obj instanceof Throwable ) {       /* Disconnect with exception */
                        IsmMqttResult bsm = new IsmMqttResult(connect, (IsmMqttConnection.DISCONNECT<<4), null, 0, true);
                        bsm.complete = true;
                        bsm.setReasonCode(-1);
                        bsm.setThrowable((Throwable)obj);
                        listenAsync.onDisconnect(bsm);
                        connect.wakeWaiters(null, (Throwable)obj);
                        break;
                    } else if (obj instanceof IsmMqttResult) {          /* Disconnect with return code */
                        IsmMqttResult bsm = (IsmMqttResult)obj;
                        bsm.complete = true;
                        if (bsm.getOperation() == IsmMqttConnection.DISCONNECT) {
                            listenAsync.onDisconnect(bsm);
                            connect.wakeWaiters((IsmMqttResult)bsm, bsm.getThrowable());
                            break;
                        } else {   
                            listenAsync.onCompletion(bsm);
                        }
                    }
                } catch (InterruptedException iex) { 
                }
            }    
        } else {
            /*
             * Legacy listener loop
             */
            for(;;) {
                try {
                    Object obj = connect.recvq.take();
                    if (obj == null)
                        break;
                    if (obj instanceof IsmMqttMessage) {
                        IsmMqttMessage msg = (IsmMqttMessage)obj;
                        listen.onMessage(msg);
                        rc = 0;
                        if (msg.getQoS() > 0) {
                            connect.acknowledge(msg, rc, null);
                        }
                    } else if (obj instanceof Throwable ) {       /* Disconnect with exception */
                        listen.onDisconnect(-1, (Throwable)obj);
                        break;
                    } else if (obj instanceof IsmMqttResult) {          /* Disconnect with return code */
                        IsmMqttResult bsm = (IsmMqttResult)obj;
                        // System.out.println("result op="+bsm.getOperation() + " rc=" + bsm.getReasonCode());
                        if (bsm.getOperation() == IsmMqttConnection.DISCONNECT) {
                            listen.onDisconnect(bsm.getReasonCode(), bsm.getThrowable());
                            break;
                        }
                    }
                } catch (InterruptedException iex) { 
                }
            }
        }
    }    
}
