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

package com.ibm.ima.ra.outbound;

import java.util.Enumeration;

import javax.jms.JMSException;
import javax.jms.Queue;
import javax.jms.QueueBrowser;

import com.ibm.ima.jms.impl.ImaQueueBrowser;

public class ImaQueueBrowserProxy extends ImaProxy implements QueueBrowser {
    /** the Destination to receive messages from */
    private final Queue           queue;
    /** the Consumer wrapped by this class */
    private final ImaQueueBrowser browser;

    ImaQueueBrowserProxy(ImaSessionProxy s, ImaQueueBrowser b, Queue q) {
        super(s);
        queue = q;
        browser = b;
    }

    protected void closeImpl(boolean connClosed) throws JMSException {
        if (!connClosed)
            browser.close();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.QueueBrowser#getEnumeration()
     */
    public Enumeration<?> getEnumeration() throws JMSException {
        return browser.getEnumeration();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.QueueBrowser#getMessageSelector()
     */
    public String getMessageSelector() throws JMSException {
        return browser.getMessageSelector();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.QueueBrowser#getQueue()
     */
    public Queue getQueue() throws JMSException {
        return queue;
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


}
