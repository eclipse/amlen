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

package com.ibm.ima.ra.inbound;

import java.io.File;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.security.AccessController;
import java.security.PrivilegedActionException;
import java.security.PrivilegedExceptionAction;

import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.ra.ImaResourceAdapter;

abstract class FailedDeliveryHandler {
    private final long                 maxErrorCount;
    private long                       errorCount = 0;
    protected final ImaMessageEndpoint endpoint;

    private int                        lastEvent   = 0;
    private long                       lastErrorTS = 0;

    static final FailedDeliveryHandler createDeliveryHandler(final ImaMessageEndpoint endpoint, final long maxDeliveryFailures)
    {
        if (ImaResourceAdapter.outsideWAS())
            return new ImaDefaultFailedDeliveryHandler(endpoint, maxDeliveryFailures);
        PrivilegedExceptionAction<FailedDeliveryHandler> pa = new PrivilegedExceptionAction<FailedDeliveryHandler>() {
            public FailedDeliveryHandler run() throws Exception {
                return createWSHandler(endpoint, maxDeliveryFailures);
            }
        };

        try {
            return AccessController.doPrivileged(pa);
        } catch (PrivilegedActionException e) {
            ImaTrace.trace(2, "Failure creating FailedDeliveryHandler.  Pause on failure is not available.");
            ImaTrace.traceException(e);
            return new ImaDefaultFailedDeliveryHandler(endpoint, maxDeliveryFailures);
        }
    }

    private static FailedDeliveryHandler createWSHandler(ImaMessageEndpoint endpoint, long maxDeliveryFailures) throws Exception {
        String sibServerJar = System.getProperty("was.install.root") + "/plugins/com.ibm.ws.sib.server.jar";
        Object[] url = { (new File(sibServerJar)).toURI().toURL() };
        ClassLoader clLoader = Thread.currentThread().getContextClassLoader();
        Method addURL = URLClassLoader.class.getDeclaredMethod("addURL", new Class<?>[] { URL.class });
        addURL.setAccessible(true);
        addURL.invoke(clLoader, url);
        Class<?> cl = clLoader.loadClass("com.ibm.ima.ra.inbound.ImaWSFailedDeliveryHandler");
        @SuppressWarnings("unchecked")
        Constructor<FailedDeliveryHandler> ctor = (Constructor<FailedDeliveryHandler>) cl
                .getDeclaredConstructor(new Class<?>[] { ImaMessageEndpoint.class, long.class });
        return ctor.newInstance(endpoint, maxDeliveryFailures);
    }

    protected FailedDeliveryHandler(ImaMessageEndpoint endpoint, long maxDeliveryFailures) {
        this.endpoint = endpoint;
        maxErrorCount = (maxDeliveryFailures < 0) ? Long.MAX_VALUE : maxDeliveryFailures;
    }

    synchronized void onDeliveryFailure(Exception ex) {
        /* TODO: Review if we should handle this exception differently */
        /*
         * if (ex instanceof javax.ejb.TransactionRolledbackLocalException) { endpoint.trace(2,
         * "Transaction rolled back from onMessage: " + ex); endpoint.traceException(5, ex); return; }
         */
        long currTime = System.currentTimeMillis();
        if ((lastEvent == 0) && /* "Good" message was received since last error*/
            ((currTime - lastErrorTS) > 30000)) { /* More than 30 sec passed since last error */
                errorCount = 0;
        }
        lastEvent = 1;
        lastErrorTS = currTime;
        errorCount++;
        if ((pauseSupported()) && (errorCount > maxErrorCount) && (endpoint.pause())) {
            endpoint.trace(2, "Exception " + ex + " thrown from onMessage callback. Going to pause message endpoint.");
            pauseEndpoint();
        } else {
            endpoint.trace(2, "Exception " + ex + " thrown from onMessage callback. Error count is: " + errorCount);
        }
    }

    synchronized void onDeliverySuccess() {
        lastEvent = 0;
    }

    protected abstract void pauseEndpoint();

    protected abstract boolean pauseSupported();

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
