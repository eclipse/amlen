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
package com.ibm.ima.ra.common;

//import java.io.PrintWriter;
//import java.io.StringWriter;

import javax.jms.JMSException;

import com.ibm.ejs.ras.Tr;
import com.ibm.ejs.ras.TraceComponent;
import com.ibm.ima.jms.impl.ImaTraceImpl;
import com.ibm.ima.jms.impl.ImaTraceable;

/**
 * This class provides access to WebSphere Application Server (WAS) trace levels.
 * It allows for MessageSight trace levels to change at runtime based on
 * changes to the WAS trace level settings.  This class is intended to assist
 * in production trouble shooting.  This class will only be used if:
 *     1. A MessageSight resource adapter is running on WAS
 * AND 2. The MessageSight resource adapter enableDynamicTrace property is
 *        set to true.  (It is false by default.)
 * 
 * @see com.ibm.ima.jms.impl.ImaTrace
 */
public final class ImaWASTraceComponent extends ImaTraceImpl implements ImaTraceable {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    final TraceComponent wasTC;
    final String traceName = "ImaTrace";
    
    /**
     * Constructor
     */   
    public ImaWASTraceComponent(String traceFile, int traceLevel, boolean suppressTimestamps) {
        super(traceFile, traceLevel, suppressTimestamps);
        wasTC = Tr.register(com.ibm.ima.ra.common.ImaWASTraceComponent.class, "MessageSight");
    }
    
    /**
     * Check if trace is allowed at the specified level.
     * @param level the trace level
     * @return true if the level is allowed to trace
     */
    public boolean isTraceable(int level) {
        if (TraceComponent.isAnyTracingEnabled()) {
            if (level < 5)
                return wasTC.isInfoEnabled();
            if (level < 8)
                return wasTC.isDetailEnabled();
            if (level < 10)
                return wasTC.isDebugEnabled();
        }
        return false;
    }

    public void trace(int level, String message) {
        if (isTraceable(level))
            super.trace(message);
    }
    
    public JMSException traceException(int level, Throwable ex) {
        if (isTraceable(level))
            super.traceException(ex);
        return (ex instanceof JMSException) ? (JMSException)ex : null;
    }
}
