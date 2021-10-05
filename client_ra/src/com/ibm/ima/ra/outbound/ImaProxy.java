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

import java.util.concurrent.atomic.AtomicBoolean;

import javax.jms.JMSException;

abstract class ImaProxy {
    ImaProxyContainer             container;

    protected final AtomicBoolean isClosed = new AtomicBoolean(false);

    ImaProxy(ImaProxyContainer ipc) {
        container = ipc;
    }

    protected abstract void closeImpl(boolean connClosed) throws JMSException;

    void closeInternal(boolean connClosed) throws JMSException {
        if (isClosed.compareAndSet(false, true)) {
            if (connClosed)
                return;
            closeImpl(connClosed);
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.MessageConsumer#close()
     */
    public void close() throws JMSException {
        if (isClosed.compareAndSet(false, true)) {
            try {
                closeImpl(false);
            } finally {
                container.onClose(this);
            }
        }
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
