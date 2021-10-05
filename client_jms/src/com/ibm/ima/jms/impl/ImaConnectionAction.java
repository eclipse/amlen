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

import java.io.IOException;

import javax.jms.JMSException;


/*
 * Implement the actions for acknowledge  
 */
public class ImaConnectionAction extends ImaAction {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	ImaClient	client;
	
    /*
     * Create an action with default size.
     */
	ImaConnectionAction (int action, ImaClient client) {
		this(action, client, 256);
	}

	/*
	 * Create an action with a specified size
	 */
	protected ImaConnectionAction (int action, ImaClient client, int size) {
		super(action, size);
		this.client = client;
	}

	/*
	 * Make the request
	 */
	void request(boolean syncCall) throws JMSException {
		outBB.flip();
		if (syncCall)
			setAction(corrid, this);
		try {
			client.send(outBB, syncCall);
		} catch (IOException e) {
			ImaJmsExceptionImpl jmsex = null;
			
			jmsex = new ImaJmsExceptionImpl("CWLNC0020", e, "The client failed when sending a message to MessageSight. This can occur if MessageSight closes the connection (for example, during shut down), or if MessageSight is stopped.  It can also occur due to network problems.");
			
			try {
				client.connect.markAllClosed(jmsex);
			} catch (Exception ex) {
				client.connect.isClosed.set(true);
			    client.connect.putInternal("isClosed", true);
			}
			
            if (ImaTrace.isTraceable(1)) {
                ImaTrace.trace("Failure on connection " + client.connect.toString(ImaConstants.TS_All) + "\nMarking connection closed.");
                ImaTrace.traceException(jmsex);
            }
            throw jmsex;
		} 
		catch (Exception e) {
			if (e instanceof JMSException)
				throw (JMSException)e;
			
			ImaJmsExceptionImpl jmsex = null;
			
			jmsex = new ImaJmsExceptionImpl("CWLNC0020", e, "The client failed when sending a message to MessageSight. This can occur if MessageSight closes the connection (for example, during shut down), or if MessageSight is stopped.  It can also occur due to network problems.");
			if (ImaTrace.isTraceable(1)) {
                ImaTrace.trace("Failure on connection " + client.connect.toString(ImaConstants.TS_All));
                ImaTrace.traceException(jmsex);
            }
			throw jmsex;
		}
				
    	/* Wait for a result */
   	    if (syncCall)
   	        await();
	}
	
}
