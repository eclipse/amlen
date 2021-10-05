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

import java.util.concurrent.atomic.AtomicInteger;

import javax.jms.MessageListener;
import javax.jms.Session;

/*
 * Implement asynchronous receive (using a message listener)
 */
final class ImaReceiveAction {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    MessageListener        			  listener;
    ImaMessageConsumer  			  consumer;
    ImaSession						  session;
    
    static final ThreadLocal<ImaConnection> currentConnection = new ThreadLocal<ImaConnection>();
        
    private final AtomicInteger       state = new AtomicInteger(0);
    /*
     * Create an action without buffer.
     * This is used for async receive 
     */
    ImaReceiveAction (ImaMessageConsumer consumer, MessageListener listener) {
        this.listener = listener;
        this.consumer = consumer;
        session = consumer.session;
    }
            
    /*
     * Deliver a message to a message listener.
     * 
     * JMS requires that only one message be delivered in a session at a time, so we implement this by
     * using the delivery lock.  The only threads competing for this lock are this thread and the session
     * thread.  In general there should be very little contention on this lock. 
     */
    void deliver(ImaMessage msg) {
        if (consumer.isClosed || consumer.isStopped || !state.compareAndSet(0, 1)){
            return;
        }
        long ack_sqn = msg.ack_sqn;
		if ((ack_sqn - session.lastDeliveredMessage) > 0) {
			try {
				consumer.lastDeliveredMessage = ack_sqn;
				session.lastDeliveredMessage = ack_sqn;
				/* Call the message listener */
				if (ImaTrace.isTraceable(9))
					ImaTrace.trace(9, "Message delivered: "
							+ msg.toString(ImaConstants.TS_All));
				currentConnection.set(session.connect);
				listener.onMessage(msg);
                currentConnection.set(null);
			} catch (Exception ex) {
				if (ImaTrace.isTraceable(2)) {
					ImaTrace.trace("Exception in onMessage: consumer="
							+ consumer.toString(ImaConstants.TS_All) + " ack_sqn="
							+ ack_sqn);
					ImaTrace.traceException(ex);
				}
				
				/*
				 * The JMS spec (for some strange reason) says that if a runtime exception is thrown by the
				 * message listener and allows us to control the retry count.  We choose a retry of 1.
				 */
				if (session.ackmode == Session.AUTO_ACKNOWLEDGE || session.ackmode == Session.DUPS_OK_ACKNOWLEDGE) {
					try {
						listener.onMessage(msg);
	                    currentConnection.set(null);
					} catch (Exception xex) {
	                    currentConnection.set(null);
						if (ImaTrace.isTraceable(2)) {
							ImaTrace.trace("Exception in onMessage redeliver: consumer="
									+ consumer.toString(ImaConstants.TS_All) + " ack_sqn="
									+ ack_sqn);
							ImaTrace.traceException(xex);
						    consumer.connect.raiseException(new ImaJmsExceptionImpl(
									"CWLNC0079", xex, "A call to onMessage() failed due to an unhandled exception."));
						}
					}
				} else {
                    currentConnection.set(null);
				    consumer.connect.raiseException(new ImaJmsExceptionImpl(
						"CWLNC0079", ex, "A call to onMessage() failed due to an unhandled exception."));
				}
			}
			
		}
		consumer.onAsyncDelivery(msg);
    	state.compareAndSet(1,0);
		return;
    }
    final synchronized void stopDelivery(boolean force){
        if (ImaTrace.isTraceable(6)) {
            ImaTrace.trace("Going to stop message delivery for consumer: id=" + consumer.consumerid + " force="+force);
        }
        if (force) {
            state.set(2);
        } else {
            while((state.get() != 2) && (!state.compareAndSet(0, 2))){
                try {
                    wait(10);
                } catch (InterruptedException e) {
                }
            }            
        }
        if (ImaTrace.isTraceable(6)) {
            ImaTrace.trace("Message delivery stopped for consumer: id=" + consumer.consumerid);
        }
    }
    final void startDelivery() {
        if (ImaTrace.isTraceable(6)) {
            ImaTrace.trace("Start message delivery for consumer: id=" + consumer.consumerid);
        }
        state.set(0);
    }
}
