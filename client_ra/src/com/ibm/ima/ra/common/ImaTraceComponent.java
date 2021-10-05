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

/*
 * TODO: move this logic into the JMS client trace support
 */
package com.ibm.ima.ra.common;

import java.io.PrintWriter;
import java.io.Serializable;

public final class ImaTraceComponent implements Cloneable, Serializable {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final long     serialVersionUID = 5643648219000720412L;
    private transient PrintWriter writer;
    private int                   logLevel;

    public ImaTraceComponent(PrintWriter pw, int logLev) {
        writer = pw;
        logLevel = logLev;
    }

    public void setLogLevel(int ll) {
        logLevel = ll;
    }

    public void setLogWriter(PrintWriter pw) {
        writer = pw;
    }

    public PrintWriter getLogWriter() {
        return writer;
    }

    public void trace(int level, String msg) {
        PrintWriter pw = writer;
        if ((pw != null) && (level <= logLevel)) {
            pw.println(msg);
        }
    }

    public boolean isTraceAllowed(int level) {
        return ((writer != null) && (level <= logLevel));
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#clone()
     */
    public ImaTraceComponent clone() {
        return new ImaTraceComponent(writer, logLevel);
    }
}
