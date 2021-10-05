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
package com.ibm.ima.jms.impl;

import javax.jms.JMSException;

/*
 * Stub to return runtime exception when required
 */
public class ImaRuntimeException {
    public static RuntimeException mapException(Throwable t) {
        if (t instanceof RuntimeException)
            return (RuntimeException)t;
        String msg;
        if (t instanceof JMSException) {
            msg = ((JMSException)t).getErrorCode() + ' ' + t.getMessage();
            if (t instanceof ImaJmsExceptionImpl) {
                return new ImaJmsRuntimeException(((JMSException) t).getErrorCode(), t.getMessage(), t);
            }
            if (t instanceof ImaIllegalStateException) {
                return new ImaIllegalStateRuntimeException(((JMSException) t).getErrorCode(), t.getMessage(), t);
            }
            if (t instanceof ImaInvalidDestinationException) {
                return new ImaInvalidDestinationRuntimeException(((JMSException) t).getErrorCode(), t.getMessage(), t);
            }
            if (t instanceof ImaInvalidClientIDException) {
                return new ImaInvalidClientIDRuntimeException(((JMSException) t).getErrorCode(), t.getMessage(), t);
            }
            if (t instanceof ImaInvalidSelectorException) {
                return new ImaInvalidSelectorRuntimeException(((JMSException) t).getErrorCode(), t.getMessage(), t);
            }
            if (t instanceof ImaJmsSecurityException) {
                return new ImaJmsSecurityRuntimeException(((JMSException) t).getErrorCode(), t.getMessage(), t);
            }
            if (t instanceof ImaMessageFormatException) {
                return new ImaMessageFormatRuntimeException(((JMSException) t).getErrorCode(), t.getMessage(), t);
            }
            if (t instanceof ImaMessageNotWriteableException) {
                return new ImaMessageNotWriteableRuntimeException(((JMSException) t).getErrorCode(), t.getMessage(), t);
            }
        } else {
            msg = t.getMessage();
        }    
        return new RuntimeException(msg, t);
    }        
}
