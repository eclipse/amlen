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

package com.ibm.ima.resources;

import java.io.IOException;

import javax.ws.rs.core.Response.Status;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.core.type.TypeReference;

import com.ibm.ima.admin.IsmServerClient;
import com.ibm.ima.admin.request.IsmConfigRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;
//import com.ibm.ima.admin.request.IsmConfigGetHARoleRequest;
//import com.ibm.ima.resources.data.HARoleResponse;

/**
 * Base class for ISM configuration resources
 */
public abstract class AbstractIsmConfigResource extends AbstractResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = AbstractIsmConfigResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    protected static final String REFERENCE_NOT_FOUND = Utils.NOT_FOUND_RC;
    protected static final String IN_USE = "18";
    protected static final String CLIENT_IN_USE = "213";
    protected static final String ISMRC_REQUEST_REJECTED = "208";
    protected static final String ISMRC_EndpointMsgPolError = "357";
    

    public AbstractIsmConfigResource() {
        super();
    }

    protected Object sendSetConfigRequest(IsmConfigRequest configRequest) {
        logger.traceEntry(CLAS, "sendSetConfigRequest", new Object[]{configRequest});

        
        Object result = null;
        try {
//            // We need to ensure the server is not in Standby mode before we allow a set of configuration data
//            if (!configRequest.isUpdateAllowedInStandby()) {
//                checkForStandbyMode(configRequest.getServerInstance());
//            }

            // set the locale preferred by the client
            configRequest.setLocale(getClientLocale().toString());
            IsmServerClient client = new IsmServerClient(configRequest, getClientLocale());
            String resultString = client.sendConfigMessage(configRequest);
            
            if (logger.isTraceEnabled()) {
                logger.trace(CLAS, "sendSetConfigRequest", "resultString for " + configRequest.toString() + ": " + configRequest.applyMask(resultString));
            }

            if (resultString == null || resultString.trim().length() == 0) {
                return "";
            }

            if (!resultString.startsWith("{") && !resultString.startsWith("[ {")) {
                // its not json, don't bother to try to parse it
                return resultString;
            }

            ObjectMapper mapper = new ObjectMapper();

            try {
                IsmResponse response = mapper.readValue(resultString, IsmResponse.class);
                logger.trace(CLAS, "sendSetConfigRequest", "IsmResponse for " + configRequest.toString() + ": " + response.toString());
                if (response.RC != null && response.RC.equals(Utils.OBJECT_ALREADY_EXISTS_RC)) {
                    throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5051", null);
                }
                result = response;
            } catch (IOException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "sendSetConfigRequest", "CWLNA5000", e);

                if (result == null) {
                    result = resultString;
                }
            }
        } catch (Throwable t) {
            if (t instanceof IsmRuntimeException) {
                String request = configRequest != null ? configRequest.toString() : "";
                logger.trace(CLAS, "sendSetConfigRequest: "+request, ((IsmRuntimeException) t).getMessage());
                throw (IsmRuntimeException)t;
            }
            logger.log(LogLevel.LOG_ERR, CLAS, "sendSetConfigRequest", "CWLNA5000", t);
            result = t.toString();
        }

        logger.traceExit(CLAS, "sendSetConfigRequest", result);
        return result;
    }

//    /**
//     * Checks to make sure the system is not in standby.
//     * @throws IsmRuntimeException if the getting the harole throws an exception or the harole is standby
//     */
//    protected void checkForStandbyMode(String serverInstance) throws IsmRuntimeException {
//        Object resultObject = null;
//        try {
//            IsmConfigGetHARoleRequest request = new IsmConfigGetHARoleRequest(userId);
//            request.setServerInstance(serverInstance);
//            resultObject = sendGetConfigRequest(request, new TypeReference<HARoleResponse>(){});
//        } catch (IsmRuntimeException ire) {
//            throw ire;
//        } catch (Throwable t) {
//            throw new IsmRuntimeException(Status.FORBIDDEN, t, "CWLNA5104", null);
//        }
//        if (resultObject instanceof HARoleResponse) {
//            HARoleResponse harole = (HARoleResponse)resultObject;            
//            if (harole.isStandby()) {
//                throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5103", null);
//            }
//        } else {
//            throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5104", null);
//        }
//
//    }

    protected Object sendGetConfigRequest(IsmConfigRequest configRequest, TypeReference<?> typeRef) {
        logger.traceEntry(CLAS, "sendGetConfigRequest", new Object[]{configRequest});

        Object result = null;

        try {
            // set the locale preferred by the client
            configRequest.setLocale(getClientLocale().toString());
            IsmServerClient client = new IsmServerClient(configRequest, getClientLocale());
            String resultString = client.sendConfigMessage(configRequest);

            if (logger.isTraceEnabled()) {
                logger.trace(CLAS, "sendGetConfigRequest", "resultString for " + configRequest.toString() + ": " + configRequest.applyMask(resultString));
            }

            if (resultString == null || resultString.trim().length() == 0) {
                return "";
            }

            String resultStringNoSpace = resultString.replaceAll("\\s+", "");
            if (!resultStringNoSpace.startsWith("{") && !resultStringNoSpace.startsWith("[{")) {
                // its not json, don't bother to try to parse it
                return resultString;
            }

            if (typeRef == null) {
                return resultString;
            }

            ObjectMapper mapper = new ObjectMapper();
            try {
                result = mapper.readValue(resultString, typeRef);
            } catch (JsonParseException e) {
                // If we fail to parse, parse as IsmResponse
                try {
                    IsmResponse response = mapper.readValue(resultString, IsmResponse.class);
                    logger.trace(CLAS, "sendGetConfigRequest", "IsmResponse: " + response.toString());
                    result = response;
                } catch (Exception ex) {
                    logger.log(LogLevel.LOG_ERR, CLAS, "sendGetConfigRequest", "CWLNA5003", e);
                }
            } catch (JsonMappingException e) {
                // If we fail to parse, parse as IsmResponse
                try {
                    IsmResponse response = mapper.readValue(resultString, IsmResponse.class);
                    logger.trace(CLAS, "sendGetConfigRequest", "IsmResponse for " + configRequest.toString() + ": " + response.toString());
                    result = response;
                } catch (Exception ex) {
                    logger.log(LogLevel.LOG_ERR, CLAS, "sendGetConfigRequest", "CWLNA5003", e);
                }
            } catch (IOException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "sendGetConfigRequest", "CWLNA5000", e);
            }

            if (result == null) {
                result = resultString;
            }
        } catch (Throwable t) {
            if (t instanceof IsmRuntimeException) {
                String request = configRequest != null ? configRequest.toString() : "";
                logger.trace(CLAS, "sendGetConfigRequest: "+request, ((IsmRuntimeException) t).getMessage());
                throw (IsmRuntimeException)t;
            }
            logger.log(LogLevel.LOG_ERR, CLAS, "sendSetConfigRequest", "CWLNA5000", t);
            result = t.toString();
        }

        return result;
    }
    
    /**
     * Throws an IsmRuntimeException. If rc maps to a good error message, uses it. Otherwise, 
     * Uses the provided errorString and CWLNA0+rc, padding rc with leading zeros if necessary
     * @param rc
     * @param errorString
     */
	protected void throwException(String rc, String errorString) {
		String errorMessageId = Utils.getUtils().getIsmErrorMessage(rc);
		if ("CWLNA5005".equals(errorMessageId) && rc.length() <= 3) {
			// do better... Use the RC
			while (rc.length() < 3) {
				rc = "0"+rc;
			}
			errorMessageId = "CWLNA0"+rc;
			throw new IsmRuntimeException(Status.BAD_REQUEST, errorMessageId, errorString, true);
		}
		// Bad result, throw an exception and include the error message
		throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, errorMessageId, new Object[] { errorString });
	}


}
