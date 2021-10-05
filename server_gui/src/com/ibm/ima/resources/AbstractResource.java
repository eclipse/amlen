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

package com.ibm.ima.resources;

import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URLEncoder;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.UriBuilder;
import javax.ws.rs.core.UriInfo;

import com.ibm.ima.resources.data.ApplianceResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.SecurityContext;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

public abstract class AbstractResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    static String CONTEXT_ROOT = "/rest";
    protected static final String IMA_CLI = "/bin/imacli";
    protected static final String BEDROCK_CLI = "/bin/cli";

    private final static String CLAS = AbstractResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    UriInfo uriInfo;

    protected Locale clientLocale = Locale.ENGLISH;

    protected String userId = null;

    public AbstractResource() {
        getUserFromContext();
        getClientLocaleFromContext();
    }


    /**
     * 
     */
    public void getUserFromContext() {
        SecurityContext context = SecurityContext.getContext();
        if (context != null) {
            userId = SecurityContext.getContext().getUser();
        }
        logger.trace(CLAS, "getUserFromContext", "userId is " + userId);
    }


    protected URI getResourceUri(String resource) {
        URI result = null;

        if (uriInfo != null) {
            UriBuilder builder = uriInfo.getRequestUriBuilder();
            try {
                result = builder.path(URLEncoder.encode(resource, "UTF-8")).build();
            } catch (UnsupportedEncodingException e) {
                logger.log(LogLevel.LOG_WARNING, CLAS, "getResourceUri", "CWLNA5002", e);
            }
        } else {
            logger.log(LogLevel.LOG_ERR, CLAS, "getResourceUri", "CWLNA5002", new Object[] {resource});
            //System.err.println("uriInfo for this resource is null");
        }

        return result;
    }

    /**
     * Obtain the locale requested by the client. This method
     * retrieves the locale directly from the SecurityContext
     * instance for the given thread. 
     *  
     * @return The locale that best matches that requested
     *         by the client.
     */
    protected Locale getClientLocaleFromContext() {
        SecurityContext context = SecurityContext.getContext();
        if (context != null) {
            clientLocale = SecurityContext.getContext().getLocale();
        }
        return this.clientLocale;
    }

    /**
     * Get the userId
     * @return
     */
    protected String getUserId() {
        
        if (userId == null) {
            getUserFromContext();
        }
        return this.userId;
    }
    
    /**
     * Obtain the locale requested by the client. Simply returns the
     * locale for this instance. If the locale happens to be null
     * the locale is retrieved from the SecurityContext instance for
     * the given thread. 
     * 
     */
    protected Locale getClientLocale() {
    	
    	if (clientLocale == null) {
    		getClientLocaleFromContext();
    	}
    	return this.clientLocale;
    }
    
    /**
     * Runs the command with applicable arguments. The actual command name
     * and the arguments are in the List command. This method does not assume
     * non-zero error codes from the command are an error. The error code
     * returned along with stdout and stderr will be placed in an 
     * ApplianceResponse instance and returned. 
     * 
     * @param command Command to run with optional arguments
     * @param action action substitution variable for message on fail
     * @param errorCode optional custom error message id
     * @return An ApplianceResponse instance
     */
    protected static ApplianceResponse runCommand(List<String> command, String action, String errorCode)  throws IsmRuntimeException {

    	ApplianceResponse cmdResponse;

    	if (errorCode == null) {
    		errorCode = "CWLNA5063";
    	}

    	String[] output = new String[2];
    	int rc = -1;
    	try {

    		StringBuilder commandToRun = new StringBuilder();
    		for (String s : command) {
    			commandToRun.append(s);
    			commandToRun.append(" ");
    		}

    		logger.trace( CLAS, "runCommand", "About to run command " + commandToRun.toString());

    		ProcessBuilder pb = new ProcessBuilder(command);
    		Process process = pb.start();
    		rc = Utils.getUtils().getOutput(process, output);

    		cmdResponse = new ApplianceResponse(rc, output[Utils.OUT_MESSAGE]);
    		cmdResponse.setError(output[Utils.ERR_MESSAGE]);
    		logger.trace(CLAS, "runCommand", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE] + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);

    	} catch (Exception e) {
    		logger.log(LogLevel.LOG_ERR, CLAS, "runCommand", "CWLNA5063", new Object[] {action}, e);
    		throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, errorCode, new Object[] {action} );
    	}



    	logger.traceExit(CLAS, "runCommand", cmdResponse);
    	return cmdResponse;
    }
    
    /**
     * Wrapper for runCommand. This method assumes that any non-zero error
     * code is an error. When a non-zero error code is detected an error
     * with the status of INTERNAL_SERVER_ERROR will be created and thrown.
     * The message for the error is created using the errorCode provided or
     * a default of CWLNA5063. When the message is bound the action will
     * be provided as a substitution parameter. This is the only parameter
     * that will be substituted.
     * 
     * @param command   The command to run (i.e. a shell script or cli)
     * @param action    A string that describes the action this command will
     *                  do. The value will be used as a variable for 
     *                  substitution if an error is detected.
     * @param errorCode Optional custom error message id. Used if an error
     *                  is detected.
     * @return          Response instance only if the actual running of 
     *                  the command returns a code of zero.
     * @throws IsmRuntimeException Thrown when the command fails or a 
     *                             non-zero return code is detected.
     */
    protected Response runCommandWrapper(String command, String action,  String errorCode) throws IsmRuntimeException {
    	
        logger.traceEntry(CLAS, "runCommandWrapper");
        
    	if (errorCode == null) {
    		errorCode = "CWLNA5063";
    	}
        
    	List<String> commandList = Arrays.asList(command);
    	ApplianceResponse response =  runCommand(commandList, action, errorCode);
    	
    	if (response.getRc() != 0) {
    		logger.log(LogLevel.LOG_ERR, CLAS, "runCommandWrapper", errorCode, new Object[] {action});
    	    throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, errorCode, new Object[] {action} );
    	}
    	
        ResponseBuilder rb =  Response.ok(response);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);

        logger.traceExit(CLAS, "runCommandWrapper", response);
        return rb.cacheControl(cache).build();
    	
    }
    
}
