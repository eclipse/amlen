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

import java.security.AccessController;
import java.security.PrivilegedExceptionAction;
import java.util.Set;
import java.util.TimerTask;

import javax.management.ObjectName;

import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.ra.common.ImaException;
import com.ibm.websphere.csi.J2EEName;
import com.ibm.websphere.management.AdminService;
import com.ibm.websphere.management.AdminServiceFactory;
import com.ibm.ws.sib.security.BusSecurityExceptionAction;
import com.ibm.ws.sib.security.auth.AuthUtils;
import com.ibm.ws.sib.security.auth.AuthUtilsFactory;

final class ImaWSFailedDeliveryHandler extends FailedDeliveryHandler {
    private final J2EEName      j2eeName;
    private final ObjectName    objectName;
    private final AdminService adminService;
    private final AuthUtils    authUtils;

    ImaWSFailedDeliveryHandler(ImaMessageEndpoint endpoint, long maxDeliveryFailures) throws Exception {
        super(endpoint, maxDeliveryFailures);
        if (endpoint.endpointFactory instanceof com.ibm.ws.j2c.MessageEndpointFactory) {
            com.ibm.ws.j2c.MessageEndpointFactory mef = (com.ibm.ws.j2c.MessageEndpointFactory) endpoint.endpointFactory;
            j2eeName = mef.getJ2EEName();
            objectName = new ObjectName("WebSphere:type=J2CMessageEndpoint,*,MessageDrivenBean=" + j2eeName);
            adminService = AdminServiceFactory.getAdminService();
            authUtils = AuthUtilsFactory.getInstance().getAuthUtils();
            return;
        }
        ImaException ex = new ImaException("CWLNC2941","Failed to create handler {0} for failed message deliveries because the MessageEndpointFactory type {1} is not supported. The MessageSight resource adapter will function correctly. However, the maxDeliveryFailures activation specification property will not be used to pause the message endpoint.",
                this.getClass().getName(),endpoint.endpointFactory.getClass().getName());
        ImaTrace.traceException(1, ex);
        throw ex;
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.ra.inbound.FailedDeliveryHandler#pauseEndpoint()
     */
    protected void pauseEndpoint() {
        TimerTask tt = new TimerTask() {
            public void run() {
                PrivilegedExceptionAction<Object> pa = new PrivilegedExceptionAction<Object>() {
                    public Object run() throws Exception {
                        doPause();
                        return null;
                    }
                };
                try {
                    AccessController.doPrivileged(pa);
                } catch (Exception ex) {
                    ImaTrace.traceException(1, ex);
                }
            }
        };
        endpoint.timer.schedule(tt, 0);
    }

    private final void doPause() throws Exception {
        @SuppressWarnings("unchecked")
        final Set<ObjectName> objects = adminService.queryNames(objectName, null);
        for (ObjectName object : objects) {
            final ObjectName resolvedObject = object;
            BusSecurityExceptionAction<Object, Exception> bsea = new BusSecurityExceptionAction<Object, Exception>() {
                public Object run() throws Exception {
                    return adminService.invoke(resolvedObject, "pause", null, null);
                }
            };
            authUtils.runAsSystem(bsea);
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.ra.inbound.FailedDeliveryHandler#pauseSupported()
     */
    protected boolean pauseSupported() {
        return true;
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
