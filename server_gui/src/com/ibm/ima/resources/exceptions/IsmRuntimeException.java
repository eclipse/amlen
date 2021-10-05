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

package com.ibm.ima.resources.exceptions;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.util.Locale;

import javax.ws.rs.core.Response.Status;

import com.ibm.ima.msgcatalog.IsmUIListResourceBundle;
import com.ibm.ima.util.IsmLogger;

public class IsmRuntimeException extends RuntimeException {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final long serialVersionUID = 3250639673907864144L;
    final Status httpStatus;
    final String messageID;
    final String message;
    final Object[] params;
	private boolean nativeMessage = false;

    private final static String CLAS = IsmRuntimeException.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    /*
     * Fail the current request with a specific status, message
     */
    public IsmRuntimeException(final Status httpStatus, Locale locale, final String messageID, final Object[] params) {
        super();

        logger.traceEntry(CLAS, "ctor", new Object[]{httpStatus, messageID, params});

        this.httpStatus = httpStatus;
        this.messageID = messageID;
        this.params = params;
        this.message = logger.getFormattedMessage(messageID, params, locale, false);

        logger.traceExit(CLAS, "ctor");
    }

    /*
     * Fail the current request with a specific status, message
     */
    public IsmRuntimeException(final Status httpStatus, final String messageID, final Object[] params) {
        super();

        logger.traceEntry(CLAS, "ctor", new Object[]{httpStatus, messageID, params});

        this.httpStatus = httpStatus;
        this.messageID = messageID;
        this.params = params;
        this.message = logger.getFormattedMessage(messageID, params, null, false);

        logger.traceExit(CLAS, "ctor");
    }
    
    /*
     * Fail the current request with a specific status, message ID and message
     */
    public IsmRuntimeException(final Status httpStatus, final String messageID, final String messageText, final boolean nativeMessage) {
        super();

        logger.traceEntry(CLAS, "ctor", new Object[]{httpStatus, messageID, messageText});
        this.nativeMessage = nativeMessage;
        this.httpStatus = httpStatus;
        this.messageID = messageID;
        this.params = null;
        this.message = messageText;

        logger.traceExit(CLAS, "ctor");
    }


    /*
     * Fail the current request with a specific status, message
     */
    public IsmRuntimeException(final Status httpStatus, final Throwable cause, final String messageID, final Object[] params) {
        super(cause);

        logger.traceEntry(CLAS, "ctor", new Object[]{httpStatus, cause, messageID, params});

        this.httpStatus = httpStatus;
        this.messageID = messageID;
        this.params = params;
        this.message = logger.getFormattedMessage(messageID, params, null, false);

        logger.traceExit(CLAS, "ctor");
    }

    /*
     * Fail the current request with the default generic message
     */
    public IsmRuntimeException(final Throwable cause) {
        super(cause);

        logger.traceEntry(CLAS, "ctor", new Object[]{cause});

        this.httpStatus = Status.INTERNAL_SERVER_ERROR;
        this.messageID = IsmUIListResourceBundle.CWLNA5000;
        this.params = null;
        this.message = logger.getFormattedMessage(this.messageID, null, null, false);

        logger.traceExit(CLAS, "ctor");
    }

    public Status getHttpStatusCode() {
        return httpStatus;
    }

    public String getMessageID() {
        return messageID;
    }

    public Object[] getParams() {
        return params;
    }

    @Override
    public String getMessage() {
        return message;
    }
    
    @SuppressWarnings("unused")
    private static void printStackTrace(Throwable cause) {
        final Writer writer = new StringWriter();
        final PrintWriter printWriter = new PrintWriter(writer);
        cause.printStackTrace(printWriter);
        logger.trace(CLAS, "printStackTrace", writer.toString());
    }

	public boolean isNativeMessage() {
		// TODO Auto-generated method stub
		return nativeMessage;
	}
}
