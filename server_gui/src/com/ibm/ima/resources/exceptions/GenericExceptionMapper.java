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

import java.util.Locale;

import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.ext.ExceptionMapper;
import javax.ws.rs.ext.Provider;

import com.ibm.ima.resources.data.ExceptionResponse;
import com.ibm.ima.security.SecurityContext;
import com.ibm.ima.util.IsmLogger;

/*
 * Maps IsmRuntimeExceptions to a HTTP response
 */
@Provider
public class GenericExceptionMapper extends AbstractExceptionMapper<Throwable>
implements ExceptionMapper<Throwable> {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = GenericExceptionMapper.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Override
    public Response toResponse(Throwable exception) {
        logger.traceEntry(CLAS, "toResponse", new Object[]{exception});

        ResponseBuilder responseBuilder;
        /* 
         * Responses should be localized. Obtain the locale requested by
         * the client.
         */
        Locale clientLocale = Locale.ENGLISH;
    	SecurityContext context = SecurityContext.getContext();
        if (context != null) {
        	clientLocale = SecurityContext.getContext().getLocale();
        }
        if (exception instanceof IsmRuntimeException) {
            responseBuilder = Response.status(((IsmRuntimeException) exception).getHttpStatusCode());
            responseBuilder.type(MediaType.APPLICATION_JSON_TYPE);
            responseBuilder.entity(new ExceptionResponse(exception, clientLocale));
        } else {
            responseBuilder = Response.status(Status.INTERNAL_SERVER_ERROR);
            responseBuilder.type(MediaType.APPLICATION_JSON_TYPE);
            responseBuilder.entity(new ExceptionResponse(new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, exception, "CWLNA5000", null), clientLocale));
        }

        Response response = responseBuilder.build();
        logger.traceExit(CLAS, "toResponse", response);
        return response;
    }
}
